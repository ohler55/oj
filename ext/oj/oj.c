/* oj.c
 * Copyright (c) 2012, Peter Ohler
 *
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * - Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 * 
 * - Neither the name of Peter Ohler nor the names of its contributors may be
 *   used to endorse or promote products derived from this software without
 *   specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

#include "oj.h"
#include "parse.h"
#include "hash.h"
#include "odd.h"
#include "encode.h"

typedef struct _YesNoOpt {
    VALUE	sym;
    char	*attr;
} *YesNoOpt;

void Init_oj();

VALUE	 Oj = Qnil;

ID	oj_add_value_id;
ID	oj_array_append_id;
ID	oj_array_end_id;
ID	oj_array_start_id;
ID	oj_as_json_id;
ID	oj_error_id;
ID	oj_file_id;
ID	oj_fileno_id;
ID	oj_ftype_id;
ID	oj_hash_end_id;
ID	oj_hash_key_id;
ID	oj_hash_set_id;
ID	oj_hash_start_id;
ID	oj_iconv_id;
ID	oj_instance_variables_id;
ID	oj_json_create_id;
ID	oj_length_id;
ID	oj_new_id;
ID	oj_parse_id;
ID	oj_pos_id;
ID	oj_read_id;
ID	oj_readpartial_id;
ID	oj_replace_id;
ID	oj_stat_id;
ID	oj_string_id;
ID	oj_to_hash_id;
ID	oj_to_json_id;
ID	oj_to_s_id;
ID	oj_to_sym_id;
ID	oj_to_time_id;
ID	oj_tv_nsec_id;
ID	oj_tv_sec_id;
ID	oj_tv_usec_id;
ID	oj_utc_id;
ID	oj_utc_offset_id;
ID	oj_utcq_id;
ID	oj_write_id;

static ID	has_key_id;

VALUE	oj_bag_class;
VALUE	oj_bigdecimal_class;
VALUE	oj_cstack_class;
VALUE	oj_date_class;
VALUE	oj_datetime_class;
VALUE	oj_parse_error_class;
VALUE	oj_stream_writer_class;
VALUE	oj_string_writer_class;
VALUE	oj_stringio_class;
VALUE	oj_struct_class;

VALUE	oj_slash_string;

static VALUE	allow_gc_sym;
static VALUE	ascii_only_sym;
static VALUE	ascii_sym;
static VALUE	auto_define_sym;
static VALUE	auto_sym;
static VALUE	bigdecimal_as_decimal_sym;
static VALUE	bigdecimal_load_sym;
static VALUE	bigdecimal_sym;
static VALUE	circular_sym;
static VALUE	class_cache_sym;
static VALUE	compat_sym;
static VALUE	create_id_sym;
static VALUE	escape_mode_sym;
static VALUE	float_prec_sym;
static VALUE	float_sym;
static VALUE	huge_sym;
static VALUE	indent_sym;
static VALUE	json_parser_error_class;
static VALUE	json_sym;
static VALUE	mode_sym;
static VALUE	nan_sym;
static VALUE	newline_sym;
static VALUE	nilnil_sym;
static VALUE	null_sym;
static VALUE	object_sym;
static VALUE	quirks_mode_sym;
static VALUE	raise_sym;
static VALUE	ruby_sym;
static VALUE	sec_prec_sym;
static VALUE	strict_sym;
static VALUE	symbol_keys_sym;
static VALUE	time_format_sym;
static VALUE	unix_sym;
static VALUE	unix_zone_sym;
static VALUE	use_to_json_sym;
static VALUE	word_sym;
static VALUE	xmlschema_sym;
static VALUE	xss_safe_sym;

static VALUE	array_nl_sym;
static VALUE	create_additions_sym;
static VALUE	object_nl_sym;
static VALUE	space_before_sym;
static VALUE	space_sym;
static VALUE	symbolize_names_sym;

static VALUE	mimic = Qnil;

#if HAS_ENCODING_SUPPORT
rb_encoding	*oj_utf8_encoding = 0;
#else
VALUE		oj_utf8_encoding = Qnil;
#endif

#if USE_PTHREAD_MUTEX
pthread_mutex_t	oj_cache_mutex;
#elif USE_RB_MUTEX
VALUE oj_cache_mutex = Qnil;
#endif
static const char	json_class[] = "json_class";

struct _Options	oj_default_options = {
    0,		// indent
    No,		// circular
    No,		// auto_define
    No,		// sym_key
    JSONEsc,	// escape_mode
    ObjectMode,	// mode
    Yes,	// class_cache
    UnixZTime,	// time_format
    Yes,	// bigdec_as_num
    AutoDec,	// bigdec_load
    Yes,	// to_json
    No,		// nilnil
    Yes,	// allow_gc
    Yes,	// quirks_mode
    json_class,	// create_id
    10,		// create_id_len
    9,		// sec_prec
    15,		// float_prec
    "%0.15g",	// float_fmt
    {		// dump_opts
	false,	//use
	"",	// indent
	"",	// before_sep
	"",	// after_sep
	"",	// hash_nl
	"",	// array_nl
	0,	// indent_size
	0,	// before_size
	0,	// after_size
	0,	// hash_size
	0,	// array_size
	AutoNan,// nan_dump
    }
};

static VALUE	define_mimic_json(int argc, VALUE *argv, VALUE self);

/* call-seq: default_options() => Hash
 *
 * Returns the default load and dump options as a Hash. The options are
 * - indent: [Fixnum|String|nil] number of spaces to indent each element in an JSON document, zero or nil is no newline between JSON elements, negative indicates no newline between top level JSON elements in a stream, a String indicates the string should be used for indentation
 * - circular: [true|false|nil] support circular references while dumping
 * - auto_define: [true|false|nil] automatically define classes if they do not exist
 * - symbol_keys: [true|false|nil] use symbols instead of strings for hash keys
 * - escape_mode: [:newline|:json|:xss_safe|:ascii|nil] determines the characters to escape
 * - class_cache: [true|false|nil] cache classes for faster parsing (if dynamically modifying classes or reloading classes then don't use this)
 * - mode: [:object|:strict|:compat|:null] load and dump modes to use for JSON
 * - time_format: [:unix|:unix_zone|:xmlschema|:ruby] time format when dumping in :compat and :object mode
 * - bigdecimal_as_decimal: [true|false|nil] dump BigDecimal as a decimal number or as a String
 * - bigdecimal_load: [:bigdecimal|:float|:auto] load decimals as BigDecimal instead of as a Float. :auto pick the most precise for the number of digits.
 * - create_id: [String|nil] create id for json compatible object encoding, default is 'json_create'
 * - second_precision: [Fixnum|nil] number of digits after the decimal when dumping the seconds portion of time
 * - float_precision: [Fixnum|nil] number of digits of precision when dumping floats, 0 indicates use Ruby
 * - use_to_json: [true|false|nil] call to_json() methods on dump, default is false
 * - nilnil: [true|false|nil] if true a nil input to load will return nil and not raise an Exception
 * - allow_gc: [true|false|nil] allow or prohibit GC during parsing, default is true (allow)
 * - quirks_mode: [true,|false|nil] Allow single JSON values instead of documents, default is true (allow)
 * - indent_str: [String|nil] String to use for indentation, overriding the indent option is not nil
 * - space: [String|nil] String to use for the space after the colon in JSON object fields
 * - space_before: [String|nil] String to use before the colon separator in JSON object fields
 * - object_nl: [String|nil] String to use after a JSON object field value
 * - array_nl: [String|nil] String to use after a JSON array value
 * - nan: [:null|:huge|:word|:raise|:auto] how to dump Infinity and NaN in null, strict, and compat mode. :null places a null, :huge places a huge number, :word places Infinity or NaN, :raise raises and exception, :auto uses default for each mode.
 * @return [Hash] all current option settings.
 */
static VALUE
get_def_opts(VALUE self) {
    VALUE	opts = rb_hash_new();

    if (0 == oj_default_options.dump_opts.indent_size) {
	rb_hash_aset(opts, indent_sym, INT2FIX(oj_default_options.indent));
    } else {
	rb_hash_aset(opts, indent_sym, rb_str_new2(oj_default_options.dump_opts.indent_str));
    }
    rb_hash_aset(opts, sec_prec_sym, INT2FIX(oj_default_options.sec_prec));
    rb_hash_aset(opts, circular_sym, (Yes == oj_default_options.circular) ? Qtrue : ((No == oj_default_options.circular) ? Qfalse : Qnil));
    rb_hash_aset(opts, class_cache_sym, (Yes == oj_default_options.class_cache) ? Qtrue : ((No == oj_default_options.class_cache) ? Qfalse : Qnil));
    rb_hash_aset(opts, auto_define_sym, (Yes == oj_default_options.auto_define) ? Qtrue : ((No == oj_default_options.auto_define) ? Qfalse : Qnil));
    rb_hash_aset(opts, symbol_keys_sym, (Yes == oj_default_options.sym_key) ? Qtrue : ((No == oj_default_options.sym_key) ? Qfalse : Qnil));
    rb_hash_aset(opts, bigdecimal_as_decimal_sym, (Yes == oj_default_options.bigdec_as_num) ? Qtrue : ((No == oj_default_options.bigdec_as_num) ? Qfalse : Qnil));
    rb_hash_aset(opts, use_to_json_sym, (Yes == oj_default_options.to_json) ? Qtrue : ((No == oj_default_options.to_json) ? Qfalse : Qnil));
    rb_hash_aset(opts, nilnil_sym, (Yes == oj_default_options.nilnil) ? Qtrue : ((No == oj_default_options.nilnil) ? Qfalse : Qnil));
    rb_hash_aset(opts, allow_gc_sym, (Yes == oj_default_options.allow_gc) ? Qtrue : ((No == oj_default_options.allow_gc) ? Qfalse : Qnil));
    rb_hash_aset(opts, quirks_mode_sym, (Yes == oj_default_options.quirks_mode) ? Qtrue : ((No == oj_default_options.quirks_mode) ? Qfalse : Qnil));
    rb_hash_aset(opts, float_prec_sym, INT2FIX(oj_default_options.float_prec));
    switch (oj_default_options.mode) {
    case StrictMode:	rb_hash_aset(opts, mode_sym, strict_sym);	break;
    case CompatMode:	rb_hash_aset(opts, mode_sym, compat_sym);	break;
    case NullMode:	rb_hash_aset(opts, mode_sym, null_sym);		break;
    case ObjectMode:
    default:		rb_hash_aset(opts, mode_sym, object_sym);	break;
    }
    switch (oj_default_options.escape_mode) {
    case NLEsc:		rb_hash_aset(opts, escape_mode_sym, newline_sym);	break;
    case JSONEsc:	rb_hash_aset(opts, escape_mode_sym, json_sym);		break;
    case XSSEsc:	rb_hash_aset(opts, escape_mode_sym, xss_safe_sym);	break;
    case ASCIIEsc:	rb_hash_aset(opts, escape_mode_sym, ascii_sym);		break;
    default:		rb_hash_aset(opts, escape_mode_sym, json_sym);		break;
    }
    switch (oj_default_options.time_format) {
    case XmlTime:	rb_hash_aset(opts, time_format_sym, xmlschema_sym);	break;
    case RubyTime:	rb_hash_aset(opts, time_format_sym, ruby_sym);		break;
    case UnixZTime:	rb_hash_aset(opts, time_format_sym, unix_zone_sym);	break;
    case UnixTime:
    default:		rb_hash_aset(opts, time_format_sym, unix_sym);		break;
    }
    switch (oj_default_options.bigdec_load) {
    case BigDec:	rb_hash_aset(opts, bigdecimal_load_sym, bigdecimal_sym);break;
    case FloatDec:	rb_hash_aset(opts, bigdecimal_load_sym, float_sym);	break;
    case AutoDec:
    default:		rb_hash_aset(opts, bigdecimal_load_sym, auto_sym);	break;
    }
    rb_hash_aset(opts, create_id_sym, (0 == oj_default_options.create_id) ? Qnil : rb_str_new2(oj_default_options.create_id));
    rb_hash_aset(opts, space_sym, (0 == oj_default_options.dump_opts.after_size) ? Qnil : rb_str_new2(oj_default_options.dump_opts.after_sep));
    rb_hash_aset(opts, space_before_sym, (0 == oj_default_options.dump_opts.before_size) ? Qnil : rb_str_new2(oj_default_options.dump_opts.before_sep));
    rb_hash_aset(opts, object_nl_sym, (0 == oj_default_options.dump_opts.hash_size) ? Qnil : rb_str_new2(oj_default_options.dump_opts.hash_nl));
    rb_hash_aset(opts, array_nl_sym, (0 == oj_default_options.dump_opts.array_size) ? Qnil : rb_str_new2(oj_default_options.dump_opts.array_nl));

    switch (oj_default_options.dump_opts.nan_dump) {
    case NullNan:	rb_hash_aset(opts, nan_sym, null_sym);	break;
    case RaiseNan:	rb_hash_aset(opts, nan_sym, raise_sym);	break;
    case WordNan:	rb_hash_aset(opts, nan_sym, word_sym);	break;
    case HugeNan:	rb_hash_aset(opts, nan_sym, huge_sym);	break;
    case AutoNan:
    default:		rb_hash_aset(opts, nan_sym, auto_sym);	break;
    }
    return opts;
}

/* call-seq: default_options=(opts)
 *
 * Sets the default options for load and dump.
 * @param [Hash] opts options to change
 * @param [Fixnum|String|nil] :indent number of spaces to indent each element in a JSON document or the String to use for identation.
 * @param [true|false|nil] :circular support circular references while dumping
 * @param [true|false|nil] :auto_define automatically define classes if they do not exist
 * @param [true|false|nil] :symbol_keys convert hash keys to symbols
 * @param [true|false|nil] :class_cache cache classes for faster parsing
 * @param [:newline|:json|:xss_safe|:ascii|nil] :escape mode encodes all high-bit characters as
 *        escaped sequences if :ascii, :json is standand UTF-8 JSON encoding,
 *        :newline is the same as :json but newlines are not escaped,
 *        and :xss_safe escapes &, <, and >, and some others.
 * @param [true|false|nil] :bigdecimal_as_decimal dump BigDecimal as a decimal number or as a String
 * @param [:bigdecimal|:float|:auto|nil] :bigdecimal_load load decimals as BigDecimal instead of as a Float. :auto pick the most precise for the number of digits.
 * @param [:object|:strict|:compat|:null] load and dump mode to use for JSON
 *	  :strict raises an exception when a non-supported Object is
 *	  encountered. :compat attempts to extract variable values from an
 *	  Object using to_json() or to_hash() then it walks the Object's
 *	  variables if neither is found. The :object mode ignores to_hash()
 *	  and to_json() methods and encodes variables using code internal to
 *	  the Oj gem. The :null mode ignores non-supported Objects and
 *	  replaces them with a null.
 * @param [:unix|:xmlschema|:ruby] time format when dumping in :compat mode
 *        :unix decimal number denoting the number of seconds since 1/1/1970,
 *        :unix_zone decimal number denoting the number of seconds since 1/1/1970 plus the utc_offset in the exponent ,
 *        :xmlschema date-time format taken from XML Schema as a String,
 *        :ruby Time.to_s formatted String
 * @param [String|nil] :create_id create id for json compatible object encoding
 * @param [Fixnum|nil] :second_precision number of digits after the decimal when dumping the seconds portion of time
 * @param [Fixnum|nil] :float_precision number of digits of precision when dumping floats, 0 indicates use Ruby
 * @param [true|false|nil] :use_to_json call to_json() methods on dump, default is false
 * @param [true|false|nil] :nilnil if true a nil input to load will return nil and not raise an Exception
 * @param [true|false|nil] :allow_gc allow or prohibit GC during parsing, default is true (allow)
 * @param [true|false|nil] :quirks_mode allow single JSON values instead of documents, default is true (allow)
 * @param [String|nil] :space String to use for the space after the colon in JSON object fields
 * @param [String|nil] :space_before String to use before the colon separator in JSON object fields
 * @param [String|nil] :object_nl String to use after a JSON object field value
 * @param [String|nil] :array_nl String to use after a JSON array value
 * @param [:null|:huge|:word|:raise] :nan how to dump Infinity and NaN in null, strict, and compat mode. :null places a null, :huge places a huge number, :word places Infinity or NaN, :raise raises and exception, :auto uses default for each mode.
 * @return [nil]
 */
static VALUE
set_def_opts(VALUE self, VALUE opts) {
    Check_Type(opts, T_HASH);
    oj_parse_options(opts, &oj_default_options);

    return Qnil;
}

void
oj_parse_options(VALUE ropts, Options copts) {
    struct _YesNoOpt	ynos[] = {
	{ circular_sym, &copts->circular },
	{ auto_define_sym, &copts->auto_define },
	{ symbol_keys_sym, &copts->sym_key },
	{ class_cache_sym, &copts->class_cache },
	{ bigdecimal_as_decimal_sym, &copts->bigdec_as_num },
	{ use_to_json_sym, &copts->to_json },
	{ nilnil_sym, &copts->nilnil },
	{ allow_gc_sym, &copts->allow_gc },
	{ quirks_mode_sym, &copts->quirks_mode },
	{ Qnil, 0 }
    };
    YesNoOpt		o;
    volatile VALUE	v;
    size_t		len;
    
    if (T_HASH != rb_type(ropts)) {
	return;
    }
    if (Qtrue == rb_funcall(ropts, has_key_id, 1, indent_sym)) {
	v = rb_hash_lookup(ropts, indent_sym);
	switch (rb_type(v)) {
	case T_NIL:
	    copts->dump_opts.indent_size = 0;
	    *copts->dump_opts.indent_str = '\0';
	    copts->indent = 0;
	    break;
	case T_FIXNUM:
	    copts->dump_opts.indent_size = 0;
	    *copts->dump_opts.indent_str = '\0';
	    copts->indent = FIX2INT(v);
	    break;
	case T_STRING:
	    if (sizeof(copts->dump_opts.indent_str) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "indent string is limited to %lu characters.", sizeof(copts->dump_opts.indent_str));
	    }
	    strcpy(copts->dump_opts.indent_str, StringValuePtr(v));
	    copts->dump_opts.indent_size = (uint8_t)len;
	    copts->indent = 0;
	    break;
	default:
	    rb_raise(rb_eTypeError, "indent must be a Fixnum, String, or nil.");
	    break;
	}
    }
    if (Qnil != (v = rb_hash_lookup(ropts, float_prec_sym))) {
	int	n;

	if (rb_cFixnum != rb_obj_class(v)) {
	    rb_raise(rb_eArgError, ":float_precision must be a Fixnum.");
	}
	Check_Type(v, T_FIXNUM);
	n = FIX2INT(v);
	if (0 >= n) {
	    *copts->float_fmt = '\0';
	    copts->float_prec = 0;
	} else {
	    if (20 < n) {
		n = 20;
	    }
	    sprintf(copts->float_fmt, "%%0.%dg", n);
	    copts->float_prec = n;
	}
    }
    if (Qnil != (v = rb_hash_lookup(ropts, sec_prec_sym))) {
	int	n;

	if (rb_cFixnum != rb_obj_class(v)) {
	    rb_raise(rb_eArgError, ":second_precision must be a Fixnum.");
	}
	n = NUM2INT(v);
	if (0 > n) {
	    n = 0;
	} else if (9 < n) {
	    n = 9;
	}
	copts->sec_prec = n;
    }
    if (Qnil != (v = rb_hash_lookup(ropts, mode_sym))) {
	if (object_sym == v) {
	    copts->mode = ObjectMode;
	} else if (strict_sym == v) {
	    copts->mode = StrictMode;
	} else if (compat_sym == v) {
	    copts->mode = CompatMode;
	} else if (null_sym == v) {
	    copts->mode = NullMode;
	} else {
	    rb_raise(rb_eArgError, ":mode must be :object, :strict, :compat, or :null.");
	}
    }
    if (Qnil != (v = rb_hash_lookup(ropts, time_format_sym))) {
	if (unix_sym == v) {
	    copts->time_format = UnixTime;
	} else if (unix_zone_sym == v) {
	    copts->time_format = UnixZTime;
	} else if (xmlschema_sym == v) {
	    copts->time_format = XmlTime;
	} else if (ruby_sym == v) {
	    copts->time_format = RubyTime;
	} else {
	    rb_raise(rb_eArgError, ":time_format must be :unix, :unix_zone, :xmlschema, or :ruby.");
	}
    }
    if (Qnil != (v = rb_hash_lookup(ropts, escape_mode_sym))) {
	if (newline_sym == v) {
	    copts->escape_mode = NLEsc;
	} else if (json_sym == v) {
	    copts->escape_mode = JSONEsc;
	} else if (xss_safe_sym == v) {
	    copts->escape_mode = XSSEsc;
	} else if (ascii_sym == v) {
	    copts->escape_mode = ASCIIEsc;
	} else {
	    rb_raise(rb_eArgError, ":encoding must be :newline, :json, :xss_safe, or :ascii.");
	}
    }
    if (Qnil != (v = rb_hash_lookup(ropts, bigdecimal_load_sym))) {
	if (bigdecimal_sym == v || Qtrue == v) {
	    copts->bigdec_load = BigDec;
	} else if (float_sym == v) {
	    copts->bigdec_load = FloatDec;
	} else if (auto_sym == v || Qfalse == v) {
	    copts->bigdec_load = AutoDec;
	} else {
	    rb_raise(rb_eArgError, ":bigdecimal_load must be :bigdecimal, :float, or :auto.");
	}
    }
    if (Qtrue == rb_funcall(ropts, has_key_id, 1, create_id_sym)) {
	v = rb_hash_lookup(ropts, create_id_sym);
	if (Qnil == v) {
	    if (json_class != oj_default_options.create_id) {
		xfree((char*)oj_default_options.create_id);
	    }
	    copts->create_id = NULL;
	    copts->create_id_len = 0;
	} else if (T_STRING == rb_type(v)) {
	    const char	*str = StringValuePtr(v);

	    len = RSTRING_LEN(v);
	    if (len != copts->create_id_len ||
		0 != strcmp(copts->create_id, str)) {
		copts->create_id = ALLOC_N(char, len + 1);
		strcpy((char*)copts->create_id, str);
		copts->create_id_len = len;
	    }
	} else {
	    rb_raise(rb_eArgError, ":create_id must be string.");
	}
    }
    for (o = ynos; 0 != o->attr; o++) {
	if (Qnil != (v = rb_hash_lookup(ropts, o->sym))) {
	    if (Qtrue == v) {
		*o->attr = Yes;
	    } else if (Qfalse == v) {
		*o->attr = No;
	    } else {
		rb_raise(rb_eArgError, "%s must be true or false.", rb_id2name(SYM2ID(o->sym)));
	    }
	}
    }
    if (Qtrue == rb_funcall(ropts, has_key_id, 1, space_sym)) {
	if (Qnil == (v = rb_hash_lookup(ropts, space_sym))) {
	    copts->dump_opts.after_size = 0;
	    *copts->dump_opts.after_sep = '\0';
	} else {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.after_sep) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "space string is limited to %lu characters.", sizeof(copts->dump_opts.after_sep));
	    }
	    strcpy(copts->dump_opts.after_sep, StringValuePtr(v));
	    copts->dump_opts.after_size = (uint8_t)len;
	}
    }
    if (Qtrue == rb_funcall(ropts, has_key_id, 1, space_before_sym)) {
	if (Qnil == (v = rb_hash_lookup(ropts, space_before_sym))) {
	    copts->dump_opts.before_size = 0;
	    *copts->dump_opts.before_sep = '\0';
	} else {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.before_sep) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "sapce_before string is limited to %lu characters.", sizeof(copts->dump_opts.before_sep));
	    }
	    strcpy(copts->dump_opts.before_sep, StringValuePtr(v));
	    copts->dump_opts.before_size = (uint8_t)len;
	}
    }
    if (Qtrue == rb_funcall(ropts, has_key_id, 1, object_nl_sym)) {
	if (Qnil == (v = rb_hash_lookup(ropts, object_nl_sym))) {
	    copts->dump_opts.hash_size = 0;
	    *copts->dump_opts.hash_nl = '\0';
	} else {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.hash_nl) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "object_nl string is limited to %lu characters.", sizeof(copts->dump_opts.hash_nl));
	    }
	    strcpy(copts->dump_opts.hash_nl, StringValuePtr(v));
	    copts->dump_opts.hash_size = (uint8_t)len;
	}
    }
    if (Qtrue == rb_funcall(ropts, has_key_id, 1, array_nl_sym)) {
	if (Qnil == (v = rb_hash_lookup(ropts, array_nl_sym))) {
	    copts->dump_opts.array_size = 0;
	    *copts->dump_opts.array_nl = '\0';
	} else {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.array_nl) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "array_nl string is limited to %lu characters.", sizeof(copts->dump_opts.array_nl));
	    }
	    strcpy(copts->dump_opts.array_nl, StringValuePtr(v));
	    copts->dump_opts.array_size = (uint8_t)len;
	}
    }
    if (Qnil != (v = rb_hash_lookup(ropts, nan_sym))) {
	if (null_sym == v) {
	    copts->dump_opts.nan_dump = NullNan;
	} else if (huge_sym == v) {
	    copts->dump_opts.nan_dump = HugeNan;
	} else if (word_sym == v) {
	    copts->dump_opts.nan_dump = WordNan;
	} else if (raise_sym == v) {
	    copts->dump_opts.nan_dump = RaiseNan;
	} else if (auto_sym == v) {
	    copts->dump_opts.nan_dump = AutoNan;
	} else {
	    rb_raise(rb_eArgError, ":nan must be :null, :huge, :word, :raise, or :auto.");
	}
    }
    copts->dump_opts.use = (0 < copts->dump_opts.indent_size ||
			    0 < copts->dump_opts.after_size ||
			    0 < copts->dump_opts.before_size ||
			    0 < copts->dump_opts.hash_size ||
			    0 < copts->dump_opts.array_size);
    // This is here only for backwards compatibility with the original Oj.
    v = rb_hash_lookup(ropts, ascii_only_sym);
    if (Qtrue == v) {
	copts->escape_mode = ASCIIEsc;
    } else if (Qfalse == v) {
	copts->escape_mode = JSONEsc;
    }
}

/* Document-method: strict_load
 *	call-seq: strict_load(json, options) => Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into an Hash, Array, String, Fixnum, Float,
 * true, false, or nil. It parses using a mode that is strict in that it maps
 * each primitive JSON type to a similar Ruby type. The :create_id is not
 * honored in this mode. Note that a Ruby Hash is used to represent the JSON
 * Object type. These two are not the same since the JSON Object type can have
 * repeating entries with the same key and Ruby Hash can not.
 *
 * When used with a document that has multiple JSON elements the block, if
 * any, will be yielded to. If no block then the last element read will be
 * returned.
 *
 * Raises an exception if the JSON is malformed or the classes specified are not
 * valid. If the input is not a valid JSON document (an empty string is not a
 * valid JSON document) an exception is raised.
 *
 * A block can also be provided with a single argument. That argument will be
 * the parsed JSON document. This is useful when parsing a string that includes
 * multiple JSON documents.
 *
 * @param [String|IO] json JSON String or an Object that responds to read()
 * @param [Hash] options load options (same as default_options)
 */

/* Document-method: compat_load
 *	call-seq: compat_load(json, options) => Object, Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into an Object, Hash, Array, String, Fixnum,
 * Float, true, false, or nil. It parses using a mode that is generally
 * compatible with other Ruby JSON parsers in that it will create objects based
 * on the :create_id value. It is not compatible in every way to every other
 * parser though as each parser has it's own variations.
 *
 * When used with a document that has multiple JSON elements the block, if
 * any, will be yielded to. If no block then the last element read will be
 * returned.
 *
 * Raises an exception if the JSON is malformed or the classes specified are not
 * valid. If the input is not a valid JSON document (an empty string is not a
 * valid JSON document) an exception is raised.
 *
 * A block can also be provided with a single argument. That argument will be
 * the parsed JSON document. This is useful when parsing a string that includes
 * multiple JSON documents.
 *
 * @param [String|IO] json JSON String or an Object that responds to read()
 * @param [Hash] options load options (same as default_options)
 */

/* Document-method: object_load
 *	call-seq: object_load(json, options) => Object, Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into an Object, Hash, Array, String, Fixnum,
 * Float, true, false, or nil. In the :object mode the JSON should have been
 * generated by Oj.dump(). The parser will reconstitute the original marshalled
 * or dumped Object. The :auto_define and :circular options have meaning with
 * this parsing mode.
 *
 * When used with a document that has multiple JSON elements the block, if
 * any, will be yielded to. If no block then the last element read will be
 * returned.
 *
 * Raises an exception if the JSON is malformed or the classes specified are not
 * valid. If the input is not a valid JSON document (an empty string is not a
 * valid JSON document) an exception is raised.
 *
 * Note: Oj is not able to automatically deserialize all classes that are a
 * subclass of a Ruby Exception. Only exception that take one required string
 * argument in the initialize() method are supported. This is an example of how
 * to write an Exception subclass that supports both a single string intializer
 * and an Exception as an argument. Additional optional arguments can be added
 * as well.
 *
 * The reason for this restriction has to do with a design decision on the part
 * of the Ruby developers. Exceptions are special Objects. They do not follow the
 * rules of other Objects. Exceptions have 'mesg' and a 'bt' attribute. Note that
 * these are not '@mesg' and '@bt'. They can not be set using the normal C or
 * Ruby calls. The only way I have found to set the 'mesg' attribute is through
 * the initializer. Unfortunately that means any subclass that provides a
 * different initializer can not be automatically decoded. A way around this is
 * to use a create function but this example shows an alternative.
 *
 * A block can also be provided with a single argument. That argument will be
 * the parsed JSON document. This is useful when parsing a string that includes
 * multiple JSON documents.
 *
 * @param [String|IO] json JSON String or an Object that responds to read()
 * @param [Hash] options load options (same as default_options)
 */

/* call-seq: load(json, options) => Object, Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into a Object, Hash, Array, String, Fixnum,
 * Float, true, false, or nil according to the default mode or the mode
 * specified. Raises an exception if the JSON is malformed or the classes
 * specified are not valid. If the string input is not a valid JSON document (an
 * empty string is not a valid JSON document) an exception is raised.
 *
 * When used with a document that has multiple JSON elements the block, if
 * any, will be yielded to. If no block then the last element read will be
 * returned.
 *
 * This parser operates on string and will attempt to load files into memory if
 * a file object is passed as the first argument. A stream input will be parsed
 * using a stream parser but others use the slightly faster string parser.
 *
 * A block can also be provided with a single argument. That argument will be
 * the parsed JSON document. This is useful when parsing a string that includes
 * multiple JSON documents.
 *
 * @param [String|IO] json JSON String or an Object that responds to read()
 * @param [Hash] options load options (same as default_options)
 */
static VALUE
load(int argc, VALUE *argv, VALUE self) {
    Mode	mode = oj_default_options.mode;

    if (1 > argc) {
	rb_raise(rb_eArgError, "Wrong number of arguments to load().");
    }
    if (2 <= argc) {
	VALUE	ropts = argv[1];
	VALUE	v;

	Check_Type(ropts, T_HASH);
	if (Qnil != (v = rb_hash_lookup(ropts, mode_sym))) {
	    if (object_sym == v) {
		mode = ObjectMode;
	    } else if (strict_sym == v) {
		mode = StrictMode;
	    } else if (compat_sym == v) {
		mode = CompatMode;
	    } else if (null_sym == v) {
		mode = NullMode;
	    } else {
		rb_raise(rb_eArgError, ":mode must be :object, :strict, :compat, or :null.");
	    }
	}
    }
    switch (mode) {
    case StrictMode:
	return oj_strict_parse(argc, argv, self);
    case NullMode:
    case CompatMode:
	return oj_compat_parse(argc, argv, self);
    case ObjectMode:
    default:
	break;
    }
    return oj_object_parse(argc, argv, self);
}

/* Document-method: load_file
 *   call-seq: load_file(path, options) => Object, Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into a Object, Hash, Array, String, Fixnum,
 * Float, true, false, or nil according to the default mode or the mode
 * specified. Raises an exception if the JSON is malformed or the classes
 * specified are not valid. If the string input is not a valid JSON document (an
 * empty string is not a valid JSON document) an exception is raised.
 *
 * When used with a document that has multiple JSON elements the block, if
 * any, will be yielded to. If no block then the last element read will be
 * returned.
 *
 * If the input file is not a valid JSON document (an empty file is not a valid
 * JSON document) an exception is raised.
 *
 * This is a stream based parser which allows a large or huge file to be loaded
 * without pulling the whole file into memory.
 *
 * A block can also be provided with a single argument. That argument will be
 * the parsed JSON document. This is useful when parsing a string that includes
 * multiple JSON documents.
 *
 * @param [String] path path to a file containing a JSON document
 * @param [Hash] options load options (same as default_options)
 */
static VALUE
load_file(int argc, VALUE *argv, VALUE self) {
    char		*path;
    int			fd;
    Mode		mode = oj_default_options.mode;
    struct _ParseInfo	pi;

    if (1 > argc) {
	rb_raise(rb_eArgError, "Wrong number of arguments to load().");
    }
    Check_Type(*argv, T_STRING);
    pi.options = oj_default_options;
    pi.handler = Qnil;
    pi.err_class = Qnil;
    if (2 <= argc) {
	VALUE	ropts = argv[1];
	VALUE	v;

	Check_Type(ropts, T_HASH);
	if (Qnil != (v = rb_hash_lookup(ropts, mode_sym))) {
	    if (object_sym == v) {
		mode = ObjectMode;
	    } else if (strict_sym == v) {
		mode = StrictMode;
	    } else if (compat_sym == v) {
		mode = CompatMode;
	    } else if (null_sym == v) {
		mode = NullMode;
	    } else {
		rb_raise(rb_eArgError, ":mode must be :object, :strict, :compat, or :null.");
	    }
	}
    }
    path = StringValuePtr(*argv);
    if (0 == (fd = open(path, O_RDONLY))) {
	rb_raise(rb_eIOError, "%s", strerror(errno));
    }
    switch (mode) {
    case StrictMode:
	oj_set_strict_callbacks(&pi);
	return oj_pi_sparse(argc, argv, &pi, fd);
    case NullMode:
    case CompatMode:
	oj_set_compat_callbacks(&pi);
	return oj_pi_sparse(argc, argv, &pi, fd);
    case ObjectMode:
    default:
	break;
    }
    oj_set_object_callbacks(&pi);

    return oj_pi_sparse(argc, argv, &pi, fd);
}

/* call-seq: safe_load(doc)
 *
 * Loads a JSON document in strict mode with :auto_define and :symbol_keys
 * turned off. This function should be safe to use with JSON received on an
 * unprotected public interface.
 *
 * @param [String|IO] doc JSON String or IO to load
 * @return [Hash|Array|String|Fixnum|Bignum|BigDecimal|nil|True|False]
 */
static VALUE
safe_load(VALUE self, VALUE doc) {
    struct _ParseInfo	pi;
    VALUE		args[1];

    pi.err_class = Qnil;
    pi.options = oj_default_options;
    pi.options.auto_define = No;
    pi.options.sym_key = No;
    pi.options.mode = StrictMode;
    oj_set_strict_callbacks(&pi);
    *args = doc;

    return oj_pi_parse(1, args, &pi, 0, 0, 1);
}

/* call-seq: saj_parse(handler, io)
 *
 * Parses an IO stream or file containing a JSON document. Raises an exception
 * if the JSON is malformed. This is a callback parser that calls the methods in
 * the handler if they exist. A sample is the Oj::Saj class which can be used as
 * a base class for the handler.
 *
 * @param [Oj::Saj] handler responds to Oj::Saj methods
 * @param [IO|String] io IO Object to read from
 */

/* call-seq: sc_parse(handler, io)
 *
 * Parses an IO stream or file containing a JSON document. Raises an exception
 * if the JSON is malformed. This is a callback parser (Simple Callback Parser)
 * that calls the methods in the handler if they exist. A sample is the
 * Oj::ScHandler class which can be used as a base class for the handler. This
 * callback parser is slightly more efficient than the Saj callback parser and
 * requires less argument checking.
 *
 * @param [Oj::ScHandler] handler responds to Oj::ScHandler methods
 * @param [IO|String] io IO Object to read from
 */

/* call-seq: dump(obj, options) => json-string
 *
 * Dumps an Object (obj) to a string.
 * @param [Object] obj Object to serialize as an JSON document String
 * @param [Hash] options same as default_options
 */
static VALUE
dump(int argc, VALUE *argv, VALUE self) {
    char		buf[4096];
    struct _Out		out;
    struct _Options	copts = oj_default_options;
    VALUE		rstr;
    
    if (2 == argc) {
	oj_parse_options(argv[1], &copts);
    }
    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    oj_dump_obj_to_json(*argv, &copts, &out);
    if (0 == out.buf) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(out.buf);
    rstr = oj_encode(rstr);
    if (out.allocated) {
	xfree(out.buf);
    }
    return rstr;
}


/* call-seq: to_file(file_path, obj, options)
 *
 * Dumps an Object to the specified file.
 * @param [String] file_path file path to write the JSON document to
 * @param [Object] obj Object to serialize as an JSON document String
 * @param [Hash] options formating options
 * @param [Fixnum] :indent format expected
 * @param [true|false] :circular allow circular references, default: false
 */
static VALUE
to_file(int argc, VALUE *argv, VALUE self) {
    struct _Options	copts = oj_default_options;
    
    if (3 == argc) {
	oj_parse_options(argv[2], &copts);
    }
    Check_Type(*argv, T_STRING);
    oj_write_obj_to_file(argv[1], StringValuePtr(*argv), &copts);

    return Qnil;
}

/* call-seq: to_stream(io, obj, options)
 *
 * Dumps an Object to the specified IO stream.
 * @param [IO] io IO stream to write the JSON document to
 * @param [Object] obj Object to serialize as an JSON document String
 * @param [Hash] options formating options
 * @param [Fixnum] :indent format expected
 * @param [true|false] :circular allow circular references, default: false
 */
static VALUE
to_stream(int argc, VALUE *argv, VALUE self) {
    struct _Options	copts = oj_default_options;
    
    if (3 == argc) {
	oj_parse_options(argv[2], &copts);
    }
    oj_write_obj_to_stream(argv[1], *argv, &copts);

    return Qnil;
}

/* call-seq: register_odd(clas, create_object, create_method, *members)
 *
 * Registers a class as special. This is useful for working around subclasses of
 * primitive types as is done with ActiveSupport classes. The use of this
 * function should be limited to just classes that can not be handled in the
 * normal way. It is not intended as a hook for changing the output of all
 * classes as it is not optimized for large numbers of classes.
 *
 * @param [Class|Module] clas Class or Module to be made special
 * @param [Object] create_object object to call the create method on
 * @param [Symbol] create_method method on the clas that will create a new
 *                 instance of the clas when given all the member values in the
 *                 order specified.
 * @param [Symbol|String] members methods used to get the member values from
 *                        instances of the clas
 */
static VALUE
register_odd(int argc, VALUE *argv, VALUE self) {
    if (3 > argc) {
	rb_raise(rb_eArgError, "incorrect number of arguments.");
    }
    switch (rb_type(*argv)) {
    case T_CLASS:
    case T_MODULE:
	break;
    default:
	rb_raise(rb_eTypeError, "expected a class or module.");
	break;
    }
    Check_Type(argv[2], T_SYMBOL);
    if (MAX_ODD_ARGS < argc - 2) {
	rb_raise(rb_eArgError, "too many members.");
    }
    oj_reg_odd(argv[0], argv[1], argv[2], argc - 3, argv + 3, false);

    return Qnil;
}

/* call-seq: register_odd_raw(clas, create_object, create_method, dump_method)
 *
 * Registers a class as special and expect the output to be a string that can be
 * included in the dumped JSON directly. This is useful for working around
 * subclasses of primitive types as is done with ActiveSupport classes. The use
 * of this function should be limited to just classes that can not be handled in
 * the normal way. It is not intended as a hook for changing the output of all
 * classes as it is not optimized for large numbers of classes. Be careful with
 * this option as the JSON may be incorrect if invalid JSON is returned.
 *
 * @param [Class|Module] clas Class or Module to be made special
 * @param [Object] create_object object to call the create method on
 * @param [Symbol] create_method method on the clas that will create a new
 *                 instance of the clas when given all the member values in the
 *                 order specified.
 * @param [Symbol|String] dump_method method to call on the object being
 *                        serialized to generate the raw JSON.
 */
static VALUE
register_odd_raw(int argc, VALUE *argv, VALUE self) {
    if (3 > argc) {
	rb_raise(rb_eArgError, "incorrect number of arguments.");
    }
    switch (rb_type(*argv)) {
    case T_CLASS:
    case T_MODULE:
	break;
    default:
	rb_raise(rb_eTypeError, "expected a class or module.");
	break;
    }
    Check_Type(argv[2], T_SYMBOL);
    if (MAX_ODD_ARGS < argc - 2) {
	rb_raise(rb_eArgError, "too many members.");
    }
    oj_reg_odd(argv[0], argv[1], argv[2], 1, argv + 3, true);

    return Qnil;
}

static void
str_writer_free(void *ptr) {
    StrWriter	sw;

    if (0 == ptr) {
	return;
    }
    sw = (StrWriter)ptr;
    xfree(sw->out.buf);
    xfree(sw->types);
    xfree(ptr);
}

/* Document-class: Oj::StringWriter
 * 
 * Supports building a JSON document one element at a time. Build the document
 * by pushing values into the document. Pushing an array or an object will
 * create that element in the JSON document and subsequent pushes will add the
 * elements to that array or object until a pop() is called. When complete
 * calling to_s() will return the JSON document. Note tha calling to_s() before
 * construction is complete will return the document in it's current state.
 */

static void
str_writer_init(StrWriter sw) {
    sw->opts = oj_default_options;
    sw->depth = 0;
    sw->types = ALLOC_N(char, 256);
    sw->types_end = sw->types + 256;
    *sw->types = '\0';
    sw->keyWritten = 0;

    sw->out.buf = ALLOC_N(char, 4096);
    sw->out.end = sw->out.buf + 4086;
    sw->out.allocated = 1;
    sw->out.cur = sw->out.buf;
    *sw->out.cur = '\0';
    sw->out.circ_cnt = 0;
    sw->out.hash_cnt = 0;
    sw->out.opts = &sw->opts;
    sw->out.indent = sw->opts.indent;
    sw->out.depth = 0;
}

/* call-seq: new(options)
 *
 * Creates a new StringWriter.
 * @param [Hash] options formating options
 */
static VALUE
str_writer_new(int argc, VALUE *argv, VALUE self) {
    StrWriter	sw = ALLOC(struct _StrWriter);
    
    str_writer_init(sw);
    if (1 == argc) {
	oj_parse_options(argv[0], &sw->opts);
    }
    sw->out.indent = sw->opts.indent;

    return Data_Wrap_Struct(oj_string_writer_class, 0, str_writer_free, sw);
}

/* call-seq: push_key(key)
 *
 * Pushes a key onto the JSON document. The key will be used for the next push
 * if currently in a JSON object and ignored otherwise. If a key is provided on
 * the next push then that new key will be ignored.
 * @param [String] key the key pending for the next push
 */
static VALUE
str_writer_push_key(VALUE self, VALUE key) {
    StrWriter	sw = (StrWriter)DATA_PTR(self);

    rb_check_type(key, T_STRING);
    oj_str_writer_push_key(sw, StringValuePtr(key));

    return Qnil;
}

/* call-seq: push_object(key=nil)
 *
 * Pushes an object onto the JSON document. Future pushes will be to this object
 * until a pop() is called.
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
str_writer_push_object(int argc, VALUE *argv, VALUE self) {
    StrWriter	sw = (StrWriter)DATA_PTR(self);

    switch (argc) {
    case 0:
	oj_str_writer_push_object(sw, 0);
	break;
    case 1:
	if (Qnil == argv[0]) {
	    oj_str_writer_push_object(sw, 0);
	} else {
	    rb_check_type(argv[0], T_STRING);
	    oj_str_writer_push_object(sw, StringValuePtr(argv[0]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_object'.");
	break;
    }
    if (rb_block_given_p()) {
	rb_yield(Qnil);
	oj_str_writer_pop(sw);
    }
    return Qnil;
}

/* call-seq: push_array(key=nil)
 *
 * Pushes an array onto the JSON document. Future pushes will be to this object
 * until a pop() is called.
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
str_writer_push_array(int argc, VALUE *argv, VALUE self) {
    StrWriter	sw = (StrWriter)DATA_PTR(self);

    switch (argc) {
    case 0:
	oj_str_writer_push_array(sw, 0);
	break;
    case 1:
	if (Qnil == argv[0]) {
	    oj_str_writer_push_array(sw, 0);
	} else {
	    rb_check_type(argv[0], T_STRING);
	    oj_str_writer_push_array(sw, StringValuePtr(argv[0]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_object'.");
	break;
    }
    if (rb_block_given_p()) {
	rb_yield(Qnil);
	oj_str_writer_pop(sw);
    }
    return Qnil;
}

/* call-seq: push_value(value, key=nil)
 *
 * Pushes a value onto the JSON document.
 * @param [Object] value value to add to the JSON document
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
str_writer_push_value(int argc, VALUE *argv, VALUE self) {
    switch (argc) {
    case 1:
	oj_str_writer_push_value((StrWriter)DATA_PTR(self), *argv, 0);
	break;
    case 2:
	if (Qnil == argv[1]) {
	    oj_str_writer_push_value((StrWriter)DATA_PTR(self), *argv, 0);
	} else {
	    rb_check_type(argv[1], T_STRING);
	    oj_str_writer_push_value((StrWriter)DATA_PTR(self), *argv, StringValuePtr(argv[1]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_value'.");
	break;
    }
    return Qnil;
}

/* call-seq: push_json(value, key=nil)
 *
 * Pushes a string onto the JSON document. The String must be a valid JSON
 * encoded string. No additional checking is done to verify the validity of the
 * string.
 * @param [String] value JSON document to add to the JSON document
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
str_writer_push_json(int argc, VALUE *argv, VALUE self) {
    rb_check_type(argv[0], T_STRING);
    switch (argc) {
    case 1:
	oj_str_writer_push_json((StrWriter)DATA_PTR(self), StringValuePtr(*argv), 0);
	break;
    case 2:
	if (Qnil == argv[1]) {
	    oj_str_writer_push_json((StrWriter)DATA_PTR(self), StringValuePtr(*argv), 0);
	} else {
	    rb_check_type(argv[1], T_STRING);
	    oj_str_writer_push_json((StrWriter)DATA_PTR(self), StringValuePtr(*argv), StringValuePtr(argv[1]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_json'.");
	break;
    }
    return Qnil;
}

/* call-seq: pop()
 *
 * Pops up a level in the JSON document closing the array or object that is
 * currently open.
 */
static VALUE
str_writer_pop(VALUE self) {
    oj_str_writer_pop((StrWriter)DATA_PTR(self));
    return Qnil;
}

/* call-seq: pop_all()
 *
 * Pops all level in the JSON document closing all the array or object that is
 * currently open.
 */
static VALUE
str_writer_pop_all(VALUE self) {
    oj_str_writer_pop_all((StrWriter)DATA_PTR(self));

    return Qnil;
}

/* call-seq: reset()
 *
 * Reset the writer back to the empty state.
 */
static VALUE
str_writer_reset(VALUE self) {
    StrWriter	sw = (StrWriter)DATA_PTR(self);

    sw->depth = 0;
    *sw->types = '\0';
    sw->keyWritten = 0;
    sw->out.cur = sw->out.buf;
    *sw->out.cur = '\0';

    return Qnil;
}

/* call-seq: to_s()
 *
 * Returns the JSON document string in what ever state the construction is at.
 */
static VALUE
str_writer_to_s(VALUE self) {
    StrWriter	sw = (StrWriter)DATA_PTR(self);
    VALUE	rstr = rb_str_new(sw->out.buf, sw->out.cur - sw->out.buf);

    return oj_encode(rstr);
}

// StreamWriter

/* Document-class: Oj::StreamWriter
 * 
 * Supports building a JSON document one element at a time. Build the IO stream
 * document by pushing values into the document. Pushing an array or an object
 * will create that element in the JSON document and subsequent pushes will add
 * the elements to that array or object until a pop() is called.
 */

static void
stream_writer_free(void *ptr) {
    StreamWriter	sw;

    if (0 == ptr) {
	return;
    }
    sw = (StreamWriter)ptr;
    xfree(sw->sw.out.buf);
    xfree(sw->sw.types);
    xfree(ptr);
}

static void
stream_writer_write(StreamWriter sw) {
    ssize_t	size = sw->sw.out.cur - sw->sw.out.buf;

    switch (sw->type) {
    case STRING_IO:
	rb_funcall(sw->stream, oj_write_id, 1, rb_str_new(sw->sw.out.buf, size));
	break;
    case STREAM_IO:
	rb_funcall(sw->stream, oj_write_id, 1, rb_str_new(sw->sw.out.buf, size));
	break;
    case FILE_IO:
	if (size != write(sw->fd, sw->sw.out.buf, size)) {
	    rb_raise(rb_eIOError, "Write failed. [%d:%s]\n", errno, strerror(errno));
	}
	break;
    default:
	rb_raise(rb_eArgError, "expected an IO Object.");
    }
}

static void
stream_writer_reset_buf(StreamWriter sw) {
    sw->sw.out.cur = sw->sw.out.buf;
    *sw->sw.out.cur = '\0';
}

/* call-seq: new(io, options)
 *
 * Creates a new StreamWriter.
 * @param [IO] io stream to write to
 * @param [Hash] options formating options
 */
static VALUE
stream_writer_new(int argc, VALUE *argv, VALUE self) {
    StreamWriterType	type = STREAM_IO;
    int			fd = 0;
    VALUE		stream = argv[0];
    VALUE		clas = rb_obj_class(stream);
    StreamWriter	sw;
#if !IS_WINDOWS
    VALUE		s;
#endif
    
    if (oj_stringio_class == clas) {
	type = STRING_IO;
#if !IS_WINDOWS
    } else if (rb_respond_to(stream, oj_fileno_id) &&
	       Qnil != (s = rb_funcall(stream, oj_fileno_id, 0)) &&
	       0 != (fd = FIX2INT(s))) {
	type = FILE_IO;
#endif
    } else if (rb_respond_to(stream, oj_write_id)) {
	type = STREAM_IO;
    } else {
	rb_raise(rb_eArgError, "expected an IO Object.");
    }
    sw = ALLOC(struct _StreamWriter);
    str_writer_init(&sw->sw);
    if (2 == argc) {
	oj_parse_options(argv[1], &sw->sw.opts);
    }
    sw->sw.out.indent = sw->sw.opts.indent;
    sw->stream = stream;
    sw->type = type;
    sw->fd = fd;

    return Data_Wrap_Struct(oj_stream_writer_class, 0, stream_writer_free, sw);
}

/* call-seq: push_key(key)
 *
 * Pushes a key onto the JSON document. The key will be used for the next push
 * if currently in a JSON object and ignored otherwise. If a key is provided on
 * the next push then that new key will be ignored.
 * @param [String] key the key pending for the next push
 */
static VALUE
stream_writer_push_key(VALUE self, VALUE key) {
    StreamWriter	sw = (StreamWriter)DATA_PTR(self);

    rb_check_type(key, T_STRING);
    stream_writer_reset_buf(sw);
    oj_str_writer_push_key(&sw->sw, StringValuePtr(key));
    stream_writer_write(sw);
    return Qnil;
}

/* call-seq: push_object(key=nil)
 *
 * Pushes an object onto the JSON document. Future pushes will be to this object
 * until a pop() is called.
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
stream_writer_push_object(int argc, VALUE *argv, VALUE self) {
    StreamWriter	sw = (StreamWriter)DATA_PTR(self);

    stream_writer_reset_buf(sw);
    switch (argc) {
    case 0:
	oj_str_writer_push_object(&sw->sw, 0);
	break;
    case 1:
	if (Qnil == argv[0]) {
	    oj_str_writer_push_object(&sw->sw, 0);
	} else {
	    rb_check_type(argv[0], T_STRING);
	    oj_str_writer_push_object(&sw->sw, StringValuePtr(argv[0]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_object'.");
	break;
    }
    stream_writer_write(sw);
    return Qnil;
}

/* call-seq: push_array(key=nil)
 *
 * Pushes an array onto the JSON document. Future pushes will be to this object
 * until a pop() is called.
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
stream_writer_push_array(int argc, VALUE *argv, VALUE self) {
    StreamWriter	sw = (StreamWriter)DATA_PTR(self);

    stream_writer_reset_buf(sw);
    switch (argc) {
    case 0:
	oj_str_writer_push_array(&sw->sw, 0);
	break;
    case 1:
	if (Qnil == argv[0]) {
	    oj_str_writer_push_array(&sw->sw, 0);
	} else {
	    rb_check_type(argv[0], T_STRING);
	    oj_str_writer_push_array(&sw->sw, StringValuePtr(argv[0]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_object'.");
	break;
    }
    stream_writer_write(sw);
    return Qnil;
}

/* call-seq: push_value(value, key=nil)
 *
 * Pushes a value onto the JSON document.
 * @param [Object] value value to add to the JSON document
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
stream_writer_push_value(int argc, VALUE *argv, VALUE self) {
    StreamWriter	sw = (StreamWriter)DATA_PTR(self);

    stream_writer_reset_buf(sw);
    switch (argc) {
    case 1:
	oj_str_writer_push_value((StrWriter)DATA_PTR(self), *argv, 0);
	break;
    case 2:
	if (Qnil == argv[0]) {
	    oj_str_writer_push_value((StrWriter)DATA_PTR(self), *argv, 0);
	} else {
	    rb_check_type(argv[1], T_STRING);
	    oj_str_writer_push_value((StrWriter)DATA_PTR(self), *argv, StringValuePtr(argv[1]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_value'.");
	break;
    }
    stream_writer_write(sw);
    return Qnil;
}

/* call-seq: push_json(value, key=nil)
 *
 * Pushes a string onto the JSON document. The String must be a valid JSON
 * encoded string. No additional checking is done to verify the validity of the
 * string.
 * @param [Object] value value to add to the JSON document
 * @param [String] key the key if adding to an object in the JSON document
 */
static VALUE
stream_writer_push_json(int argc, VALUE *argv, VALUE self) {
    StreamWriter	sw = (StreamWriter)DATA_PTR(self);

    rb_check_type(argv[0], T_STRING);
    stream_writer_reset_buf(sw);
    switch (argc) {
    case 1:
	oj_str_writer_push_json((StrWriter)DATA_PTR(self), StringValuePtr(*argv), 0);
	break;
    case 2:
	if (Qnil == argv[0]) {
	    oj_str_writer_push_json((StrWriter)DATA_PTR(self), StringValuePtr(*argv), 0);
	} else {
	    rb_check_type(argv[1], T_STRING);
	    oj_str_writer_push_json((StrWriter)DATA_PTR(self), StringValuePtr(*argv), StringValuePtr(argv[1]));
	}
	break;
    default:
	rb_raise(rb_eArgError, "Wrong number of argument to 'push_json'.");
	break;
    }
    stream_writer_write(sw);
    return Qnil;
}

/* call-seq: pop()
 *
 * Pops up a level in the JSON document closing the array or object that is
 * currently open.
 */
static VALUE
stream_writer_pop(VALUE self) {
    StreamWriter	sw = (StreamWriter)DATA_PTR(self);

    stream_writer_reset_buf(sw);
    oj_str_writer_pop(&sw->sw);
    stream_writer_write(sw);
    return Qnil;
}

/* call-seq: pop_all()
 *
 * Pops all level in the JSON document closing all the array or object that is
 * currently open.
 */
static VALUE
stream_writer_pop_all(VALUE self) {
    StreamWriter	sw = (StreamWriter)DATA_PTR(self);

    stream_writer_reset_buf(sw);
    oj_str_writer_pop_all(&sw->sw);
    stream_writer_write(sw);

    return Qnil;
}

// Mimic JSON section

static VALUE
mimic_dump(int argc, VALUE *argv, VALUE self) {
    char		buf[4096];
    struct _Out		out;
    struct _Options	copts = oj_default_options;
    VALUE		rstr;
    
    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    oj_dump_obj_to_json(*argv, &copts, &out);
    if (0 == out.buf) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(out.buf);
    rstr = oj_encode(rstr);
    if (2 <= argc && Qnil != argv[1]) {
	VALUE	io = argv[1];
	VALUE	args[1];

	*args = rstr;
	rb_funcall2(io, oj_write_id, 1, args);
	rstr = io;
    }
    if (out.allocated) {
	xfree(out.buf);
    }
    return rstr;
}

// This is the signature for the hash_foreach callback also.
static int
mimic_walk(VALUE key, VALUE obj, VALUE proc) {
    switch (rb_type(obj)) {
    case T_HASH:
	rb_hash_foreach(obj, mimic_walk, proc);
	break;
    case T_ARRAY:
	{
	    size_t	cnt = RARRAY_LEN(obj);
	    size_t	i;

	    for (i = 0; i < cnt; i++) {
		mimic_walk(Qnil, rb_ary_entry(obj, i), proc);
	    }
	    break;
	}
    default:
	break;
    }
    if (Qnil == proc) {
	if (rb_block_given_p()) {
	    rb_yield(obj);
	}
    } else {
#if HAS_PROC_WITH_BLOCK
	VALUE	args[1];

	*args = obj;
	rb_proc_call_with_block(proc, 1, args, Qnil);
#else
	rb_raise(rb_eNotImpError, "Calling a Proc with a block not supported in this version. Use func() {|x| } syntax instead.");
#endif
    }
    return ST_CONTINUE;
}

static VALUE
mimic_load(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;
    VALUE		obj;
    VALUE		p = Qnil;

    pi.err_class = json_parser_error_class;
    pi.options = oj_default_options;
    oj_set_compat_callbacks(&pi);

    obj = oj_pi_parse(argc, argv, &pi, 0, 0, 0);
    if (2 <= argc) {
	p = argv[1];
    }
    mimic_walk(Qnil, obj, p);

    return obj;
}

static VALUE
mimic_dump_load(int argc, VALUE *argv, VALUE self) {
    if (1 > argc) {
	rb_raise(rb_eArgError, "wrong number of arguments (0 for 1)");
    } else if (T_STRING == rb_type(*argv)) {
	return mimic_load(argc, argv, self);
    } else {
	return mimic_dump(argc, argv, self);
    }
    return Qnil;
}

static VALUE
mimic_generate_core(int argc, VALUE *argv, Options copts) {
    char	buf[4096];
    struct _Out	out;
    VALUE	rstr;
    
    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    if (2 == argc && Qnil != argv[1]) {
	VALUE	ropts = argv[1];
	VALUE	v;
	size_t	len;

	if (T_HASH != rb_type(ropts)) {
	    rb_raise(rb_eArgError, "options must be a hash.");
	}
	if (Qnil != (v = rb_hash_lookup(ropts, indent_sym))) { // TBD fixnum also ok
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.indent_str) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "indent string is limited to %lu characters.", sizeof(copts->dump_opts.indent_str));
	    }
	    strcpy(copts->dump_opts.indent_str, StringValuePtr(v));
	    copts->dump_opts.indent_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, space_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.after_sep) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "space string is limited to %lu characters.", sizeof(copts->dump_opts.after_sep));
	    }
	    strcpy(copts->dump_opts.after_sep, StringValuePtr(v));
	    copts->dump_opts.after_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, space_before_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.before_sep) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "space_before string is limited to %lu characters.", sizeof(copts->dump_opts.before_sep));
	    }
	    strcpy(copts->dump_opts.before_sep, StringValuePtr(v));
	    copts->dump_opts.before_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, object_nl_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.hash_nl) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "object_nl string is limited to %lu characters.", sizeof(copts->dump_opts.hash_nl));
	    }
	    strcpy(copts->dump_opts.hash_nl, StringValuePtr(v));
	    copts->dump_opts.hash_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	if (Qnil != (v = rb_hash_lookup(ropts, array_nl_sym))) {
	    rb_check_type(v, T_STRING);
	    if (sizeof(copts->dump_opts.array_nl) <= (len = RSTRING_LEN(v))) {
		rb_raise(rb_eArgError, "array_nl string is limited to %lu characters.", sizeof(copts->dump_opts.array_nl));
	    }
	    strcpy(copts->dump_opts.array_nl, StringValuePtr(v));
	    copts->dump_opts.array_size = (uint8_t)len;
	    copts->dump_opts.use = true;
	}
	// :allow_nan is not supported as Oj always allows_nan
	// :max_nesting is always set to 100
    }
    oj_dump_obj_to_json(*argv, copts, &out);
    if (0 == out.buf) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(out.buf);
    rstr = oj_encode(rstr);
    if (out.allocated) {
	xfree(out.buf);
    }
    return rstr;
}

static VALUE
mimic_generate(int argc, VALUE *argv, VALUE self) {
    struct _Options	copts = oj_default_options;

    return mimic_generate_core(argc, argv, &copts);
}

static VALUE
mimic_pretty_generate(int argc, VALUE *argv, VALUE self) {
    struct _Options	copts = oj_default_options;

    strcpy(copts.dump_opts.indent_str, "  ");
    copts.dump_opts.indent_size = (uint8_t)strlen(copts.dump_opts.indent_str);
    strcpy(copts.dump_opts.before_sep, " ");
    copts.dump_opts.before_size = (uint8_t)strlen(copts.dump_opts.before_sep);
    strcpy(copts.dump_opts.after_sep, " ");
    copts.dump_opts.after_size = (uint8_t)strlen(copts.dump_opts.after_sep);
    strcpy(copts.dump_opts.hash_nl, "\n");
    copts.dump_opts.hash_size = (uint8_t)strlen(copts.dump_opts.hash_nl);
    strcpy(copts.dump_opts.array_nl, "\n");
    copts.dump_opts.array_size = (uint8_t)strlen(copts.dump_opts.array_nl);
    copts.dump_opts.use = true;

    return mimic_generate_core(argc, argv, &copts);
}

static VALUE
mimic_parse(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;
    VALUE		args[1];

    if (argc < 1) {
	rb_raise(rb_eArgError, "Wrong number of arguments to parse.");
    }
    oj_set_compat_callbacks(&pi);
    pi.err_class = json_parser_error_class;
    pi.options = oj_default_options;
    pi.options.auto_define = No;
    pi.options.quirks_mode = No;

    if (2 <= argc) {
	VALUE	ropts = argv[1];
	VALUE	v;

	if (T_HASH != rb_type(ropts)) {
	    rb_raise(rb_eArgError, "options must be a hash.");
	}
	if (Qnil != (v = rb_hash_lookup(ropts, symbolize_names_sym))) {
	    pi.options.sym_key = (Qtrue == v) ? Yes : No;
	}

	if (Qnil != (v = rb_hash_lookup(ropts, quirks_mode_sym))) {
	    pi.options.quirks_mode = (Qtrue == v) ? Yes : No;
	}

	if (Qnil != (v = rb_hash_lookup(ropts, create_additions_sym))) {
	    if (Qfalse == v) {
		oj_set_strict_callbacks(&pi);
	    }
	}
	// :allow_nan is not supported as Oj always allows nan
	// :max_nesting is ignored as Oj has not nesting limit
	// :object_class is always Hash
	// :array_class is always Array
    }
    *args = *argv;

    return oj_pi_parse(1, args, &pi, 0, 0, 0);
}

static VALUE
mimic_recurse_proc(VALUE self, VALUE obj) {
    rb_need_block();
    mimic_walk(Qnil, obj, Qnil);

    return Qnil;
}

static VALUE
no_op1(VALUE self, VALUE obj) {
    return Qnil;
}

static VALUE
mimic_create_id(VALUE self, VALUE id) {
    Check_Type(id, T_STRING);

    if (0 != oj_default_options.create_id) {
	if (json_class != oj_default_options.create_id) {
	    xfree((char*)oj_default_options.create_id);
	}
	oj_default_options.create_id = 0;
	oj_default_options.create_id_len = 0;
    }
    if (Qnil != id) {
	size_t	len = RSTRING_LEN(id) + 1;

	oj_default_options.create_id = ALLOC_N(char, len);
	strcpy((char*)oj_default_options.create_id, StringValuePtr(id));
	oj_default_options.create_id_len = len - 1;
    }
    return id;
}

static struct _Options	mimic_object_to_json_options = {
    0,		// indent
    No,		// circular
    No,		// auto_define
    No,		// sym_key
    JSONEsc,	// escape_mode
    CompatMode,	// mode
    No,		// class_cache
    RubyTime,	// time_format
    No,		// bigdec_as_num
    FloatDec,	// bigdec_load
    No,		// to_json
    Yes,	// nilnil
    Yes,	// allow_gc
    Yes,	// quirks_mode
    json_class,	// create_id
    10,		// create_id_len
    9,		// sec_prec
    15,		// float_prec
    "%0.15g",	// float_fmt
    {		// dump_opts
	false,	//use
	"",	// indent
	"",	// before_sep
	"",	// after_sep
	"",	// hash_nl
	"",	// array_nl
	0,	// indent_size
	0,	// before_size
	0,	// after_size
	0,	// hash_size
	0,	// array_size
	AutoNan,// nan_dump
    }
};

static VALUE
mimic_object_to_json(int argc, VALUE *argv, VALUE self) {
    char		buf[4096];
    struct _Out		out;
    VALUE		rstr;
    struct _Options	copts = oj_default_options;

    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    // Have to turn off to_json to avoid the Active Support recursion problem.
    copts.to_json = No;
    // To be strict the mimic_object_to_json_options should be used but people
    // seem to prefer the option of changing that.
    //oj_dump_obj_to_json(self, &mimic_object_to_json_options, &out);
    oj_dump_obj_to_json_using_params(self, &copts, &out, argc, argv);
    if (0 == out.buf) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(out.buf);
    rstr = oj_encode(rstr);
    if (out.allocated) {
	xfree(out.buf);
    }
    return rstr;
}


/* Document-method: mimic_JSON
 *    call-seq: mimic_JSON() => Module
 *
 * Creates the JSON module with methods and classes to mimic the JSON gem. After
 * this method is invoked calls that expect the JSON module will use Oj instead
 * and be faster than the original JSON. Most options that could be passed to
 * the JSON methods are supported. The calls to set parser or generator will not
 * raise an Exception but will not have any effect. The method can also be
 * called after the json gem is loaded. The necessary methods on the json gem
 * will be replaced with Oj methods.
 *
 * Note that this also sets the default options of :mode to :compat and
 * :encoding to :ascii.
 */
static VALUE
define_mimic_json(int argc, VALUE *argv, VALUE self) {
    VALUE	ext;
    VALUE	dummy;
    VALUE	json_error;
    
    // Either set the paths to indicate JSON has been loaded or replaces the
    // methods if it has been loaded.
    if (rb_const_defined_at(rb_cObject, rb_intern("JSON"))) {
	mimic = rb_const_get_at(rb_cObject, rb_intern("JSON"));
    } else {
	mimic = rb_define_module("JSON");
    }
    rb_define_module_function(rb_cObject, "JSON", mimic_dump_load, -1);
    if (rb_const_defined_at(mimic, rb_intern("Ext"))) {
	ext = rb_const_get_at(mimic, rb_intern("Ext"));
     } else {
	ext = rb_define_module_under(mimic, "Ext");
    }
    if (!rb_const_defined_at(ext, rb_intern("Parser"))) {
	dummy = rb_define_class_under(ext, "Parser", rb_cObject);
    }
    if (!rb_const_defined_at(ext, rb_intern("Generator"))) {
	dummy = rb_define_class_under(ext, "Generator", rb_cObject);
    }
    // convince Ruby that the json gem has already been loaded
    dummy = rb_gv_get("$LOADED_FEATURES");
    if (rb_type(dummy) == T_ARRAY) {
	rb_ary_push(dummy, rb_str_new2("json"));
	if (0 < argc) {
	    VALUE	mimic_args[1];

	    *mimic_args = *argv;
	    rb_funcall2(Oj, rb_intern("mimic_loaded"), 1, mimic_args);
	} else {
	    rb_funcall2(Oj, rb_intern("mimic_loaded"), 0, 0);
	}
    }
    dummy = rb_gv_get("$VERBOSE");
    rb_gv_set("$VERBOSE", Qfalse);
    rb_define_module_function(mimic, "parser=", no_op1, 1);
    rb_define_module_function(mimic, "generator=", no_op1, 1);
    rb_define_module_function(mimic, "create_id=", mimic_create_id, 1);

    rb_define_module_function(mimic, "dump", mimic_dump, -1);
    rb_define_module_function(mimic, "load", mimic_load, -1);
    rb_define_module_function(mimic, "restore", mimic_load, -1);
    rb_define_module_function(mimic, "recurse_proc", mimic_recurse_proc, 1);
    rb_define_module_function(mimic, "[]", mimic_dump_load, -1);

    rb_define_module_function(mimic, "generate", mimic_generate, -1);
    rb_define_module_function(mimic, "fast_generate", mimic_generate, -1);
    rb_define_module_function(mimic, "pretty_generate", mimic_pretty_generate, -1);
    /* for older versions of JSON, the deprecated unparse methods */
    rb_define_module_function(mimic, "unparse", mimic_generate, -1);
    rb_define_module_function(mimic, "fast_unparse", mimic_generate, -1);
    rb_define_module_function(mimic, "pretty_unparse", mimic_pretty_generate, -1);

    rb_define_module_function(mimic, "parse", mimic_parse, -1);
    rb_define_module_function(mimic, "parse!", mimic_parse, -1);

    rb_define_method(rb_cObject, "to_json", mimic_object_to_json, -1);

    rb_gv_set("$VERBOSE", dummy);

    create_additions_sym = ID2SYM(rb_intern("create_additions"));	rb_gc_register_address(&create_additions_sym);
    symbolize_names_sym = ID2SYM(rb_intern("symbolize_names"));		rb_gc_register_address(&symbolize_names_sym);

    if (rb_const_defined_at(mimic, rb_intern("JSONError"))) {
	rb_funcall(mimic, rb_intern("remove_const"), 1, ID2SYM(rb_intern("JSONError")));
    }
    json_error = rb_define_class_under(mimic, "JSONError", rb_eStandardError);
    if (rb_const_defined_at(mimic, rb_intern("ParserError"))) {
	rb_funcall(mimic, rb_intern("remove_const"), 1, ID2SYM(rb_intern("ParserError")));
    }
    json_parser_error_class = rb_define_class_under(mimic, "ParserError", json_error);

    if (!rb_const_defined_at(mimic, rb_intern("State"))) {
        rb_define_class_under(mimic, "State", rb_cObject);
    }

    oj_default_options = mimic_object_to_json_options;
    oj_default_options.to_json = Yes;

    return mimic;
}

/*
extern void	oj_hash_test();

static VALUE
hash_test(VALUE self) {
    oj_hash_test();
    return Qnil;
}
*/

#if !HAS_ENCODING_SUPPORT
static VALUE
iconv_encoder(VALUE x) {
    VALUE	iconv;

    rb_require("iconv");
    iconv = rb_const_get(rb_cObject, rb_intern("Iconv"));

    return rb_funcall(iconv, rb_intern("new"), 2, rb_str_new2("ASCII//TRANSLIT"), rb_str_new2("UTF-8"));
}

static VALUE
iconv_rescue(VALUE x) {
    return Qnil;
}
#endif

static VALUE
protect_require(VALUE x) {
    rb_require("time");
    rb_require("bigdecimal");
    return Qnil;
}

void Init_oj() {
    int	err = 0;

    Oj = rb_define_module("Oj");

    oj_cstack_class = rb_define_class_under(Oj, "CStack", rb_cObject);

    oj_string_writer_class = rb_define_class_under(Oj, "StringWriter", rb_cObject);
    rb_define_module_function(oj_string_writer_class, "new", str_writer_new, -1);
    rb_define_method(oj_string_writer_class, "push_key", str_writer_push_key, 1);
    rb_define_method(oj_string_writer_class, "push_object", str_writer_push_object, -1);
    rb_define_method(oj_string_writer_class, "push_array", str_writer_push_array, -1);
    rb_define_method(oj_string_writer_class, "push_value", str_writer_push_value, -1);
    rb_define_method(oj_string_writer_class, "push_json", str_writer_push_json, -1);
    rb_define_method(oj_string_writer_class, "pop", str_writer_pop, 0);
    rb_define_method(oj_string_writer_class, "pop_all", str_writer_pop_all, 0);
    rb_define_method(oj_string_writer_class, "reset", str_writer_reset, 0);
    rb_define_method(oj_string_writer_class, "to_s", str_writer_to_s, 0);

    oj_stream_writer_class = rb_define_class_under(Oj, "StreamWriter", rb_cObject);
    rb_define_module_function(oj_stream_writer_class, "new", stream_writer_new, -1);
    rb_define_method(oj_stream_writer_class, "push_key", stream_writer_push_key, 1);
    rb_define_method(oj_stream_writer_class, "push_object", stream_writer_push_object, -1);
    rb_define_method(oj_stream_writer_class, "push_array", stream_writer_push_array, -1);
    rb_define_method(oj_stream_writer_class, "push_value", stream_writer_push_value, -1);
    rb_define_method(oj_stream_writer_class, "push_json", stream_writer_push_json, -1);
    rb_define_method(oj_stream_writer_class, "pop", stream_writer_pop, 0);
    rb_define_method(oj_stream_writer_class, "pop_all", stream_writer_pop_all, 0);

    rb_require("time");
    rb_require("date");
    // On Rubinius the require fails but can be done from a ruby file.
    rb_protect(protect_require, Qnil, &err);
#if NEEDS_RATIONAL
    rb_require("rational");
#endif
    rb_require("stringio");
#if HAS_ENCODING_SUPPORT
    oj_utf8_encoding = rb_enc_find("UTF-8");
#else
    // need an option to turn this on
    oj_utf8_encoding = rb_rescue(iconv_encoder, Qnil, iconv_rescue, Qnil);
    oj_utf8_encoding = Qnil;
#endif

    //rb_define_module_function(Oj, "hash_test", hash_test, 0);

    rb_define_module_function(Oj, "default_options", get_def_opts, 0);
    rb_define_module_function(Oj, "default_options=", set_def_opts, 1);

    rb_define_module_function(Oj, "mimic_JSON", define_mimic_json, -1);
    rb_define_module_function(Oj, "load", load, -1);
    rb_define_module_function(Oj, "load_file", load_file, -1);
    rb_define_module_function(Oj, "safe_load", safe_load, 1);
    rb_define_module_function(Oj, "strict_load", oj_strict_parse, -1);
    rb_define_module_function(Oj, "compat_load", oj_compat_parse, -1);
    rb_define_module_function(Oj, "object_load", oj_object_parse, -1);

    rb_define_module_function(Oj, "dump", dump, -1);
    rb_define_module_function(Oj, "to_file", to_file, -1);
    rb_define_module_function(Oj, "to_stream", to_stream, -1);
    rb_define_module_function(Oj, "register_odd", register_odd, -1);
    rb_define_module_function(Oj, "register_odd_raw", register_odd_raw, -1);

    rb_define_module_function(Oj, "saj_parse", oj_saj_parse, -1);
    rb_define_module_function(Oj, "sc_parse", oj_sc_parse, -1);

    oj_add_value_id = rb_intern("add_value");
    oj_array_append_id = rb_intern("array_append");
    oj_array_end_id = rb_intern("array_end");
    oj_array_start_id = rb_intern("array_start");
    oj_as_json_id = rb_intern("as_json");
    oj_error_id = rb_intern("error");
    oj_file_id = rb_intern("file?");
    oj_fileno_id = rb_intern("fileno");
    oj_ftype_id = rb_intern("ftype");
    oj_hash_end_id = rb_intern("hash_end");
    oj_hash_key_id = rb_intern("hash_key");
    oj_hash_set_id = rb_intern("hash_set");
    oj_hash_start_id = rb_intern("hash_start");
    oj_iconv_id = rb_intern("iconv");
    oj_instance_variables_id = rb_intern("instance_variables");
    oj_json_create_id = rb_intern("json_create");
    oj_length_id = rb_intern("length");
    oj_new_id = rb_intern("new");
    oj_parse_id = rb_intern("parse");
    oj_pos_id = rb_intern("pos");
    oj_read_id = rb_intern("read");
    oj_readpartial_id = rb_intern("readpartial");
    oj_replace_id = rb_intern("replace");
    oj_stat_id = rb_intern("stat");
    oj_string_id = rb_intern("string");
    oj_to_hash_id = rb_intern("to_hash");
    oj_to_json_id = rb_intern("to_json");
    oj_to_s_id = rb_intern("to_s");
    oj_to_sym_id = rb_intern("to_sym");
    oj_to_time_id = rb_intern("to_time");
    oj_tv_nsec_id = rb_intern("tv_nsec");
    oj_tv_sec_id = rb_intern("tv_sec");
    oj_tv_usec_id = rb_intern("tv_usec");
    oj_utc_id = rb_intern("utc");
    oj_utc_offset_id = rb_intern("utc_offset");
    oj_utcq_id = rb_intern("utc?");
    oj_write_id = rb_intern("write");
    has_key_id = rb_intern("has_key?");

    rb_require("oj/bag");
    rb_require("oj/error");
    rb_require("oj/mimic");
    rb_require("oj/saj");
    rb_require("oj/schandler");

    oj_bag_class = rb_const_get_at(Oj, rb_intern("Bag"));
    oj_bigdecimal_class = rb_const_get(rb_cObject, rb_intern("BigDecimal"));
    oj_date_class = rb_const_get(rb_cObject, rb_intern("Date"));
    oj_datetime_class = rb_const_get(rb_cObject, rb_intern("DateTime"));
    oj_parse_error_class = rb_const_get_at(Oj, rb_intern("ParseError"));
    oj_stringio_class = rb_const_get(rb_cObject, rb_intern("StringIO"));
    oj_struct_class = rb_const_get(rb_cObject, rb_intern("Struct"));
    json_parser_error_class = Qnil; // replaced if mimic is called

    allow_gc_sym = ID2SYM(rb_intern("allow_gc"));	rb_gc_register_address(&allow_gc_sym);
    array_nl_sym = ID2SYM(rb_intern("array_nl"));	rb_gc_register_address(&array_nl_sym);
    ascii_only_sym = ID2SYM(rb_intern("ascii_only"));	rb_gc_register_address(&ascii_only_sym);
    ascii_sym = ID2SYM(rb_intern("ascii"));		rb_gc_register_address(&ascii_sym);
    auto_define_sym = ID2SYM(rb_intern("auto_define"));	rb_gc_register_address(&auto_define_sym);
    auto_sym = ID2SYM(rb_intern("auto"));		rb_gc_register_address(&auto_sym);
    bigdecimal_as_decimal_sym = ID2SYM(rb_intern("bigdecimal_as_decimal"));rb_gc_register_address(&bigdecimal_as_decimal_sym);
    bigdecimal_load_sym = ID2SYM(rb_intern("bigdecimal_load"));rb_gc_register_address(&bigdecimal_load_sym);
    bigdecimal_sym = ID2SYM(rb_intern("bigdecimal"));	rb_gc_register_address(&bigdecimal_sym);
    circular_sym = ID2SYM(rb_intern("circular"));	rb_gc_register_address(&circular_sym);
    class_cache_sym = ID2SYM(rb_intern("class_cache"));	rb_gc_register_address(&class_cache_sym);
    compat_sym = ID2SYM(rb_intern("compat"));		rb_gc_register_address(&compat_sym);
    create_id_sym = ID2SYM(rb_intern("create_id"));	rb_gc_register_address(&create_id_sym);
    escape_mode_sym = ID2SYM(rb_intern("escape_mode"));	rb_gc_register_address(&escape_mode_sym);
    float_prec_sym = ID2SYM(rb_intern("float_precision"));rb_gc_register_address(&float_prec_sym);
    float_sym = ID2SYM(rb_intern("float"));		rb_gc_register_address(&float_sym);
    huge_sym = ID2SYM(rb_intern("huge"));		rb_gc_register_address(&huge_sym);
    indent_sym = ID2SYM(rb_intern("indent"));		rb_gc_register_address(&indent_sym);
    json_sym = ID2SYM(rb_intern("json"));		rb_gc_register_address(&json_sym);
    mode_sym = ID2SYM(rb_intern("mode"));		rb_gc_register_address(&mode_sym);
    nan_sym = ID2SYM(rb_intern("nan"));		rb_gc_register_address(&nan_sym);
    newline_sym = ID2SYM(rb_intern("newline"));		rb_gc_register_address(&newline_sym);
    nilnil_sym = ID2SYM(rb_intern("nilnil"));		rb_gc_register_address(&nilnil_sym);
    null_sym = ID2SYM(rb_intern("null"));		rb_gc_register_address(&null_sym);
    object_nl_sym = ID2SYM(rb_intern("object_nl"));	rb_gc_register_address(&object_nl_sym);
    object_sym = ID2SYM(rb_intern("object"));		rb_gc_register_address(&object_sym);
    quirks_mode_sym = ID2SYM(rb_intern("quirks_mode"));	rb_gc_register_address(&quirks_mode_sym);
    raise_sym = ID2SYM(rb_intern("raise"));		rb_gc_register_address(&raise_sym);
    ruby_sym = ID2SYM(rb_intern("ruby"));		rb_gc_register_address(&ruby_sym);
    sec_prec_sym = ID2SYM(rb_intern("second_precision"));rb_gc_register_address(&sec_prec_sym);
    space_before_sym = ID2SYM(rb_intern("space_before"));rb_gc_register_address(&space_before_sym);
    space_sym = ID2SYM(rb_intern("space"));		rb_gc_register_address(&space_sym);
    strict_sym = ID2SYM(rb_intern("strict"));		rb_gc_register_address(&strict_sym);
    symbol_keys_sym = ID2SYM(rb_intern("symbol_keys"));	rb_gc_register_address(&symbol_keys_sym);
    time_format_sym = ID2SYM(rb_intern("time_format"));	rb_gc_register_address(&time_format_sym);
    unix_sym = ID2SYM(rb_intern("unix"));		rb_gc_register_address(&unix_sym);
    unix_zone_sym = ID2SYM(rb_intern("unix_zone"));	rb_gc_register_address(&unix_zone_sym);
    use_to_json_sym = ID2SYM(rb_intern("use_to_json"));	rb_gc_register_address(&use_to_json_sym);
    word_sym = ID2SYM(rb_intern("word"));		rb_gc_register_address(&word_sym);
    xmlschema_sym = ID2SYM(rb_intern("xmlschema"));	rb_gc_register_address(&xmlschema_sym);
    xss_safe_sym = ID2SYM(rb_intern("xss_safe"));	rb_gc_register_address(&xss_safe_sym);

    oj_slash_string = rb_str_new2("/");			rb_gc_register_address(&oj_slash_string);

    oj_default_options.mode = ObjectMode;

    oj_hash_init();
    oj_odd_init();

#if USE_PTHREAD_MUTEX
    pthread_mutex_init(&oj_cache_mutex, 0);
#elif USE_RB_MUTEX
    oj_cache_mutex = rb_mutex_new();
    rb_gc_register_address(&oj_cache_mutex);
#endif
    oj_init_doc();
}

// mimic JSON documentation

/* Document-module: JSON
 * 
 * JSON is a JSON parser. This module when defined by the Oj module is a
 * faster replacement for the original.
 */
/* Document-module: JSON::Ext
 * 
 * The Ext module is a placeholder in the mimic JSON module used for
 * compatibility only.
 */
/* Document-class: JSON::Ext::Parser
 * 
 * The JSON::Ext::Parser is a placeholder in the mimic JSON module used for
 * compatibility only.
 */
/* Document-class: JSON::Ext::Generator
 * 
 * The JSON::Ext::Generator is a placeholder in the mimic JSON module used for
 * compatibility only.
 */

/* Document-method: create_id=
 *   call-seq: create_id=(id) -> String
 *
 * Sets the create_id tag to look for in JSON document. That key triggers the
 * creation of a class with the same name.
 *
 * @param [nil|String] id new create_id
 * @return the id
 */
/* Document-method: parser=
 *   call-seq: parser=(parser) -> nil
 * 
 * Does nothing other than provide compatibiltiy.
 * @param [Object] parser ignored
 */
/* Document-method: generator=
 *   call-seq: generator=(generator) -> nil
 * 
 * Does nothing other than provide compatibiltiy.
 * @param [Object] generator ignored
 */
/* Document-method: dump
 *   call-seq: dump(obj, anIO=nil, limit = nil) -> String
 * 
 * Encodes an object as a JSON String.
 * 
 * @param [Object] obj object to convert to encode as JSON
 * @param [IO] anIO an IO that allows writing
 * @param [Fixnum] limit ignored
 */
/* Document-method: load
 *   call-seq: load(source, proc=nil) -> Object
 * 
 * Loads a Ruby Object from a JSON source that can be either a String or an
 * IO. If Proc is given or a block is providedit is called with each nested
 * element of the loaded Object.
 * 
 * @param [String|IO] source JSON source
 * @param [Proc] proc to yield to on each element or nil
 */
/* Document-method: restore
 *   call-seq: restore(source, proc=nil) -> Object
 * 
 * Loads a Ruby Object from a JSON source that can be either a String or an
 * IO. If Proc is given or a block is providedit is called with each nested
 * element of the loaded Object.
 * 
 * @param [String|IO] source JSON source
 * @param [Proc] proc to yield to on each element or nil
 */
/* Document-method: recurse_proc
 *   call-seq: recurse_proc(obj, &proc) -> nil
 * 
 * Yields to the proc for every element in the obj recursivly.
 * 
 * @param [Hash|Array] obj object to walk
 * @param [Proc] proc to yield to on each element
 */
/* Document-method: []
 *   call-seq: [](obj, opts={}) -> Object
 * 
 * If the obj argument is a String then it is assumed to be a JSON String and
 * parsed otherwise the obj is encoded as a JSON String.
 * 
 * @param [String|Hash|Array] obj object to convert
 * @param [Hash] opts same options as either generate or parse
 */
/* Document-method: generate
 *   call-seq: generate(obj, opts=nil) -> String
 * 
 * Encode obj as a JSON String. The obj argument must be a Hash, Array, or
 * respond to to_h or to_json. Options other than those listed such as
 * +:allow_nan+ or +:max_nesting+ are ignored.
 * 
 * @param [Object|Hash|Array] obj object to convert to a JSON String
 * @param [Hash] opts options
 * @param [String] :indent String to use for indentation
 * @param [String] :space String placed after a , or : delimiter
 * @param [String] :space_before String placed before a : delimiter
 * @param [String] :object_nl String placed after a JSON object
 * @param [String] :array_nl String placed after a JSON array
 */
/* Document-method: fast_generate
 *   call-seq: fast_generate(obj, opts=nil) -> String
 * Same as generate().
 * @see generate
 */
/* Document-method: pretty_generate
 *   call-seq: pretty_generate(obj, opts=nil) -> String
 * Same as generate() but with different defaults for the spacing options.
 * @see generate
 */
/* Document-method: parse
 *   call-seq: parse(source, opts=nil) -> Object
 *
 * Parses a JSON String or IO into a Ruby Object.  Options other than those
 * listed such as +:allow_nan+ or +:max_nesting+ are ignored. +:object_class+ and
 * +:array_object+ are not supported.
 *
 * @param [String|IO] source source to parse
 * @param [Hash] opts options
 * @param [true|false] :symbolize_names flag indicating JSON object keys should be Symbols instead of Strings
 * @param [true|false] :create_additions flag indicating a key matching +create_id+ in a JSON object should trigger the creation of Ruby Object
 * @see create_id=
 */
/* Document-method: parse!
 *   call-seq: parse!(source, opts=nil) -> Object
 * Same as parse().
 * @see parse
 */
