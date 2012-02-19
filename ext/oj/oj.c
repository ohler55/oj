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

struct _Options  default_options = {
    { '\0' },		// encoding
    2,			// indent
    No,			// circular
    NoMode,		// mode
    TolerantEffort,	// effort
};

void Init_oj();

VALUE    Oj = Qnil;

VALUE   circular_sym;
VALUE   effort_sym;
VALUE   encoding_sym;
VALUE   indent_sym;
VALUE   lazy_sym;
VALUE   mode_sym;
VALUE   object_sym;
VALUE   simple_sym;
VALUE   strict_sym;
VALUE   tolerant_sym;


/* call-seq: default_options() => Hash
 *
 * Returns the default load and dump options as a Hash. The options are
 * - indent: [Fixnum] number of spaces to indent each element in an XML document
 * - encoding: [String] character encoding for the JSON file
 * - circular: [true|false|nil] support circular references while dumping
 * - mode: [:object|:simple|nil] load method to use for JSON
 * - effort: [:strict|:tolerant|:lazy_define] set the tolerance level for loading
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
    case ObjectMode:	rb_hash_aset(opts, mode_sym, object_sym);	break;
    case SimpleMode:	rb_hash_aset(opts, mode_sym, simple_sym);	break;
    case NoMode:
    default:            rb_hash_aset(opts, mode_sym, Qnil);             break;
    }
    switch (default_options.effort) {
    case StrictEffort:		rb_hash_aset(opts, effort_sym, strict_sym);	break;
    case TolerantEffort:	rb_hash_aset(opts, effort_sym, tolerant_sym);	break;
    case LazyEffort:		rb_hash_aset(opts, effort_sym, lazy_sym);	break;
    case NoEffort:
    default:			rb_hash_aset(opts, effort_sym, Qnil);		break;
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
 * @param [:object|:simple|nil] :mode load method to use for JSON
 * @param [:strict|:tolerant|:lazy] :effort set the tolerance level for loading
 * @return [nil]
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

    v = rb_hash_aref(opts, encoding_sym);
    if (Qnil == v) {
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

    v = rb_hash_aref(opts, mode_sym);
    if (Qnil == v) {
        default_options.mode = NoMode;
    } else if (object_sym == v) {
        default_options.mode = ObjectMode;
    } else if (simple_sym == v) {
        default_options.mode = SimpleMode;
    } else {
        rb_raise(rb_eArgError, ":mode must be :object, :simple, or nil.\n");
    }

    v = rb_hash_aref(opts, effort_sym);
    if (Qnil == v) {
        default_options.effort = NoEffort;
    } else if (strict_sym == v) {
        default_options.effort = StrictEffort;
    } else if (tolerant_sym == v) {
        default_options.effort = TolerantEffort;
    } else if (lazy_sym == v) {
        default_options.effort = LazyEffort;
    } else {
        rb_raise(rb_eArgError, ":effort must be :strict, :tolerant, :lazy, or nil.\n");
    }
    for (o = ynos; 0 != o->attr; o++) {
        v = rb_hash_lookup(opts, o->sym);
        if (Qnil == v) {
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
            } else if (simple_sym == v) {
                copts->mode = SimpleMode;
            } else {
                rb_raise(rb_eArgError, ":mode must be :object or :simple.\n");
            }
        }
        if (Qnil != (v = rb_hash_lookup(ropts, effort_sym))) {
            if (lazy_sym == v) {
                copts->effort = LazyEffort;
            } else if (tolerant_sym == v) {
                copts->effort = TolerantEffort;
            } else if (strict_sym == v) {
                copts->effort = StrictEffort;
            } else {
                rb_raise(rb_eArgError, ":effort must be :strict, :tolerant, or :lazy.\n");
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
    obj = parse(json, &options);
    free(json);

    return obj;
}

/* call-seq: load(xml, options) => Hash, Array, String, Fixnum, Float, true, false, or nil
 *
 * Parses a JSON document String into a Hash, Array, String, Fixnum, Float,
 * true, false, or nil Raises an exception if the JSON is * malformed or the
 * classes specified are not valid.
 * @param [String] json JSON String
 * @param [Hash] options load options
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

static VALUE
dump(int argc, VALUE *argv, VALUE self) {
    char                *json;
    struct _Options     copts = default_options;
    VALUE               rstr;
    
    if (2 == argc) {
        parse_options(argv[1], &copts);
    }
    if (0 == (json = write_obj_to_str(*argv, &copts))) {
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

    circular_sym = ID2SYM(rb_intern("circular"));	rb_ary_push(keep, circular_sym);
    effort_sym = ID2SYM(rb_intern("effort"));		rb_ary_push(keep, effort_sym);
    encoding_sym = ID2SYM(rb_intern("encoding"));	rb_ary_push(keep, encoding_sym);
    indent_sym = ID2SYM(rb_intern("indent"));		rb_ary_push(keep, indent_sym);
    lazy_sym = ID2SYM(rb_intern("lazy"));		rb_ary_push(keep, lazy_sym);
    mode_sym = ID2SYM(rb_intern("mode"));		rb_ary_push(keep, mode_sym);
    object_sym = ID2SYM(rb_intern("object"));		rb_ary_push(keep, object_sym);
    simple_sym = ID2SYM(rb_intern("simple"));		rb_ary_push(keep, simple_sym);
    strict_sym = ID2SYM(rb_intern("strict"));		rb_ary_push(keep, strict_sym);
    tolerant_sym = ID2SYM(rb_intern("tolerant"));	rb_ary_push(keep, tolerant_sym);
}

void
_raise_error(const char *msg, const char *xml, const char *current, const char* file, int line) {
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
