/* dump_object.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"
#include "encode.h"
#include "mimic_rails.h"

#define OJ_INFINITY (1.0/0.0)

bool	oj_rails_hash_opt = false;
bool	oj_rails_array_opt = false;

static void
dump_as_json(VALUE obj, int depth, Out out, bool as_ok) {
    volatile VALUE	ja;

    if (0 < out->argc) {
	ja = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
    } else {
	ja = rb_funcall(obj, oj_as_json_id, 0);
    }
    if (ja == obj || !as_ok) {
	// Once as_json is call it should never be called again on the same
	// object with as_ok.
	oj_dump_rails_val(ja, depth, out, false);
    } else {
	int	type = rb_type(ja);

	if (T_HASH == type || T_ARRAY == type) {
	    oj_dump_rails_val(ja, depth, out, false);
	} else {
	    oj_dump_rails_val(ja, depth, out, true);
	}
    }
}

static void
dump_to_hash(VALUE obj, int depth, Out out) {
    oj_dump_rails_val(rb_funcall(obj, oj_to_hash_id, 0), depth, out, false);
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
	    strcpy(buf, "null");
	    cnt = 4;
	} else if (d == (double)(long long int)d) {
	    cnt = snprintf(buf, sizeof(buf), "%.1f", d);
	} else {
	    cnt = snprintf(buf, sizeof(buf), "%0.16g", d);
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
    if (as_ok && !oj_rails_array_opt && rb_respond_to(a, oj_as_json_id)) {
	dump_as_json(a, depth, out, false);
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
	key = rb_funcall(key, oj_to_s_id, 0);
	rtype = rb_type(key);
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
    oj_dump_rails_val(value, depth, out, false);
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
    if (as_ok && !oj_rails_hash_opt && rb_respond_to(obj, oj_as_json_id)) {
	dump_as_json(obj, depth, out, false);
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
	ROpt	ro;
	
	if (NULL != (ro = oj_rails_get_opt(out->ropts, rb_obj_class(obj))) && ro->on) {
	    ro->dump(obj, depth, out, as_ok);
	} else if (rb_respond_to(obj, oj_as_json_id)) {
	    dump_as_json(obj, depth, out, true);
	} else if (rb_respond_to(obj, oj_to_hash_id)) {
	    dump_to_hash(obj, depth, out);
	} else {
	    oj_dump_obj_to_s(obj, out);
	}
    } else if (rb_respond_to(obj, oj_to_hash_id)) {
	// Always attempt to_hash.
	dump_to_hash(obj, depth, out);
    } else {
	oj_dump_obj_to_s(obj, out);
    }
}

static void
dump_as_string(VALUE obj, int depth, Out out, bool as_ok) {
    oj_dump_obj_to_s(obj, out);
}

static DumpFunc	rails_funcs[] = {
    NULL,	 	// RUBY_T_NONE     = 0x00,
    dump_obj,		// RUBY_T_OBJECT   = 0x01,
    oj_dump_class, 	// RUBY_T_CLASS    = 0x02,
    oj_dump_class,	// RUBY_T_MODULE   = 0x03,
    dump_float, 	// RUBY_T_FLOAT    = 0x04,
    oj_dump_str, 	// RUBY_T_STRING   = 0x05,
    dump_as_string,	// RUBY_T_REGEXP   = 0x06,
    dump_array,		// RUBY_T_ARRAY    = 0x07,
    dump_hash,	 	// RUBY_T_HASH     = 0x08,
    dump_obj,		// RUBY_T_STRUCT   = 0x09,
    oj_dump_bignum,	// RUBY_T_BIGNUM   = 0x0a,
    NULL, 		// RUBY_T_FILE     = 0x0b,
    dump_obj,		// RUBY_T_DATA     = 0x0c,
    NULL, 		// RUBY_T_MATCH    = 0x0d,
    // Rails raises a stack error on Complex and Rational. It also corrupts
    // something whic causes a segfault on the next call. Oj will not mimic
    // that behavior.
    dump_as_string, 	// RUBY_T_COMPLEX  = 0x0e,
    dump_as_string, 	// RUBY_T_RATIONAL = 0x0f,
    NULL, 		// 0x10
    oj_dump_nil, 	// RUBY_T_NIL      = 0x11,
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
