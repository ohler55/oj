/* dump_object.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"

// Workaround in case INFINITY is not defined in math.h or if the OS is CentOS
#define OJ_INFINITY (1.0/0.0)

typedef void	(*AltFunc)(VALUE obj, int depth, Out out);

typedef struct _Alt {
    const char	*name;
    VALUE	clas;
    AltFunc	func;
    bool	active;
} *Alt;

typedef struct _Attr {
    const char	*name;
    int		len;
    VALUE	value;
} *Attr;

static void
raise_gen_err(const char *msg) {
    volatile VALUE	json_module;
    volatile VALUE	gen_err_class;
    volatile VALUE	json_error_class;

    if (rb_const_defined_at(rb_cObject, rb_intern("JSON"))) {
	json_module = rb_const_get_at(rb_cObject, rb_intern("JSON"));
    } else {
	json_module = rb_define_module("JSON");
    }
    if (rb_const_defined_at(json_module, rb_intern("JSONError"))) {
        json_error_class = rb_const_get(json_module, rb_intern("JSONError"));
    } else {
        json_error_class = rb_define_class_under(json_module, "JSONError", rb_eStandardError);
    }
    if (rb_const_defined_at(json_module, rb_intern("GeneratorError"))) {
        gen_err_class = rb_const_get(json_module, rb_intern("GeneratorError"));
    } else {
    	gen_err_class = rb_define_class_under(json_module, "GeneratorError", json_error_class);
    }
    rb_raise(gen_err_class, "%s", msg);
}

static void
dump_obj_attrs(const char *classname, Attr attrs, int depth, Out out) {
    int		d2 = depth + 1;
    int		d3 = d2 + 1;
    size_t	len = strlen(classname);
    size_t	sep_len = out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2;
    size_t	size = d2 * out->indent + d3 * out->indent + 10 + len + out->opts->create_id_len + sep_len;

    assure_size(out, size);
    *out->cur++ = '{';
    fill_indent(out, d2);
    *out->cur++ = '"';
    memcpy(out->cur, out->opts->create_id, out->opts->create_id_len);
    out->cur += out->opts->create_id_len;
    *out->cur++ = '"';
    if (0 < out->opts->dump_opts.before_size) {
	strcpy(out->cur, out->opts->dump_opts.before_sep);
	out->cur += out->opts->dump_opts.before_size;
    }
    *out->cur++ = ':';
    if (0 < out->opts->dump_opts.after_size) {
	strcpy(out->cur, out->opts->dump_opts.after_sep);
	out->cur += out->opts->dump_opts.after_size;
    }
    *out->cur++ = '"';
    memcpy(out->cur, classname, len);
    out->cur += len;
    *out->cur++ = '"';
    size = d3 * out->indent + 2;
    for (; NULL != attrs->name; attrs++) {
	assure_size(out, size + attrs->len + sep_len + 2);
	*out->cur++ = ',';
	fill_indent(out, d3);
	*out->cur++ = '"';
	memcpy(out->cur, attrs->name, attrs->len);
	out->cur += attrs->len;
	*out->cur++ = '"';
	if (0 < out->opts->dump_opts.before_size) {
	    strcpy(out->cur, out->opts->dump_opts.before_sep);
	    out->cur += out->opts->dump_opts.before_size;
	}
	*out->cur++ = ':';
	if (0 < out->opts->dump_opts.after_size) {
	    strcpy(out->cur, out->opts->dump_opts.after_sep);
	    out->cur += out->opts->dump_opts.after_size;
	}
	oj_dump_compat_val(attrs->value, d3, out, true);
    }
    assure_size(out, depth * out->indent + 2);
    fill_indent(out, depth);
    *out->cur++ = '}';
    *out->cur = '\0';
}

static ID	numerator_id = 0;
static ID	denominator_id = 0;

static void
rational_alt(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "n", 1, Qnil },
	{ "d", 1, Qnil },
	{ NULL, 0, Qnil },
    };
    if (0 == numerator_id) {
	numerator_id = rb_intern("numerator");
	denominator_id = rb_intern("denominator");
    }
    attrs[0].value = rb_funcall(obj, numerator_id, 0);
    attrs[1].value = rb_funcall(obj, denominator_id, 0);

    dump_obj_attrs(rb_class2name(rb_obj_class(obj)), attrs, depth, out);
}

static ID	real_id = 0;
static ID	imag_id = 0;

static void
complex_alt(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "r", 1, Qnil },
	{ "i", 1, Qnil },
	{ NULL, 0, Qnil },
    };

    if (0 == real_id) {
	real_id = rb_intern("real");
	imag_id = rb_intern("imag");
    }
    attrs[0].value = rb_funcall(obj, real_id, 0);
    attrs[1].value = rb_funcall(obj, imag_id, 0);

    dump_obj_attrs(rb_class2name(rb_obj_class(obj)), attrs, depth, out);
}

static struct _Alt	alts[] = {
    { "Complex", Qnil, complex_alt, false },
    { "Rational", Qnil, rational_alt, false },
    //{ "OpenStruct", Qnil, NULL, false },
    // TBD the rest of the library classes
    { NULL, Qundef, NULL, false },
};

static bool
dump_alt(VALUE obj, int depth, Out out) {
    VALUE	clas = rb_obj_class(obj);
    Alt		a = alts;

    for (; NULL != a->name; a++) {
	if (Qnil == a->clas) {	
	    a->clas = rb_const_get_at(rb_cObject, rb_intern(a->name));
	}
	if (clas == a->clas && a->active) {
	    a->func(obj, depth, out);
	    return true;
	}
    }
    return false;
}

VALUE
oj_add_to_json(int argc, VALUE *argv, VALUE self) {
    Alt	a = alts;

    if (0 == argc) {
	for (; NULL != a->name; a++) {
	    if (Qnil == a->clas) {	
		a->clas = rb_const_get_at(rb_cObject, rb_intern(a->name));
	    }
	    a->active = true;
	}
    } else {
	for (; 0 < argc; argc--, argv++) {
	    for (; NULL != a->name; a++) {
		if (Qnil == a->clas) {	
		    a->clas = rb_const_get_at(rb_cObject, rb_intern(a->name));
		}
		if (*argv == a->clas) {
		    a->active = true;
		    break;
		}
	    }
	}
    }
    return Qnil;
}

VALUE
oj_remove_to_json(int argc, VALUE *argv, VALUE self) {
    Alt	a = alts;

    if (0 == argc) {
	for (; NULL != a->name; a++) {
	    if (Qnil == a->clas) {	
		a->clas = rb_const_get_at(rb_cObject, rb_intern(a->name));
	    }
	    a->active = false;
	}
    } else {
	for (; 0 < argc; argc--, argv++) {
	    for (; NULL != a->name; a++) {
		if (Qnil == a->clas) {	
		    a->clas = rb_const_get_at(rb_cObject, rb_intern(a->name));
		}
		if (*argv == a->clas) {
		    a->active = false;
		    break;
		}
	    }
	}
    }
    return Qnil;
}

// The JSON gem is inconsistent with handling of infinity. Using
// JSON.dump(0.1/0) returns the string Infinity but (0.1/0).to_json raise and
// exception. Worse, for BigDecimals a quoted "Infinity" is returned.
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
    } else if (OJ_INFINITY == d) {
	if (out->opts->dump_opts.nan_dump) {
	    strcpy(buf, "Infinity");
	} else {
	    raise_gen_err("Infinity not allowed in JSON.");
	}
    } else if (-OJ_INFINITY == d) {
	if (out->opts->dump_opts.nan_dump) {
	    strcpy(buf, "-Infinity");
	} else {
	    raise_gen_err("-Infinity not allowed in JSON.");
	}
    } else if (isnan(d)) {
	if (out->opts->dump_opts.nan_dump) {
	    strcpy(buf, "NaN");
	} else {
	    raise_gen_err("NaN not allowed in JSON.");
	}
    } else if (d == (double)(long long int)d) {
	//cnt = snprintf(buf, sizeof(buf), "%.1Lf", (long double)d);
	cnt = snprintf(buf, sizeof(buf), "%.1f", d);
    } else {
	//cnt = snprintf(buf, sizeof(buf), "%0.16Lg", (long double)d);
	cnt = snprintf(buf, sizeof(buf), "%0.16g", d);
    }
    assure_size(out, cnt);
    for (b = buf; '\0' != *b; b++) {
	*out->cur++ = *b;
    }
    *out->cur = '\0';
}

static int
hash_cb(VALUE key, VALUE value, Out out) {
    int	depth = out->depth;

    if (out->omit_nil && Qnil == value) {
	return ST_CONTINUE;
    }
    if (!out->opts->dump_opts.use) {
	assure_size(out, depth * out->indent + 1);
	fill_indent(out, depth);
    } else {
	assure_size(out, depth * out->opts->dump_opts.indent_size + out->opts->dump_opts.hash_size + 1);
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
    switch (rb_type(key)) {
    case T_STRING:
	oj_dump_str(key, 0, out, false);
	break;
    case T_SYMBOL:
	oj_dump_sym(key, 0, out, false);
	break;
    default:
	/*rb_raise(rb_eTypeError, "In :compat mode all Hash keys must be Strings or Symbols, not %s.\n", rb_class2name(rb_obj_class(key)));*/
	oj_dump_str(rb_funcall(key, oj_to_s_id, 0), 0, out, false);
	break;
    }
    if (!out->opts->dump_opts.use) {
	*out->cur++ = ':';
    } else {
	assure_size(out, out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2);
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
    oj_dump_compat_val(value, depth, out, true);
    out->depth = depth;
    *out->cur++ = ',';

    return ST_CONTINUE;
}

static void
dump_hash(VALUE obj, int depth, Out out, bool as_ok) {
    int		cnt;

    cnt = (int)RHASH_SIZE(obj);
    assure_size(out, 2);
    if (0 == cnt) {
	*out->cur++ = '{';
	*out->cur++ = '}';
    } else {
	long	id = oj_check_circular(obj, out);

	if (0 > id) {
	    oj_dump_nil(Qnil, 0, out, false);
	    return;
	}
	*out->cur++ = '{';
	out->depth = depth + 1;
	rb_hash_foreach(obj, hash_cb, (VALUE)out);
	if (',' == *(out->cur - 1)) {
	    out->cur--; // backup to overwrite last comma
	}
	if (!out->opts->dump_opts.use) {
	    assure_size(out, depth * out->indent + 2);
	    fill_indent(out, depth);
	} else {
	    assure_size(out, depth * out->opts->dump_opts.indent_size + out->opts->dump_opts.hash_size + 1);
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

// In compat mode only the first call check for to_json. After that to_s is
// called.
static void
dump_obj(VALUE obj, int depth, Out out, bool as_ok) {
    if (dump_alt(obj, depth, out)) {
	return;
    }

/*
    VALUE	clas = rb_obj_class(obj);
    // to_s classes
    if (rb_cTime == clas ||
	oj_bigdecimal_class == clas ||
	rb_cRational == clas ||
	oj_datetime_class == clas ||
	oj_date_class == clas) {

	volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);

	return;
    }
*/
    if (as_ok && rb_respond_to(obj, oj_to_json_id)) {
	volatile VALUE	rs;
	const char	*s;
	int		len;

	rs = rb_funcall(obj, oj_to_json_id, 0);
	s = rb_string_value_ptr((VALUE*)&rs);
	len = (int)RSTRING_LEN(rs);

	assure_size(out, len + 1);
	memcpy(out->cur, s, len);
	out->cur += len;
	*out->cur = '\0';
	return;
    }
    // Nothing else matched so encode as a JSON object with Ruby obj members
    // as JSON object members.
    oj_dump_obj_to_s(obj, out);
}

static void
dump_array(VALUE a, int depth, Out out, bool as_ok) {
    size_t	size;
    int		i, cnt;
    int		d2 = depth + 1;
    long	id = oj_check_circular(a, out);

    if (id < 0) {
	return;
    }
    cnt = (int)RARRAY_LEN(a);
    *out->cur++ = '[';
    if (0 < id) {
	oj_dump_nil(Qnil, 0, out, false);
    }
    size = 2;
    assure_size(out, size);
    if (0 == cnt) {
	*out->cur++ = ']';
    } else {
	if (0 < id) {
	    *out->cur++ = ',';
	}
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
	    oj_dump_compat_val(rb_ary_entry(a, i), d2, out, true);
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

static void
dump_struct(VALUE obj, int depth, Out out, bool as_ok) {
    VALUE	clas = rb_obj_class(obj);

    // to_s classes
    if (rb_cRange == clas) {
	*out->cur++ = '"';
	oj_dump_compat_val(rb_funcall(obj, oj_begin_id, 0), 0, out, false);
	assure_size(out, 3);
	*out->cur++ = '.';
	*out->cur++ = '.';
	if (Qtrue == rb_funcall(obj, oj_exclude_end_id, 0)) {
	    *out->cur++ = '.';
	}
	oj_dump_compat_val(rb_funcall(obj, oj_end_id, 0), 0, out, false);
	*out->cur++ = '"';

	return;
    }
    if (as_ok && rb_respond_to(obj, oj_to_json_id)) {
	volatile VALUE	rs = rb_funcall(obj, oj_to_json_id, 0);
	const char	*s;
	int		len;

	s = rb_string_value_ptr((VALUE*)&rs);
	len = (int)RSTRING_LEN(rs);
	assure_size(out, len);
	memcpy(out->cur, s, len);
	out->cur += len;
    } else {
#if 0
	VALUE		ma = Qnil;
	VALUE		v;
	char		num_id[32];
	int		i;
	int		d2 = depth + 1;
	int		d3 = d2 + 1;
	size_t		size = d2 * out->indent + d3 * out->indent + 3;
	const char	*name;
	int		cnt;
	size_t		len;	

	assure_size(out, size);
	*out->cur++ = '{';
	fill_indent(out, d2);
	size = d3 * out->indent + 2;
#if HAS_STRUCT_MEMBERS
	ma = rb_struct_s_members(clas);
#endif

#ifdef RSTRUCT_LEN
#if UNIFY_FIXNUM_AND_BIGNUM
	cnt = (int)NUM2LONG(RSTRUCT_LEN(obj));
#else // UNIFY_FIXNUM_AND_INTEGER
	cnt = (int)RSTRUCT_LEN(obj);
#endif // UNIFY_FIXNUM_AND_INTEGER
#else
	// This is a bit risky as a struct in C ruby is not the same as a Struct
	// class in interpreted Ruby so length() may not be defined.
	cnt = FIX2INT(rb_funcall2(obj, oj_length_id, 0, 0));
#endif
	for (i = 0; i < cnt; i++) {
#ifdef RSTRUCT_LEN
	    v = RSTRUCT_GET(obj, i);
#else
	    v = rb_struct_aref(obj, INT2FIX(i));
#endif
	    if (ma != Qnil) {
		name = rb_id2name(SYM2ID(rb_ary_entry(ma, i)));
		len = strlen(name);
	    } else {
		len = snprintf(num_id, sizeof(num_id), "%d", i);
		name = num_id;
	    }
	    assure_size(out, size + len + 3);
	    fill_indent(out, d3);
	    *out->cur++ = '"';
	    memcpy(out->cur, name, len);
	    out->cur += len;
	    *out->cur++ = '"';
	    *out->cur++ = ':';
	    oj_dump_compat_val(v, d3, out, false);
	    *out->cur++ = ',';
	}
	out->cur--;
	*out->cur++ = '}';
	*out->cur = '\0';
#else
	oj_dump_obj_to_s(obj, out);
#endif
    }
}

static void
dump_regexp(VALUE obj, int depth, Out out, bool as_ok) {
    oj_dump_obj_to_s(obj, out);
}

static void
dump_complex(VALUE obj, int depth, Out out, bool as_ok) {
    dump_obj(obj, depth, out, as_ok);
    //oj_dump_obj_to_s(obj, out);
}

static void
dump_rational(VALUE obj, int depth, Out out, bool as_ok) {
    dump_obj(obj, depth, out, as_ok);
    //oj_dump_obj_to_s(obj, out);
}

static DumpFunc	compat_funcs[] = {
    NULL,	 	// RUBY_T_NONE   = 0x00,
    dump_obj,		// RUBY_T_OBJECT = 0x01,
    oj_dump_class, 	// RUBY_T_CLASS  = 0x02,
    oj_dump_class,	// RUBY_T_MODULE = 0x03,
    dump_float, 	// RUBY_T_FLOAT  = 0x04,
    oj_dump_str, 	// RUBY_T_STRING = 0x05,
    dump_regexp,	// RUBY_T_REGEXP = 0x06,
    dump_array,		// RUBY_T_ARRAY  = 0x07,
    dump_hash,	 	// RUBY_T_HASH   = 0x08,
    dump_struct,	// RUBY_T_STRUCT = 0x09,
    oj_dump_bignum,	// RUBY_T_BIGNUM = 0x0a,
    NULL, 		// RUBY_T_FILE   = 0x0b,
    dump_obj,		// RUBY_T_DATA   = 0x0c,
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
oj_dump_compat_val(VALUE obj, int depth, Out out, bool as_ok) {
    int	type = rb_type(obj);

    if (out->opts->dump_opts.max_depth < depth) {
	rb_raise(rb_eNoMemError, "Too deeply nested.\n");
    }
    if (0 < type && type <= RUBY_T_FIXNUM) {
	DumpFunc	f = compat_funcs[type];

	if (NULL != f) {
	    f(obj, depth, out, as_ok);
	    return;
	}
    }
    oj_dump_nil(Qnil, depth, out, false);
}
