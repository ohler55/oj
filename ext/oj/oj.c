/* oj.c
 * Copyright (c) 2011, Peter Ohler
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

#include "ruby.h"
#include "oj.h"

typedef struct _YesNoOpt {
    VALUE       sym;
    char        *attr;
} *YesNoOpt;

void Init_oj();

VALUE    Oj = Qnil;

ID	oj_instance_variables_id;
ID	oj_to_hash_id;
ID	oj_to_json_id;
ID	oj_tv_sec_id;
ID	oj_tv_usec_id;

static VALUE	circular_sym;
static VALUE	compat_sym;
static VALUE	encoding_sym;
static VALUE	indent_sym;
static VALUE	mode_sym;
static VALUE	null_sym;
static VALUE	object_sym;
static VALUE	strict_sym;

static struct _Options  default_options = {
    { '\0' },		// encoding
    0,			// indent
    No,			// circular
    ObjectMode,		// mode
};

/* call-seq: default_options() => Hash
 *
 * Returns the default load and dump options as a Hash. The options are
 * - indent: [Fixnum] number of spaces to indent each element in an XML document
 * - encoding: [String] character encoding for the JSON file
 * - circular: [true|false|nil] support circular references while dumping
 * - mode: [:object|:strict|:compat|:null] load and dump modes to use for JSON
 * @return [Hash] all current option settings.
 */
static VALUE
get_def_opts(VALUE self) {
    VALUE       opts = rb_hash_new();
    int         elen = (int)strlen(default_options.encoding);

    rb_hash_aset(opts, encoding_sym, (0 == elen) ? Qnil : rb_str_new(default_options.encoding, elen));
    rb_hash_aset(opts, indent_sym, INT2FIX(default_options.indent));
    rb_hash_aset(opts, circular_sym, (Yes == default_options.circular) ? Qtrue : ((No == default_options.circular) ? Qfalse : Qnil));
    switch (default_options.mode) {
    case StrictMode:	rb_hash_aset(opts, mode_sym, strict_sym);	break;
    case CompatMode:	rb_hash_aset(opts, mode_sym, compat_sym);	break;
    case NullMode:	rb_hash_aset(opts, mode_sym, null_sym);		break;
    case ObjectMode:
    default:            rb_hash_aset(opts, mode_sym, object_sym);	break;
    }
    return opts;
}

/* call-seq: default_options=(opts)
 *
 * Sets the default options for load and dump.
 * @param [Hash] opts options to change
 * @param [Fixnum] :indent number of spaces to indent each element in an XML document
 * @param [String] :encoding character encoding for the JSON file
 * @param [true|false|nil] :circular support circular references while dumping
 * @parsm [:object|:strict|:compat|:null] load and dump mode to use for JSON
 *        :strict raises an exception when a non-supported Object is
 *        encountered. :compat attempts to extract variable values from an
 *        Object using to_json() or to_hash() then it walks the Object's
 *        variables if neither is found. The :object mode ignores to_hash()
 *        and to_json() methods and encodes variables using code internal to
 *        the Oj gem. The :null mode ignores non-supported Objects and
 *        replaces them with a null.  @return [nil]
 */
static VALUE
set_def_opts(VALUE self, VALUE opts) {
    struct _YesNoOpt    ynos[] = {
        { circular_sym, &default_options.circular },
        { Qnil, 0 }
    };
    YesNoOpt    o;
    VALUE       v;
    
    Check_Type(opts, T_HASH);

    v = rb_hash_lookup2(opts, encoding_sym, Qundef);
    if (Qundef == v) {
	// no change
    } else if (Qnil == v) {
        *default_options.encoding = '\0';
    } else {
        Check_Type(v, T_STRING);
        strncpy(default_options.encoding, StringValuePtr(v), sizeof(default_options.encoding) - 1);
    }

    v = rb_hash_aref(opts, indent_sym);
    if (Qnil != v) {
        Check_Type(v, T_FIXNUM);
        default_options.indent = FIX2INT(v);
    }

    v = rb_hash_lookup2(opts, mode_sym, Qundef);
    if (Qundef == v || Qnil == v) {
	// ignore
    } else if (object_sym == v) {
        default_options.mode = ObjectMode;
    } else if (strict_sym == v) {
        default_options.mode = StrictMode;
    } else if (compat_sym == v) {
        default_options.mode = CompatMode;
    } else if (null_sym == v) {
        default_options.mode = NullMode;
    } else {
        rb_raise(rb_eArgError, ":mode must be :object, :strict, :compat, or :null.\n");
    }

    for (o = ynos; 0 != o->attr; o++) {
        v = rb_hash_lookup2(opts, o->sym, Qundef);
	if (Qundef == v) {
	    // no change
	} else if (Qnil == v) {
            *o->attr = NotSet;
        } else if (Qtrue == v) {
            *o->attr = Yes;
        } else if (Qfalse == v) {
            *o->attr = No;
        } else {
            rb_raise(rb_eArgError, "%s must be true, false, or nil.\n", StringValuePtr(o->sym));
        }
    }
    return Qnil;
}

static void
parse_options(VALUE ropts, Options copts) {
    struct _YesNoOpt    ynos[] = {
        { circular_sym, &copts->circular },
        { Qnil, 0 }
    };
    YesNoOpt    o;
    
    if (rb_cHash == rb_obj_class(ropts)) {
        VALUE   v;
        
        if (Qnil != (v = rb_hash_lookup(ropts, indent_sym))) {
            if (rb_cFixnum != rb_obj_class(v)) {
                rb_raise(rb_eArgError, ":indent must be a Fixnum.\n");
            }
            copts->indent = NUM2INT(v);
        }
        if (Qnil != (v = rb_hash_lookup(ropts, encoding_sym))) {
            if (rb_cString != rb_obj_class(v)) {
                rb_raise(rb_eArgError, ":encoding must be a String.\n");
            }
            strncpy(copts->encoding, StringValuePtr(v), sizeof(copts->encoding) - 1);
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
                rb_raise(rb_eArgError, ":mode must be :object, :strict, :compat, or :null.\n");
            }
        }
        for (o = ynos; 0 != o->attr; o++) {
            if (Qnil != (v = rb_hash_lookup(ropts, o->sym))) {
                VALUE       c = rb_obj_class(v);

                if (rb_cTrueClass == c) {
                    *o->attr = Yes;
                } else if (rb_cFalseClass == c) {
                    *o->attr = No;
                } else {
                    rb_raise(rb_eArgError, "%s must be true or false.\n", StringValuePtr(o->sym));
                }
            }
        }
    }
 }

static VALUE
load(char *json, int argc, VALUE *argv, VALUE self) {
    VALUE		obj;
    struct _Options	options = default_options;

    if (1 == argc) {
	parse_options(*argv, &options);
    }
    obj = oj_parse(json, &options);
    free(json);

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
load_str(int argc, VALUE *argv, VALUE self) {
    char        *json;
    
    Check_Type(*argv, T_STRING);
    // the json string gets modified so make a copy of it
    json = strdup(StringValuePtr(*argv));

    return load(json, argc - 1, argv + 1, self);
}

static VALUE
load_file(int argc, VALUE *argv, VALUE self) {
    char                *path;
    char                *json;
    FILE                *f;
    unsigned long       len;
    
    Check_Type(*argv, T_STRING);
    path = StringValuePtr(*argv);
    if (0 == (f = fopen(path, "r"))) {
        rb_raise(rb_eIOError, "%s\n", strerror(errno));
    }
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    if (0 == (json = malloc(len + 1))) {
        fclose(f);
        rb_raise(rb_eNoMemError, "Could not allocate memory for %ld byte file.\n", len);
    }
    fseek(f, 0, SEEK_SET);
    if (len != fread(json, 1, len, f)) {
        fclose(f);
        rb_raise(rb_eLoadError, "Failed to read %ld bytes from %s.\n", len, path);
    }
    fclose(f);
    json[len] = '\0';

    return load(json, argc - 1, argv + 1, self);
}

/* call-seq: dump(obj, options) => json-string
 *
 * Dumps an Object (obj) to a string.
 * @param [Object] obj Object to serialize as an JSON document String
 * @param [Hash] options same as default_options
 */
static VALUE
dump(int argc, VALUE *argv, VALUE self) {
    char                *json;
    struct _Options     copts = default_options;
    VALUE               rstr;
    
    if (2 == argc) {
        parse_options(argv[1], &copts);
    }
    if (0 == (json = oj_write_obj_to_str(*argv, &copts))) {
        rb_raise(rb_eNoMemError, "Not enough memory.\n");
    }
    rstr = rb_str_new2(json);
#ifdef ENCODING_INLINE_MAX
    if ('\0' != *copts.encoding) {
        rb_enc_associate(rstr, rb_enc_find(copts.encoding));
    }
#endif
    free(json);

    return rstr;
}

void Init_oj() {
    VALUE       keep = Qnil;

    Oj = rb_define_module("Oj");
    keep = rb_cv_get(Oj, "@@keep"); // needed to stop GC from deleting and reusing VALUEs

    rb_define_module_function(Oj, "default_options", get_def_opts, 0);
    rb_define_module_function(Oj, "default_options=", set_def_opts, 1);

    rb_define_module_function(Oj, "load", load_str, -1);
    rb_define_module_function(Oj, "load_file", load_file, -1);
    rb_define_module_function(Oj, "dump", dump, -1);

    oj_instance_variables_id = rb_intern("instance_variables");
    oj_to_hash_id = rb_intern("to_hash");
    oj_to_json_id = rb_intern("to_json");
    oj_tv_sec_id = rb_intern("tv_sec");
    oj_tv_usec_id = rb_intern("tv_usec");
    
    circular_sym = ID2SYM(rb_intern("circular"));	rb_ary_push(keep, circular_sym);
    compat_sym = ID2SYM(rb_intern("compat"));		rb_ary_push(keep, compat_sym);
    encoding_sym = ID2SYM(rb_intern("encoding"));	rb_ary_push(keep, encoding_sym);
    indent_sym = ID2SYM(rb_intern("indent"));		rb_ary_push(keep, indent_sym);
    mode_sym = ID2SYM(rb_intern("mode"));		rb_ary_push(keep, mode_sym);
    null_sym = ID2SYM(rb_intern("null"));		rb_ary_push(keep, null_sym);
    object_sym = ID2SYM(rb_intern("object"));		rb_ary_push(keep, object_sym);
    strict_sym = ID2SYM(rb_intern("strict"));		rb_ary_push(keep, strict_sym);

    default_options.mode = ObjectMode;
}

void
_oj_raise_error(const char *msg, const char *xml, const char *current, const char* file, int line) {
    int         xline = 1;
    int         col = 1;

    for (; xml < current && '\n' != *current; current--) {
        col++;
    }
    for (; xml < current; current--) {
        if ('\n' == *current) {
            xline++;
        }
    }
    rb_raise(rb_eSyntaxError, "%s at line %d, column %d [%s:%d]\n", msg, xline, col, file, line);
}
