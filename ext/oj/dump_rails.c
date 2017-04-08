/* dump_object.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"
#include "encode.h"

#define OJ_INFINITY (1.0/0.0)

static bool	hash_opt = false;
static bool	array_opt = false;
static bool	object_opt = false;

static void
dump_nil(VALUE obj, int depth, Out out, bool as_ok) {
    if (0 == depth) {
	rb_raise(rb_eTypeError, "Not a supported type for encoding at depth of 0.");
    } else {
	assure_size(out, 4);
	*out->cur++ = 'n';
	*out->cur++ = 'u';
	*out->cur++ = 'l';
	*out->cur++ = 'l';
	*out->cur = '\0';
    }
}

static void
dump_as_json(VALUE obj, int depth, Out out) {
    volatile VALUE	ja;

    if (0 < out->argc) {
	ja = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
    } else {
	ja = rb_funcall(obj, oj_as_json_id, 0);
    }
    // Once as_json is call it should never be called again.
    oj_dump_rails_val(ja, depth, out, false);
}

static void
dump_float(VALUE obj, int depth, Out out, bool as_ok) {
    char	buf[64];
    char	*b;
    double	d = rb_num2dbl(obj);
    int		cnt = 0;

    if (0.0 == d) {
	b = buf;
	*b++ = '0';
	*b++ = '.';
	*b++ = '0';
	*b++ = '\0';
	cnt = 3;
    } else {
	if (isnan(d) || OJ_INFINITY == d || -OJ_INFINITY == d) {
	    rb_raise(rb_eTypeError, "NaN is not allowed rails mode.\n");
	} else if (d == (double)(long long int)d) {
	    cnt = snprintf(buf, sizeof(buf), "%.1f", d);
	} else {
	    cnt = snprintf(buf, sizeof(buf), "%0.16f", d);
	}
    }
    assure_size(out, cnt);
    for (b = buf; '\0' != *b; b++) {
	*out->cur++ = *b;
    }
    *out->cur = '\0';
}

static void
dump_array(VALUE a, int depth, Out out, bool as_ok) {
    size_t	size;
    int		i, cnt;
    int		d2 = depth + 1;

    if (Yes == out->opts->circular) {
	if (0 > oj_check_circular(a, out)) {
	    oj_dump_nil(Qnil, 0, out, false);
	    return;
	}
    }
    if (as_ok && !array_opt && rb_respond_to(a, oj_as_json_id)) {
	dump_as_json(a, depth, out);
	return;
    }
    cnt = (int)RARRAY_LEN(a);
    *out->cur++ = '[';
    size = 2;
    assure_size(out, size);
    if (0 == cnt) {
	*out->cur++ = ']';
    } else {
	if (out->opts->dump_opts.use) {
	    size = d2 * out->opts->dump_opts.indent_size + out->opts->dump_opts.array_size + 1;
	} else {
	    size = d2 * out->indent + 2;
	}
	cnt--;
	for (i = 0; i <= cnt; i++) {
	    assure_size(out, size);
	    if (out->opts->dump_opts.use) {
		if (0 < out->opts->dump_opts.array_size) {
		    strcpy(out->cur, out->opts->dump_opts.array_nl);
		    out->cur += out->opts->dump_opts.array_size;
		}
		if (0 < out->opts->dump_opts.indent_size) {
		    int	i;
		    for (i = d2; 0 < i; i--) {
			strcpy(out->cur, out->opts->dump_opts.indent_str);
			out->cur += out->opts->dump_opts.indent_size;
		    }
		}
	    } else {
		fill_indent(out, d2);
	    }
	    oj_dump_rails_val(rb_ary_entry(a, i), d2, out, as_ok);
	    if (i < cnt) {
		*out->cur++ = ',';
	    }
	}
	size = depth * out->indent + 1;
	assure_size(out, size);
	if (out->opts->dump_opts.use) {
	    if (0 < out->opts->dump_opts.array_size) {
		strcpy(out->cur, out->opts->dump_opts.array_nl);
		out->cur += out->opts->dump_opts.array_size;
	    }
	    if (0 < out->opts->dump_opts.indent_size) {
		int	i;

		for (i = depth; 0 < i; i--) {
		    strcpy(out->cur, out->opts->dump_opts.indent_str);
		    out->cur += out->opts->dump_opts.indent_size;
		}
	    }
	} else {
	    fill_indent(out, depth);
	}
	*out->cur++ = ']';
    }
    *out->cur = '\0';
}

static int
hash_cb(VALUE key, VALUE value, Out out) {
    int		depth = out->depth;
    long	size;
    int		rtype = rb_type(key);
    
    if (rtype != T_STRING && rtype != T_SYMBOL) {
	// TBD if stringify option then convert to string
	rb_raise(rb_eTypeError, "In :rails mode all Hash keys must be Strings or Symbols, not %s.\n", rb_obj_classname(key));
    }
    if (!out->opts->dump_opts.use) {
	size = depth * out->indent + 1;
	assure_size(out, size);
	fill_indent(out, depth);
	if (rtype == T_STRING) {
	    oj_dump_str(key, 0, out, false);
	} else {
	    oj_dump_sym(key, 0, out, false);
	}
	*out->cur++ = ':';
    } else {
	size = depth * out->opts->dump_opts.indent_size + out->opts->dump_opts.hash_size + 1;
	assure_size(out, size);
	if (0 < out->opts->dump_opts.hash_size) {
	    strcpy(out->cur, out->opts->dump_opts.hash_nl);
	    out->cur += out->opts->dump_opts.hash_size;
	}
	if (0 < out->opts->dump_opts.indent_size) {
	    int	i;
	    for (i = depth; 0 < i; i--) {
		strcpy(out->cur, out->opts->dump_opts.indent_str);
		out->cur += out->opts->dump_opts.indent_size;
	    }
	}
	if (rtype == T_STRING) {
	    oj_dump_str(key, 0, out, false);
	} else {
	    oj_dump_sym(key, 0, out, false);
	}
	size = out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2;
	assure_size(out, size);
	if (0 < out->opts->dump_opts.before_size) {
	    strcpy(out->cur, out->opts->dump_opts.before_sep);
	    out->cur += out->opts->dump_opts.before_size;
	}
	*out->cur++ = ':';
	if (0 < out->opts->dump_opts.after_size) {
	    strcpy(out->cur, out->opts->dump_opts.after_sep);
	    out->cur += out->opts->dump_opts.after_size;
	}
    }
    oj_dump_strict_val(value, depth, out);
    out->depth = depth;
    *out->cur++ = ',';

    return ST_CONTINUE;
}

static void
dump_hash(VALUE obj, int depth, Out out, bool as_ok) {
    int		cnt;
    size_t	size;

    if (Yes == out->opts->circular) {
	if (0 > oj_check_circular(obj, out)) {
	    oj_dump_nil(Qnil, 0, out, false);
	    return;
	}
    }
    if (as_ok && !hash_opt && rb_respond_to(obj, oj_as_json_id)) {
	dump_as_json(obj, depth, out);
	return;
    }
    cnt = (int)RHASH_SIZE(obj);
    size = depth * out->indent + 2;
    assure_size(out, 2);
    *out->cur++ = '{';
    if (0 == cnt) {
	*out->cur++ = '}';
    } else {
	out->depth = depth + 1;
	rb_hash_foreach(obj, hash_cb, (VALUE)out);
	if (',' == *(out->cur - 1)) {
	    out->cur--; // backup to overwrite last comma
	}
	if (!out->opts->dump_opts.use) {
	    assure_size(out, size);
	    fill_indent(out, depth);
	} else {
	    size = depth * out->opts->dump_opts.indent_size + out->opts->dump_opts.hash_size + 1;
	    assure_size(out, size);
	    if (0 < out->opts->dump_opts.hash_size) {
		strcpy(out->cur, out->opts->dump_opts.hash_nl);
		out->cur += out->opts->dump_opts.hash_size;
	    }
	    if (0 < out->opts->dump_opts.indent_size) {
		int	i;

		for (i = depth; 0 < i; i--) {
		    strcpy(out->cur, out->opts->dump_opts.indent_str);
		    out->cur += out->opts->dump_opts.indent_size;
		}
	    }
	}
	*out->cur++ = '}';
    }
    *out->cur = '\0';
}

static void
dump_obj(VALUE obj, int depth, Out out, bool as_ok) {
    if (as_ok) {
	if (object_opt) {
	    // TBD print out all vars
	} else if (false) {
	    // TBD check optimized active classes
	} else if (rb_respond_to(obj, oj_as_json_id)) {
	    dump_as_json(obj, depth, out);
	} else {
	    oj_dump_obj_to_s(obj, out);
	}
    } else {
	oj_dump_obj_to_s(obj, out);
    }
}


static void
dump_regexp(VALUE obj, int depth, Out out, bool as_ok) {
    oj_dump_obj_to_s(obj, out);
}

static void
dump_complex(VALUE obj, int depth, Out out, bool as_ok) {
    rb_raise(rb_eSysStackError, "Follow rails lead.");
    //oj_dump_obj_to_s(obj, out);
}

static void
dump_rational(VALUE obj, int depth, Out out, bool as_ok) {
    rb_raise(rb_eSysStackError, "Follow rails lead.");
    //oj_dump_obj_to_s(obj, out);
}

static DumpFunc	rails_funcs[] = {
    NULL,	 	// RUBY_T_NONE     = 0x00,
    dump_obj,		// RUBY_T_OBJECT   = 0x01,
    oj_dump_class, 	// RUBY_T_CLASS    = 0x02,
    oj_dump_class,	// RUBY_T_MODULE   = 0x03,
    dump_float, 	// RUBY_T_FLOAT    = 0x04,
    oj_dump_str, 	// RUBY_T_STRING   = 0x05,
    dump_regexp,	// RUBY_T_REGEXP   = 0x06,
    dump_array,		// RUBY_T_ARRAY    = 0x07,
    dump_hash,	 	// RUBY_T_HASH     = 0x08,
    dump_obj,		// RUBY_T_STRUCT   = 0x09,
    oj_dump_bignum,	// RUBY_T_BIGNUM   = 0x0a,
    NULL, 		// RUBY_T_FILE     = 0x0b,
    dump_obj,		// RUBY_T_DATA     = 0x0c,
    NULL, 		// RUBY_T_MATCH    = 0x0d,
    dump_complex, 	// RUBY_T_COMPLEX  = 0x0e,
    dump_rational, 	// RUBY_T_RATIONAL = 0x0f,
    NULL, 		// 0x10
    dump_nil,	 	// RUBY_T_NIL      = 0x11,
    oj_dump_true, 	// RUBY_T_TRUE     = 0x12,
    oj_dump_false,	// RUBY_T_FALSE    = 0x13,
    oj_dump_sym,	// RUBY_T_SYMBOL   = 0x14,
    oj_dump_fixnum,	// RUBY_T_FIXNUM   = 0x15,
};

void
oj_dump_rails_val(VALUE obj, int depth, Out out, bool as_ok) {
    int	type = rb_type(obj);
    
    if (MAX_DEPTH < depth) {
	rb_raise(rb_eNoMemError, "Too deeply nested.\n");
    }
    if (0 < type && type <= RUBY_T_FIXNUM) {
	DumpFunc	f = rails_funcs[type];

	if (NULL != f) {
	    f(obj, depth, out, as_ok);
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
    // TBD if encoder initialized with args then pass them to as_json call
    out.argc = 0;
    out.argv = NULL;
    if (Yes == copts.circular) {
	oj_cache8_new(&out.circ_cache);
    }

    oj_dump_rails_val(*argv, 0, &out, true);

    if (0 < out.indent) {
	switch (*(out.cur - 1)) {
	case ']':
	case '}':
	    assure_size(&out, 2);
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
