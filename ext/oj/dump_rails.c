/* dump_object.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"
#include "encode.h"

static void
dump_regexp(VALUE obj, int depth, Out out, bool as_ok) {
    oj_dump_obj_to_s(obj, out);
}

static void
dump_complex(VALUE obj, int depth, Out out, bool as_ok) {
    oj_dump_obj_to_s(obj, out);
}

static void
dump_rational(VALUE obj, int depth, Out out, bool as_ok) {
    oj_dump_obj_to_s(obj, out);
}

static DumpFunc	rails_funcs[] = {
    NULL,	 	// RUBY_T_NONE   = 0x00,
    NULL, //dump_obj,		// RUBY_T_OBJECT = 0x01,
    oj_dump_class, 	// RUBY_T_CLASS  = 0x02,
    oj_dump_class,	// RUBY_T_MODULE = 0x03,
    oj_dump_float, 	// RUBY_T_FLOAT  = 0x04,
    oj_dump_str, 	// RUBY_T_STRING = 0x05,
    dump_regexp,	// RUBY_T_REGEXP = 0x06,
    NULL, //dump_array,		// RUBY_T_ARRAY  = 0x07,
    NULL, //dump_hash,	 	// RUBY_T_HASH   = 0x08,
    NULL, //dump_struct,	// RUBY_T_STRUCT = 0x09,
    oj_dump_bignum,	// RUBY_T_BIGNUM = 0x0a,
    NULL, 		// RUBY_T_FILE   = 0x0b,
    NULL, //dump_data,		// RUBY_T_DATA   = 0x0c,
    NULL, 		// RUBY_T_MATCH  = 0x0d,
    dump_complex, 	// RUBY_T_COMPLEX  = 0x0e,
    dump_rational, 	// RUBY_T_RATIONAL = 0x0f,
    NULL, 		// 0x10
    oj_dump_nil, 	// RUBY_T_NIL    = 0x11,
    oj_dump_true, 	// RUBY_T_TRUE   = 0x12,
    oj_dump_false,	// RUBY_T_FALSE  = 0x13,
    oj_dump_sym,	// RUBY_T_SYMBOL = 0x14,
    oj_dump_fixnum,	// RUBY_T_FIXNUM = 0x15,
};

void
oj_dump_rails_val(VALUE obj, int depth, Out out) {
    int	type = rb_type(obj);
    
    if (MAX_DEPTH < depth) {
	rb_raise(rb_eNoMemError, "Too deeply nested.\n");
    }
    if (0 < type && type <= RUBY_T_FIXNUM) {
	DumpFunc	f = rails_funcs[type];

	if (NULL != f) {
	    f(obj, depth, out, true);
	    return;
	}
    }
    oj_dump_nil(Qnil, depth, out, false);
}

/* Document-module: Oj
 * @!method encode(obj, opts=nil)
 * 
 * Encode obj as a JSON String.
 * 
 * @param obj [Object|Hash|Array] object to convert to a JSON String
 * @param opts [Hash] options
 *
 * @return [String]
 */
VALUE
oj_rails_encode(int argc, VALUE *argv, VALUE self) {
    char		buf[4096];
    struct _Out		out;
    struct _Options	copts = oj_default_options;
    VALUE		rstr;

    if (1 > argc) {
	rb_raise(rb_eArgError, "wrong number of arguments (0 for 1).");
    }
    switch (rb_type(*argv)) {
    case T_OBJECT:
    case T_CLASS:
    case T_MODULE:
    case T_FLOAT:
    case T_STRING:
    case T_REGEXP:
    case T_ARRAY:
    case T_HASH:
    case T_STRUCT:
    case T_BIGNUM:
    case T_FILE:
    case T_DATA:
    case T_MATCH:
    case T_COMPLEX:
    case T_RATIONAL:
	//case T_NIL:
    case T_TRUE:
    case T_FALSE:
    case T_SYMBOL:
    case T_FIXNUM:
	break;
    default:
	rb_raise(rb_eTypeError, "Not a supported type for encoding.");
	break;
    }

    copts.str_rx.head = NULL;
    copts.str_rx.tail = NULL;
    copts.mode = RailsMode;
    out.buf = buf;
    out.end = buf + sizeof(buf) - 10;
    out.allocated = 0;
    out.omit_nil = copts.dump_opts.omit_nil;
    out.caller = 0;
    out.cur = out.buf;
    out.circ_cnt = 0;
    out.opts = &copts;
    out.hash_cnt = 0;
    out.indent = copts.indent;
    out.argc = argc;
    out.argv = argv;
    if (Yes == copts.circular) {
	oj_cache8_new(&out.circ_cache);
    }

    oj_dump_rails_val(*argv, 0, &out);

    if (0 < out.indent) {
	switch (*(out.cur - 1)) {
	case ']':
	case '}':
	    // TBD grow(out, 1);
	    *out.cur++ = '\n';
	default:
	    break;
	}
    }
    *out.cur = '\0';
    if (Yes == copts.circular) {
	oj_cache8_delete(out.circ_cache);
    }

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
