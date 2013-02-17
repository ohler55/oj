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

#include "oj.h"

typedef struct _YesNoOpt {
    VALUE	sym;
    char	*attr;
} *YesNoOpt;

void Init_oj();

VALUE	 Oj = Qnil;

ID	oj_add_value_id;
ID	oj_array_end_id;
ID	oj_array_start_id;
ID	oj_as_json_id;
ID	oj_error_id;
ID	oj_fileno_id;
ID	oj_hash_end_id;
ID	oj_hash_start_id;
ID	oj_instance_variables_id;
ID	oj_json_create_id;
ID	oj_new_id;
ID	oj_read_id;
ID	oj_string_id;
ID	oj_to_hash_id;
ID	oj_to_json_id;
ID	oj_to_s_id;
ID	oj_to_sym_id;
ID	oj_to_time_id;
ID	oj_tv_nsec_id;
ID	oj_tv_sec_id;
ID	oj_tv_usec_id;
ID	oj_utc_offset_id;
ID	oj_write_id;

VALUE	oj_bag_class;
VALUE	oj_bigdecimal_class;
VALUE	oj_date_class;
VALUE	oj_datetime_class;
VALUE	oj_parse_error_class;
VALUE	oj_stringio_class;
VALUE	oj_struct_class;
VALUE	oj_time_class;

VALUE	oj_slash_string;

static VALUE	ascii_only_sym;
static VALUE	auto_define_sym;
static VALUE	bigdecimal_as_decimal_sym;
static VALUE	circular_sym;
static VALUE	compat_sym;
static VALUE	create_id_sym;
static VALUE	indent_sym;
static VALUE	max_stack_sym;
static VALUE	mode_sym;
static VALUE	null_sym;
static VALUE	object_sym;
static VALUE	ruby_sym;
static VALUE	sec_prec_sym;
static VALUE	strict_sym;
static VALUE	symbol_keys_sym;
static VALUE	time_format_sym;
static VALUE	unix_sym;
static VALUE	xmlschema_sym;

static VALUE	array_nl_sym;
static VALUE	create_additions_sym;
static VALUE	object_nl_sym;
static VALUE	space_before_sym;
static VALUE	space_sym;
static VALUE	symbolize_names_sym;

static VALUE	mimic = Qnil;

Cache	oj_class_cache = 0;
Cache	oj_attr_cache = 0;

#if HAS_ENCODING_SUPPORT
rb_encoding	*oj_utf8_encoding = 0;
#endif

#if SAFE_CACHE
pthread_mutex_t	oj_cache_mutex; // only used if SAFE_CACHE defined
#endif
static const char	json_class[] = "json_class";

struct _Options	oj_default_options = {
    0,			// indent
    No,			// circular
    No,			// auto_define
    No,			// sym_key
    No,			// ascii_only
    ObjectMode,		// mode
    UnixTime,		// time_format
    Yes,		// bigdec_as_num
    json_class,		// create_id
    65536,		// max_stack
    9,			// sec_prec
    0,			// dump_opts
};

static VALUE	define_mimic_json(int argc, VALUE *argv, VALUE self);
static struct _Odd	odds[5]; // bump up if new Odd classes are added

Odd
oj_get_odd(VALUE clas) {
    Odd	odd = odds;

    for (; Qundef != odd->clas; odd++) {
	if (clas == odd->clas) {
	    return odd;
	}
    }
    return 0;
}

/* call-seq: default_options() => Hash
 *
 * Returns the default load and dump options as a Hash. The options are
 * - indent: [Fixnum] number of spaces to indent each element in an JSON document
 * - circular: [true|false|nil] support circular references while dumping
 * - auto_define: [true|false|nil] automatically define classes if they do not exist
 * - symbol_keys: [true|false|nil] use symbols instead of strings for hash keys
 * - mode: [:object|:strict|:compat|:null] load and dump modes to use for JSON
 * - time_format: [:unix|:xmlschema|:ruby] time format when dumping in :compat mode
 * - bigdecimal_as_decimal: [true|false|nil] dump BigDecimal as a decimal number or as a String
 * - create_id: [String|nil] create id for json compatible object encoding, default is 'json_create'
 * - max_stack: [Fixnum|nil] maximum json size to allocate on the stack, default is 65536
 * - second_precision: [Fixnum|nil] number of digits after the decimal when dumping the seconds portion of time
 * @return [Hash] all current option settings.
 */
static VALUE
get_def_opts(VALUE self) {
    VALUE	opts = rb_hash_new();
    
    rb_hash_aset(opts, indent_sym, INT2FIX(oj_default_options.indent));
    rb_hash_aset(opts, sec_prec_sym, INT2FIX(oj_default_options.sec_prec));
    rb_hash_aset(opts, max_stack_sym, INT2FIX(oj_default_options.max_stack));
    rb_hash_aset(opts, circular_sym, (Yes == oj_default_options.circular) ? Qtrue : ((No == oj_default_options.circular) ? Qfalse : Qnil));
    rb_hash_aset(opts, auto_define_sym, (Yes == oj_default_options.auto_define) ? Qtrue : ((No == oj_default_options.auto_define) ? Qfalse : Qnil));
    rb_hash_aset(opts, ascii_only_sym, (Yes == oj_default_options.ascii_only) ? Qtrue : ((No == oj_default_options.ascii_only) ? Qfalse : Qnil));
    rb_hash_aset(opts, symbol_keys_sym, (Yes == oj_default_options.sym_key) ? Qtrue : ((No == oj_default_options.sym_key) ? Qfalse : Qnil));
    rb_hash_aset(opts, bigdecimal_as_decimal_sym, (Yes == oj_default_options.bigdec_as_num) ? Qtrue : ((No == oj_default_options.bigdec_as_num) ? Qfalse : Qnil));
    switch (oj_default_options.mode) {
    case StrictMode:	rb_hash_aset(opts, mode_sym, strict_sym);	break;
    case CompatMode:	rb_hash_aset(opts, mode_sym, compat_sym);	break;
    case NullMode:	rb_hash_aset(opts, mode_sym, null_sym);		break;
    case ObjectMode:
    default:		rb_hash_aset(opts, mode_sym, object_sym);	break;
    }
    switch (oj_default_options.time_format) {
    case XmlTime:	rb_hash_aset(opts, time_format_sym, xmlschema_sym);	break;
    case RubyTime:	rb_hash_aset(opts, time_format_sym, ruby_sym);		break;
    case UnixTime:
    default:		rb_hash_aset(opts, time_format_sym, unix_sym);		break;
    }
    rb_hash_aset(opts, create_id_sym, (0 == oj_default_options.create_id) ? Qnil : rb_str_new2(oj_default_options.create_id));

    return opts;
}

/* call-seq: default_options=(opts)
 *
 * Sets the default options for load and dump.
 * @param [Hash] opts options to change
 * @param [Fixnum] :indent number of spaces to indent each element in an JSON document
 * @param [true|false|nil] :circular support circular references while dumping
 * @param [true|false|nil] :auto_define automatically define classes if they do not exist
 * @param [true|false|nil] :symbol_keys convert hash keys to symbols
 * @param [true|false|nil] :ascii_only encode all high-bit characters as escaped sequences if true
 * @param [true|false|nil] :bigdecimal_as_decimal dump BigDecimal as a decimal number or as a String
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
 *        :xmlschema date-time format taken from XML Schema as a String,
 *        :ruby Time.to_s formatted String
 * @param [String|nil] :create_id create id for json compatible object encoding
 * @param [Fixnum|nil] :max_stack maximum size to allocate on the stack for a JSON String
 * @param [Fixnum|nil] :second_precision number of digits after the decimal when dumping the seconds portion of time
 * @return [nil]
 */
static VALUE
set_def_opts(VALUE self, VALUE opts) {
    struct _YesNoOpt	ynos[] = {
	{ circular_sym, &oj_default_options.circular },
	{ auto_define_sym, &oj_default_options.auto_define },
	{ symbol_keys_sym, &oj_default_options.sym_key },
	{ ascii_only_sym, &oj_default_options.ascii_only },
	{ bigdecimal_as_decimal_sym, &oj_default_options.bigdec_as_num },
	{ Qnil, 0 }
    };
    YesNoOpt	o;
    VALUE	v;
    
    Check_Type(opts, T_HASH);
    v = rb_hash_aref(opts, indent_sym);
    if (Qnil != v) {
	Check_Type(v, T_FIXNUM);
	oj_default_options.indent = FIX2INT(v);
    }
    v = rb_hash_aref(opts, sec_prec_sym);
    if (Qnil != v) {
	int	n;

	Check_Type(v, T_FIXNUM);
	n = FIX2INT(v);
	if (0 > n) {
	    n = 0;
	} else if (9 < n) {
	    n = 9;
	}
	oj_default_options.sec_prec = n;
    }
    v = rb_hash_aref(opts, max_stack_sym);
    if (Qnil != v) {
	int	i;

	Check_Type(v, T_FIXNUM);
	i = FIX2INT(v);
	if (0 > i) {
	    i = 0;
	}
	oj_default_options.max_stack = (size_t)i;
    }

    v = rb_hash_lookup(opts, mode_sym);
    if (Qnil == v) {
	// ignore
    } else if (object_sym == v) {
	oj_default_options.mode = ObjectMode;
    } else if (strict_sym == v) {
	oj_default_options.mode = StrictMode;
    } else if (compat_sym == v) {
	oj_default_options.mode = CompatMode;
    } else if (null_sym == v) {
	oj_default_options.mode = NullMode;
    } else {
	rb_raise(rb_eArgError, ":mode must be :object, :strict, :compat, or :null.");
    }

    v = rb_hash_lookup(opts, time_format_sym);
    if (Qnil == v) {
	// ignore
    } else if (unix_sym == v) {
	oj_default_options.time_format = UnixTime;
    } else if (xmlschema_sym == v) {
	oj_default_options.time_format = XmlTime;
    } else if (ruby_sym == v) {
	oj_default_options.time_format = RubyTime;
    } else {
	rb_raise(rb_eArgError, ":time_format must be :unix, :xmlschema, or :ruby.");
    }

    if (Qtrue == rb_funcall(opts, rb_intern("has_key?"), 1, create_id_sym)) {
	if (0 != oj_default_options.create_id) {
	    if (json_class != oj_default_options.create_id) {
		xfree((char*)oj_default_options.create_id);
	    }
	    oj_default_options.create_id = 0;
	}
	v = rb_hash_lookup(opts, create_id_sym);
	if (Qnil != v) {
	    size_t	len = RSTRING_LEN(v) + 1;

	    oj_default_options.create_id = ALLOC_N(char, len);
	    strcpy((char*)oj_default_options.create_id, StringValuePtr(v));
	}
    }

    for (o = ynos; 0 != o->attr; o++) {
	if (Qtrue != rb_funcall(opts, rb_intern("has_key?"), 1, o->sym)) {
	    continue;
	}
	if (Qnil != (v = rb_hash_lookup(opts, o->sym))) {
	    if (Qtrue == v) {
		*o->attr = Yes;
	    } else if (Qfalse == v) {
		*o->attr = No;
	    } else {
		rb_raise(rb_eArgError, "%s must be true, false, or nil.", rb_id2name(SYM2ID(o->sym)));
	    }
	}
    }
    return Qnil;
}

static void
parse_options(VALUE ropts, Options copts) {
    struct _YesNoOpt	ynos[] = {
	{ circular_sym, &copts->circular },
	{ auto_define_sym, &copts->auto_define },
	{ symbol_keys_sym, &copts->sym_key },
	{ ascii_only_sym, &copts->ascii_only },
	{ bigdecimal_as_decimal_sym, &copts->bigdec_as_num },
	{ Qnil, 0 }
    };
    YesNoOpt	o;
    
    if (rb_cHash == rb_obj_class(ropts)) {
	VALUE	v;
	
	if (Qnil != (v = rb_hash_lookup(ropts, indent_sym))) {
	    if (rb_cFixnum != rb_obj_class(v)) {
		rb_raise(rb_eArgError, ":indent must be a Fixnum.");
	    }
	    copts->indent = NUM2INT(v);
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
	    } else if (xmlschema_sym == v) {
		copts->time_format = XmlTime;
	    } else if (ruby_sym == v) {
		copts->time_format = RubyTime;
	    } else {
		rb_raise(rb_eArgError, ":time_format must be :unix, :xmlschema, or :ruby.");
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
    }
 }

static VALUE
load_with_opts(VALUE input, Options copts) {
    char	*json = 0;
    size_t	len = 0;
    VALUE	obj;

    if (rb_type(input) == T_STRING) {
	// the json string gets modified so make a copy of it
	len = RSTRING_LEN(input) + 1;
	if (copts->max_stack < len) {
	    json = ALLOC_N(char, len);
	} else {
	    json = ALLOCA_N(char, len);
	}
	strcpy(json, StringValuePtr(input));
    } else {
	VALUE	clas = rb_obj_class(input);
	VALUE	s;

	if (oj_stringio_class == clas) {
	    s = rb_funcall2(input, oj_string_id, 0, 0);
	    len = RSTRING_LEN(s) + 1;
	    if (copts->max_stack < len) {
		json = ALLOC_N(char, len);
	    } else {
		json = ALLOCA_N(char, len);
	    }
	    strcpy(json, StringValuePtr(s));
#ifndef JRUBY_RUBY
#if !IS_WINDOWS
	    // JRuby gets confused with what is the real fileno.
	} else if (rb_respond_to(input, oj_fileno_id) && Qnil != (s = rb_funcall(input, oj_fileno_id, 0))) {
	    int		fd = FIX2INT(s);
	    ssize_t	cnt;

	    len = lseek(fd, 0, SEEK_END);
	    lseek(fd, 0, SEEK_SET);
	    if (copts->max_stack < len) {
		json = ALLOC_N(char, len + 1);
	    } else {
		json = ALLOCA_N(char, len + 1);
	    }
	    if (0 >= (cnt = read(fd, json, len)) || cnt != (ssize_t)len) {
		rb_raise(rb_eIOError, "failed to read from IO Object.");
	    }
	    json[len] = '\0';
#endif
#endif
	} else if (rb_respond_to(input, oj_read_id)) {
	    s = rb_funcall2(input, oj_read_id, 0, 0);
	    len = RSTRING_LEN(s) + 1;
	    if (copts->max_stack < len) {
		json = ALLOC_N(char, len);
	    } else {
		json = ALLOCA_N(char, len);
	    }
	    strcpy(json, StringValuePtr(s));
	} else {
	    rb_raise(rb_eArgError, "load() expected a String or IO Object.");
	}
    }
    obj = oj_parse(json, copts);
    if (copts->max_stack < len) {
	xfree(json);
    }
    return obj;
}

/* call-seq: load(json, options) => Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into a Hash, Array, String, Fixnum, Float,
 * true, false, or nil. Raises an exception if the JSON is malformed or the
 * classes specified are not valid.
 * @param [String] json JSON String
 * @param [Hash] options load options (same as default_options)
 */
static VALUE
load(int argc, VALUE *argv, VALUE self) {
    struct _Options	options = oj_default_options;

    if (1 > argc) {
	rb_raise(rb_eArgError, "Wrong number of arguments to load().");
    }
    if (2 <= argc) {
	parse_options(argv[1], &options);
    }
    return load_with_opts(*argv, &options);
}

/* Document-method: load_file
 *   call-seq: load_file(path, options) => Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document from a file into a Hash, Array, String, Fixnum,
 * Float, true, false, or nil. Raises an exception if the JSON is malformed or
 * the classes specified are not valid.
 *
 * @param [String] path path to a file containing a JSON document
 * @param [Hash] options load options (same as default_options)
 */
static VALUE
load_file(int argc, VALUE *argv, VALUE self) {
    char		*path;
    char		*json;
    FILE		*f;
    unsigned long	len;
    VALUE		obj;
    struct _Options	options = oj_default_options;
    size_t		max_stack = oj_default_options.max_stack;

    Check_Type(*argv, T_STRING);
    path = StringValuePtr(*argv);
    if (0 == (f = fopen(path, "r"))) {
	rb_raise(rb_eIOError, "%s", strerror(errno));
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    if (max_stack < len) {
	json = ALLOC_N(char, len + 1);
    } else {
	json = ALLOCA_N(char, len + 1);
    }
    fseek(f, 0, SEEK_SET);
    if (len != fread(json, 1, len, f)) {
	fclose(f);
	rb_raise(rb_const_get_at(Oj, rb_intern("LoadError")), "Failed to read %ld bytes from %s.", len, path);
    }
    fclose(f);
    json[len] = '\0';
    if (2 <= argc) {
	parse_options(argv[1], &options);
    }
    obj = oj_parse(json, &options);
    if (max_stack < len) {
	xfree(json);
    }
    return obj;
}

/* call-seq: strict_load(doc)
 *
 * Loads a JSON document in strict mode with auto_define and symbol_keys turned off. This function should be safe to use
 * with JSON received on an unprotected public interface.
 * @param [String|IO] doc JSON String or IO to load
 * @return [Hash|Array|String|Fixnum|Bignum|BigDecimal|nil|True|False]
 */
static VALUE
strict_load(VALUE self, VALUE doc) {
    struct _Options	options = oj_default_options;

    options.auto_define = No;
    options.sym_key = No;
    options.mode = StrictMode;

    return load_with_opts(doc, &options);
}

/* call-seq: dump(obj, options) => json-string
 *
 * Dumps an Object (obj) to a string.
 * @param [Object] obj Object to serialize as an JSON document String
 * @param [Hash] options same as default_options
 */
static VALUE
dump(int argc, VALUE *argv, VALUE self) {
    char		*json;
    struct _Options	copts = oj_default_options;
    VALUE		rstr;
    
    if (2 == argc) {
	parse_options(argv[1], &copts);
    }
    if (0 == (json = oj_write_obj_to_str(*argv, &copts))) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(json);
#if HAS_ENCODING_SUPPORT
    rb_enc_associate(rstr, oj_utf8_encoding);
#endif
    xfree(json);

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
	parse_options(argv[2], &copts);
    }
    Check_Type(*argv, T_STRING);
    oj_write_obj_to_file(argv[1], StringValuePtr(*argv), &copts);

    return Qnil;
}

/* call-seq: saj_parse(handler, io)
 *
 * Parses an IO stream or file containing an JSON document. Raises an exception
 * if the JSON is malformed.
 * @param [Oj::Saj] handler SAJ (responds to Oj::Saj methods) like handler
 * @param [IO|String] io IO Object to read from
 */
static VALUE
saj_parse(int argc, VALUE *argv, VALUE self) {
    struct _Options	copts = oj_default_options;
    char		*json = 0;
    size_t		len = 0;
    VALUE		input = argv[1];

    if (argc < 2) {
	rb_raise(rb_eArgError, "Wrong number of arguments to saj_parse.\n");
    }
    if (rb_type(input) == T_STRING) {
	// the json string gets modified so make a copy of it
	len = RSTRING_LEN(input) + 1;
	if (copts.max_stack < len) {
	    json = ALLOC_N(char, len);
	} else {
	    json = ALLOCA_N(char, len);
	}
	strcpy(json, StringValuePtr(input));
    } else {
	VALUE	clas = rb_obj_class(input);
	VALUE	s;

	if (oj_stringio_class == clas) {
	    s = rb_funcall2(input, oj_string_id, 0, 0);
	    len = RSTRING_LEN(s) + 1;
	    if (copts.max_stack < len) {
		json = ALLOC_N(char, len);
	    } else {
		json = ALLOCA_N(char, len);
	    }
	    strcpy(json, StringValuePtr(s));
#ifndef JRUBY_RUBY
#if !IS_WINDOWS
	    // JRuby gets confused with what is the real fileno.
	} else if (rb_respond_to(input, oj_fileno_id) && Qnil != (s = rb_funcall(input, oj_fileno_id, 0))) {
	    int		fd = FIX2INT(s);
	    ssize_t	cnt;

	    len = lseek(fd, 0, SEEK_END);
	    lseek(fd, 0, SEEK_SET);
	    if (copts.max_stack < len) {
		json = ALLOC_N(char, len + 1);
	    } else {
		json = ALLOCA_N(char, len + 1);
	    }
	    if (0 >= (cnt = read(fd, json, len)) || cnt != (ssize_t)len) {
		rb_raise(rb_eIOError, "failed to read from IO Object.");
	    }
	    json[len] = '\0';
#endif
#endif
	} else if (rb_respond_to(input, oj_read_id)) {
	    s = rb_funcall2(input, oj_read_id, 0, 0);
	    len = RSTRING_LEN(s) + 1;
	    if (copts.max_stack < len) {
		json = ALLOC_N(char, len);
	    } else {
		json = ALLOCA_N(char, len);
	    }
	    strcpy(json, StringValuePtr(s));
	} else {
	    rb_raise(rb_eArgError, "saj_parse() expected a String or IO Object.");
	}
    }
    oj_saj_parse(*argv, json);
    if (copts.max_stack < len) {
	xfree(json);
    }
    return Qnil;
}

// Mimic JSON section

static VALUE
mimic_dump(int argc, VALUE *argv, VALUE self) {
    char		*json;
    struct _Options	copts = oj_default_options;
    VALUE		rstr;
    
    if (0 == (json = oj_write_obj_to_str(*argv, &copts))) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(json);
#if HAS_ENCODING_SUPPORT
    rb_enc_associate(rstr, oj_utf8_encoding);
#endif
    if (2 <= argc && Qnil != argv[1]) {
	VALUE	io = argv[1];
	VALUE	args[1];

	*args = rstr;
	rb_funcall2(io, oj_write_id, 1, args);
	rstr = io;
    }
    xfree(json);

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
	    VALUE	*np = RARRAY_PTR(obj);
	    size_t	cnt = RARRAY_LEN(obj);
	
	    for (; 0 < cnt; cnt--, np++) {
		mimic_walk(Qnil, *np, proc);
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
    VALUE	obj = load(1, argv, self);
    VALUE	p = Qnil;

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
    char		*json;
    VALUE		rstr;
    
    if (2 == argc && Qnil != argv[1]) {
	struct _DumpOpts	dump_opts;
	VALUE			ropts = argv[1];
	VALUE			v;

	memset(&dump_opts, 0, sizeof(dump_opts)); // may not be needed
	if (T_HASH != rb_type(ropts)) {
	    rb_raise(rb_eArgError, "options must be a hash.");
	}
	if (Qnil != (v = rb_hash_lookup(ropts, indent_sym))) {
	    rb_check_type(v, T_STRING);
	    if (0 == copts->dump_opts) {
		copts->dump_opts = &dump_opts;
	    }
	    copts->dump_opts->indent = StringValuePtr(v);
	    copts->dump_opts->indent_size = (uint8_t)strlen(copts->dump_opts->indent);
	}
	if (Qnil != (v = rb_hash_lookup(ropts, space_sym))) {
	    rb_check_type(v, T_STRING);
	    if (0 == copts->dump_opts) {
		copts->dump_opts = &dump_opts;
	    }
	    copts->dump_opts->after_sep = StringValuePtr(v);
	    copts->dump_opts->after_size = (uint8_t)strlen(copts->dump_opts->after_sep);
	}
	if (Qnil != (v = rb_hash_lookup(ropts, space_before_sym))) {
	    rb_check_type(v, T_STRING);
	    if (0 == copts->dump_opts) {
		copts->dump_opts = &dump_opts;
	    }
	    copts->dump_opts->before_sep = StringValuePtr(v);
	    copts->dump_opts->before_size = (uint8_t)strlen(copts->dump_opts->before_sep);
	}
	if (Qnil != (v = rb_hash_lookup(ropts, object_nl_sym))) {
	    rb_check_type(v, T_STRING);
	    if (0 == copts->dump_opts) {
		copts->dump_opts = &dump_opts;
	    }
	    copts->dump_opts->hash_nl = StringValuePtr(v);
	    copts->dump_opts->hash_size = (uint8_t)strlen(copts->dump_opts->hash_nl);
	}
	if (Qnil != (v = rb_hash_lookup(ropts, array_nl_sym))) {
	    rb_check_type(v, T_STRING);
	    if (0 == copts->dump_opts) {
		copts->dump_opts = &dump_opts;
	    }
	    copts->dump_opts->array_nl = StringValuePtr(v);
	    copts->dump_opts->array_size = (uint8_t)strlen(copts->dump_opts->array_nl);
	}
	// :allow_nan is not supported as Oj always allows_nan
	// :max_nesting is always set to 100
    }
    if (0 == (json = oj_write_obj_to_str(*argv, copts))) {
	rb_raise(rb_eNoMemError, "Not enough memory.");
    }
    rstr = rb_str_new2(json);
#if HAS_ENCODING_SUPPORT
    rb_enc_associate(rstr, oj_utf8_encoding);
#endif
    xfree(json);

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
    struct _DumpOpts	dump_opts;
    
    dump_opts.indent = "  ";
    dump_opts.indent_size = (uint8_t)strlen(dump_opts.indent);
    dump_opts.before_sep = " ";
    dump_opts.before_size = (uint8_t)strlen(dump_opts.before_sep);
    dump_opts.after_sep = " ";
    dump_opts.after_size = (uint8_t)strlen(dump_opts.after_sep);
    dump_opts.hash_nl = "\n";
    dump_opts.hash_size = (uint8_t)strlen(dump_opts.hash_nl);
    dump_opts.array_nl = "\n";
    dump_opts.array_size = (uint8_t)strlen(dump_opts.array_nl);
    copts.dump_opts = &dump_opts;

    return mimic_generate_core(argc, argv, &copts);
}

static VALUE
mimic_parse(int argc, VALUE *argv, VALUE self) {
    struct _Options	options = oj_default_options;

    if (1 > argc) {
	rb_raise(rb_eArgError, "Wrong number of arguments to parse().");
    }
    if (2 <= argc && Qnil != argv[1]) {
	VALUE	ropts = argv[1];
	VALUE	v;

	if (T_HASH != rb_type(ropts)) {
	    rb_raise(rb_eArgError, "options must be a hash.");
	}
	if (Qnil != (v = rb_hash_lookup(ropts, symbolize_names_sym))) {
	    options.sym_key = (Qtrue == v) ? Yes : No;
	}
	if (Qnil != (v = rb_hash_lookup(ropts,	create_additions_sym))) {
	    options.mode = (Qtrue == v) ? CompatMode : StrictMode;
	}
	// :allow_nan is not supported as Oj always allows nan
	// :max_nesting is always set to 100
	// :object_class is always Hash
	// :array_class is always Array
    }
    return load_with_opts(*argv, &options);
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
    }
    if (Qnil != id) {
	size_t	len = RSTRING_LEN(id) + 1;

	oj_default_options.create_id = ALLOC_N(char, len);
	strcpy((char*)oj_default_options.create_id, StringValuePtr(id));
    }
    return id;
}

/* Document-method: mimic_JSON
 *    call-seq: mimic_JSON() => Module
 *
 * Creates the JSON module with methods and classes to mimic the JSON
 * gem. After this method is invoked calls that expect the JSON module will
 * use Oj instead and be faster than the original JSON. Most options that
 * could be passed to the JSON methods are supported. The calls to set parser
 * or generator will not raise an Exception but will not have any effect.
 */
static VALUE
define_mimic_json(int argc, VALUE *argv, VALUE self) {
    if (Qnil == mimic) {
	VALUE	ext;
	VALUE	dummy;

	if (rb_const_defined_at(rb_cObject, rb_intern("JSON"))) {
	    rb_raise(rb_const_get_at(Oj, rb_intern("MimicError")),
		     "JSON module already exists. Can not mimic. Do not require 'json' before calling mimic_JSON.");
	}
	mimic = rb_define_module("JSON");
	ext = rb_define_module_under(mimic, "Ext");
	dummy = rb_define_class_under(ext, "Parser", rb_cObject);
	dummy = rb_define_class_under(ext, "Generator", rb_cObject);
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

	array_nl_sym = ID2SYM(rb_intern("array_nl"));			rb_gc_register_address(&array_nl_sym);
	create_additions_sym = ID2SYM(rb_intern("create_additions"));	rb_gc_register_address(&create_additions_sym);
	object_nl_sym = ID2SYM(rb_intern("object_nl"));			rb_gc_register_address(&object_nl_sym);
	space_before_sym = ID2SYM(rb_intern("space_before"));		rb_gc_register_address(&space_before_sym);
	space_sym = ID2SYM(rb_intern("space"));				rb_gc_register_address(&space_sym);
	symbolize_names_sym = ID2SYM(rb_intern("symbolize_names"));	rb_gc_register_address(&symbolize_names_sym);

	oj_default_options.mode = CompatMode;
	oj_default_options.ascii_only = Yes;
    }
    return mimic;
}

void Init_oj() {
    Odd	odd;
    ID	*idp;

    Oj = rb_define_module("Oj");

    rb_require("time");
    rb_require("date");
    rb_require("bigdecimal");
    rb_require("stringio");

    rb_define_module_function(Oj, "default_options", get_def_opts, 0);
    rb_define_module_function(Oj, "default_options=", set_def_opts, 1);

    rb_define_module_function(Oj, "mimic_JSON", define_mimic_json, -1);
    rb_define_module_function(Oj, "load", load, -1);
    rb_define_module_function(Oj, "load_file", load_file, -1);
    rb_define_module_function(Oj, "strict_load", strict_load, 1);
    rb_define_module_function(Oj, "dump", dump, -1);
    rb_define_module_function(Oj, "to_file", to_file, -1);

    rb_define_module_function(Oj, "saj_parse", saj_parse, -1);

    oj_add_value_id = rb_intern("add_value");
    oj_array_end_id = rb_intern("array_end");
    oj_array_start_id = rb_intern("array_start");
    oj_as_json_id = rb_intern("as_json");
    oj_error_id = rb_intern("error");
    oj_fileno_id = rb_intern("fileno");
    oj_hash_end_id = rb_intern("hash_end");
    oj_hash_start_id = rb_intern("hash_start");
    oj_instance_variables_id = rb_intern("instance_variables");
    oj_json_create_id = rb_intern("json_create");
    oj_new_id = rb_intern("new");
    oj_read_id = rb_intern("read");
    oj_string_id = rb_intern("string");
    oj_to_hash_id = rb_intern("to_hash");
    oj_to_json_id = rb_intern("to_json");
    oj_to_s_id = rb_intern("to_s");
    oj_to_sym_id = rb_intern("to_sym");
    oj_to_time_id = rb_intern("to_time");
    oj_tv_nsec_id = rb_intern("tv_nsec");
    oj_tv_sec_id = rb_intern("tv_sec");
    oj_tv_usec_id = rb_intern("tv_usec");
    oj_utc_offset_id = rb_intern("utc_offset");
    oj_write_id = rb_intern("write");

    oj_bag_class = rb_const_get_at(Oj, rb_intern("Bag"));
    oj_bigdecimal_class = rb_const_get(rb_cObject, rb_intern("BigDecimal"));
    oj_date_class = rb_const_get(rb_cObject, rb_intern("Date"));
    oj_datetime_class = rb_const_get(rb_cObject, rb_intern("DateTime"));
    oj_parse_error_class = rb_const_get_at(Oj, rb_intern("ParseError"));
    oj_stringio_class = rb_const_get(rb_cObject, rb_intern("StringIO"));
    oj_struct_class = rb_const_get(rb_cObject, rb_intern("Struct"));
    oj_time_class = rb_const_get(rb_cObject, rb_intern("Time"));

    ascii_only_sym = ID2SYM(rb_intern("ascii_only"));	rb_gc_register_address(&ascii_only_sym);
    auto_define_sym = ID2SYM(rb_intern("auto_define"));	rb_gc_register_address(&auto_define_sym);
    bigdecimal_as_decimal_sym = ID2SYM(rb_intern("bigdecimal_as_decimal"));rb_gc_register_address(&bigdecimal_as_decimal_sym);
    circular_sym = ID2SYM(rb_intern("circular"));	rb_gc_register_address(&circular_sym);
    compat_sym = ID2SYM(rb_intern("compat"));		rb_gc_register_address(&compat_sym);
    create_id_sym = ID2SYM(rb_intern("create_id"));	rb_gc_register_address(&create_id_sym);
    indent_sym = ID2SYM(rb_intern("indent"));		rb_gc_register_address(&indent_sym);
    max_stack_sym = ID2SYM(rb_intern("max_stack"));	rb_gc_register_address(&max_stack_sym);
    mode_sym = ID2SYM(rb_intern("mode"));		rb_gc_register_address(&mode_sym);
    null_sym = ID2SYM(rb_intern("null"));		rb_gc_register_address(&null_sym);
    object_sym = ID2SYM(rb_intern("object"));		rb_gc_register_address(&object_sym);
    ruby_sym = ID2SYM(rb_intern("ruby"));		rb_gc_register_address(&ruby_sym);
    sec_prec_sym = ID2SYM(rb_intern("second_precision"));rb_gc_register_address(&sec_prec_sym);
    strict_sym = ID2SYM(rb_intern("strict"));		rb_gc_register_address(&strict_sym);
    symbol_keys_sym = ID2SYM(rb_intern("symbol_keys"));	rb_gc_register_address(&symbol_keys_sym);
    time_format_sym = ID2SYM(rb_intern("time_format"));	rb_gc_register_address(&time_format_sym);
    unix_sym = ID2SYM(rb_intern("unix"));		rb_gc_register_address(&unix_sym);
    xmlschema_sym = ID2SYM(rb_intern("xmlschema"));	rb_gc_register_address(&xmlschema_sym);

    oj_slash_string = rb_str_new2("/");			rb_gc_register_address(&oj_slash_string);

    oj_default_options.mode = ObjectMode;
#if HAS_ENCODING_SUPPORT
    oj_utf8_encoding = rb_enc_find("UTF-8");
#endif

    oj_cache_new(&oj_class_cache);
    oj_cache_new(&oj_attr_cache);

    odd = odds;
    // Rational
    idp = odd->attrs;
    odd->clas = rb_const_get(rb_cObject, rb_intern("Rational"));
    odd->create_obj = rb_cObject;
    odd->create_op = rb_intern("Rational");
    odd->attr_cnt = 2;
    *idp++ = rb_intern("numerator");
    *idp++ = rb_intern("denominator");
    *idp++ = 0;
    // Date
    odd++;
    idp = odd->attrs;
    odd->clas = oj_date_class;
    odd->create_obj = odd->clas;
    odd->create_op = oj_new_id;
    odd->attr_cnt = 4;
    *idp++ = rb_intern("year");
    *idp++ = rb_intern("month");
    *idp++ = rb_intern("day");
    *idp++ = rb_intern("start");
    *idp++ = 0;
    // DateTime
    odd++;
    idp = odd->attrs;
    odd->clas = oj_datetime_class;
    odd->create_obj = odd->clas;
    odd->create_op = oj_new_id;
    odd->attr_cnt = 8;
    *idp++ = rb_intern("year");
    *idp++ = rb_intern("month");
    *idp++ = rb_intern("day");
    *idp++ = rb_intern("hour");
    *idp++ = rb_intern("min");
    *idp++ = rb_intern("sec");
    *idp++ = rb_intern("offset");
    *idp++ = rb_intern("start");
    *idp++ = 0;
    // Range
    odd++;
    idp = odd->attrs;
    odd->clas = rb_const_get(rb_cObject, rb_intern("Range"));
    odd->create_obj = odd->clas;
    odd->create_op = oj_new_id;
    odd->attr_cnt = 3;
    *idp++ = rb_intern("begin");
    *idp++ = rb_intern("end");
    *idp++ = rb_intern("exclude_end?");
    *idp++ = 0;
    // The end. bump up the size of odds if a new class is added.
    odd++;
    odd->clas = Qundef;

#if SAFE_CACHE
    pthread_mutex_init(&oj_cache_mutex, 0);
#endif
    oj_init_doc();
}

void
_oj_raise_error(const char *msg, const char *json, const char *current, const char* file, int line) {
    int	jline = 1;
    int	col = 1;

    for (; json < current && '\n' != *current; current--) {
	col++;
    }
    for (; json < current; current--) {
	if ('\n' == *current) {
	    jline++;
	}
    }
    rb_raise(oj_parse_error_class, "%s at line %d, column %d [%s:%d]", msg, jline, col, file, line);
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
