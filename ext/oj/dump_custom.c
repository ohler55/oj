/* dump_custom.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"
#include "odd.h"
#include "code.h"

static void
bigdecimal_dump(VALUE obj, int depth, Out out) {
    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);
    const char		*str = rb_string_value_ptr((VALUE*)&rstr);
    int			len = RSTRING_LEN(rstr);

    if (0 == strcasecmp("Infinity", str)) {
	str = oj_nan_str(obj, out->opts->dump_opts.nan_dump, out->opts->mode, true, &len);
	oj_dump_raw(str, len, out);
    } else if (0 == strcasecmp("-Infinity", str)) {
	str = oj_nan_str(obj, out->opts->dump_opts.nan_dump, out->opts->mode, false, &len);
	oj_dump_raw(str, len, out);
    } else {
	oj_dump_raw(str, len, out);
    }
}

static ID	real_id = 0;
static ID	imag_id = 0;

static void
complex_dump(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "real", 4, Qnil },
	{ "imag", 4, Qnil },
	{ NULL, 0, Qnil },
    };

    if (0 == real_id) {
	real_id = rb_intern("real");
	imag_id = rb_intern("imag");
    }
    attrs[0].value = rb_funcall(obj, real_id, 0);
    attrs[1].value = rb_funcall(obj, imag_id, 0);

    oj_code_attrs(obj, attrs, depth, out);
}

static ID	xmlschema_id = 0;
static ID	to_time_id = 0;

static void
datetime_dump(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "xmlschema", 9, Qnil },
	{ NULL, 0, Qnil },
    };
    if (0 == xmlschema_id) {
	xmlschema_id = rb_intern("xmlschema");
	to_time_id = rb_intern("to_time");
    }
    switch (out->opts->time_format) {
    case XmlTime:
	attrs->name = "xmlschema";
	attrs->len = 9;
	attrs->value = rb_funcall(obj, xmlschema_id, 1, INT2FIX(out->opts->sec_prec));
	break;
    case UnixZTime:
	attrs->name = "unixzone";
	attrs->len = 8;
	attrs->value = Qundef;
	attrs->time = rb_funcall(obj, to_time_id, 0);
	break;
    case UnixTime:
	attrs->name = "unix";
	attrs->value = Qundef;
	attrs->time = rb_funcall(obj, to_time_id, 0);
	break;
    case RubyTime:
    default:
	attrs->name = "ruby";
	attrs->len = 4;
	attrs->value = rb_funcall(obj, oj_to_s_id, 0);
	break;
    }
    oj_code_attrs(obj, attrs, depth, out);
}

static ID	table_id = 0;

static void
openstruct_dump(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "table", 5, Qnil },
	{ NULL, 0, Qnil },
    };
    if (0 == table_id) {
	table_id = rb_intern("table");
    }
    attrs->value = rb_funcall(obj, table_id, 0);

    oj_code_attrs(obj, attrs, depth, out);
}

static void
range_dump(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "begin", 5, Qnil },
	{ "end", 3, Qnil },
	{ "exclude", 7, Qnil },
	{ NULL, 0, Qnil },
    };
    attrs[0].value = rb_funcall(obj, oj_begin_id, 0);
    attrs[1].value = rb_funcall(obj, oj_end_id, 0);
    attrs[2].value = rb_funcall(obj, oj_exclude_end_id, 0);

    oj_code_attrs(obj, attrs, depth, out);
}

static ID	numerator_id = 0;
static ID	denominator_id = 0;

static void
rational_dump(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "numerator", 9, Qnil },
	{ "denominator", 11, Qnil },
	{ NULL, 0, Qnil },
    };
    if (0 == numerator_id) {
	numerator_id = rb_intern("numerator");
	denominator_id = rb_intern("denominator");
    }
    attrs[0].value = rb_funcall(obj, numerator_id, 0);
    attrs[1].value = rb_funcall(obj, denominator_id, 0);

    oj_code_attrs(obj, attrs, depth, out);
}

static ID	options_id = 0;
static ID	source_id = 0;

static void
regexp_dump(VALUE obj, int depth, Out out) {
    struct _Attr	attrs[] = {
	{ "source", 9, Qnil },
	{ "options", 11, Qnil },
	{ NULL, 0, Qnil },
    };
    if (0 == options_id) {
	options_id = rb_intern("options");
	source_id = rb_intern("source");
    }
    attrs[0].value = rb_funcall(obj, source_id, 0);
    attrs[1].value = rb_funcall(obj, options_id, 0);

    oj_code_attrs(obj, attrs, depth, out);
}

static void
dump_time(VALUE obj, int depth, Out out) {
    switch (out->opts->time_format) {
    case RubyTime:	oj_dump_obj_to_s(obj, out);	break;
    case XmlTime:	oj_dump_xml_time(obj, out);	break;
    case UnixZTime:	oj_dump_time(obj, out, 1);	break;
    case UnixTime:
    default:		oj_dump_time(obj, out, 0);	break;
    }
}

static struct _Code	codes[] = {
    { "BigDecimal", Qnil, bigdecimal_dump, NULL, true },
    /////{ "Complex", Qnil, dump_complex, NULL, true },
    { "Date", Qnil, datetime_dump, NULL, true },
    { "DateTime", Qnil, datetime_dump, NULL, true },
    { "OpenStruct", Qnil, openstruct_dump, NULL, true },
    { "Range", Qnil, range_dump, NULL, true },
    /////{ "Rational", Qnil, rational_alt, NULL, true },
    { "Regexp", Qnil, regexp_dump, NULL, true },
    { "Time", Qnil, dump_time, NULL, true },
    { NULL, Qundef, NULL, NULL, false },
};

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
    oj_dump_custom_val(value, depth, out, true);
    out->depth = depth;
    *out->cur++ = ',';

    return ST_CONTINUE;
}

static void
dump_hash(VALUE obj, int depth, Out out, bool as_ok) {
    int		cnt;
    long	id = oj_check_circular(obj, out);

    if (0 > id) {
	oj_dump_nil(Qnil, depth, out, false);
	return;
    }
    cnt = (int)RHASH_SIZE(obj);
    assure_size(out, 2);
    if (0 == cnt) {
	*out->cur++ = '{';
	*out->cur++ = '}';
    } else {
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

static void
dump_odd(VALUE obj, Odd odd, VALUE clas, int depth, Out out) {
    ID			*idp;
    AttrGetFunc		*fp;
    volatile VALUE	v;
    const char		*name;
    size_t		size;
    int			d2 = depth + 1;

    assure_size(out, 2);
    *out->cur++ = '{';
    if (NULL != out->opts->create_id) {
	const char	*classname = rb_class2name(clas);
	int		clen = (int)strlen(classname);
	size_t		sep_len = out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2;

	size = d2 * out->indent + 10 + clen + out->opts->create_id_len + sep_len;
	assure_size(out, size);
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
	memcpy(out->cur, classname, clen);
	out->cur += clen;
	*out->cur++ = '"';
	*out->cur++ = ',';
    }
    if (odd->raw) {
	v = rb_funcall(obj, *odd->attrs, 0);
	if (Qundef == v || T_STRING != rb_type(v)) {
	    rb_raise(rb_eEncodingError, "Invalid type for raw JSON.\n");
	} else {	    
	    const char	*s = rb_string_value_ptr((VALUE*)&v);
	    int		len = RSTRING_LEN(v);
	    const char	*name = rb_id2name(*odd->attrs);
	    size_t	nlen = strlen(name);

	    size = len + d2 * out->indent + nlen + 10;
	    assure_size(out, size);
	    fill_indent(out, d2);
	    *out->cur++ = '"';
	    memcpy(out->cur, name, nlen);
	    out->cur += nlen;
	    *out->cur++ = '"';
	    *out->cur++ = ':';
	    memcpy(out->cur, s, len);
	    out->cur += len;
	    *out->cur = '\0';
	}
    } else {
	size = d2 * out->indent + 1;
	for (idp = odd->attrs, fp = odd->attrFuncs; 0 != *idp; idp++, fp++) {
	    size_t	nlen;

	    assure_size(out, size);
	    name = rb_id2name(*idp);
	    nlen = strlen(name);
	    if (0 != *fp) {
		v = (*fp)(obj);
	    } else if (0 == strchr(name, '.')) {
		v = rb_funcall(obj, *idp, 0);
	    } else {
		char	nbuf[256];
		char	*n2 = nbuf;
		char	*n;
		char	*end;
		ID	i;
	    
		if (sizeof(nbuf) <= nlen) {
		    n2 = strdup(name);
		} else {
		    strcpy(n2, name);
		}
		n = n2;
		v = obj;
		while (0 != (end = strchr(n, '.'))) {
		    *end = '\0';
		    i = rb_intern(n);
		    v = rb_funcall(v, i, 0);
		    n = end + 1;
		}
		i = rb_intern(n);
		v = rb_funcall(v, i, 0);
		if (nbuf != n2) {
		    free(n2);
		}
	    }
	    fill_indent(out, d2);
	    oj_dump_cstr(name, nlen, 0, 0, out);
	    *out->cur++ = ':';
	    oj_dump_custom_val(v, d2, out, true);
	    assure_size(out, 2);
	    *out->cur++ = ',';
	}
	out->cur--;
    }
    *out->cur++ = '}';
    *out->cur = '\0';
}

// Return class if still needs dumping.
static VALUE
dump_common(VALUE obj, int depth, Out out) {
    if (Yes == out->opts->to_json && rb_respond_to(obj, oj_to_json_id)) {
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
    } else if (Yes == out->opts->as_json && rb_respond_to(obj, oj_as_json_id)) {
	volatile VALUE	aj;
	int		arity;

#if HAS_METHOD_ARITY
	arity = rb_obj_method_arity(obj, oj_as_json_id);
#else
	arity = out->argc;
#endif
	// Some classes elect to not take an options argument so check the arity
	// of as_json.
	switch (arity) {
	case 0:
	    aj = rb_funcall(obj, oj_as_json_id, 0);
	    break;
	case 1:
	    if (1 <= out->argc) {
		aj = rb_funcall2(obj, oj_as_json_id, 1, (VALUE*)out->argv);
	    } else {
		aj = rb_funcall(obj, oj_as_json_id, 1, Qnil);
	    }
	    break;
	default:
	    aj = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
	    break;
	}
	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), false, false, out);
	} else {
	    oj_dump_custom_val(aj, depth, out, true);
	}
    } else if (Yes == out->opts->to_hash && rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);

	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    oj_dump_custom_val(h, depth, out, false);
	} else {
	    dump_hash(h, depth, out, true);
	}
    } else if (!oj_code_dump(codes, obj, depth, out)) {
	VALUE	clas = rb_obj_class(obj);
	Odd	odd = oj_get_odd(clas);

	if (NULL == odd) {
	    return clas;
	}
	dump_odd(obj, odd, clas, depth + 1, out);
    }
    return Qnil;
}

static int
dump_attr_cb(ID key, VALUE value, Out out) {
    int		depth = out->depth;
    size_t	size;
    const char	*attr;

    if (out->omit_nil && Qnil == value) {
	return ST_CONTINUE;
    }
    size = depth * out->indent + 1;
    attr = rb_id2name(key);

#if HAS_EXCEPTION_MAGIC
    if (0 == strcmp("bt", attr) || 0 == strcmp("mesg", attr)) {
	return ST_CONTINUE;
    }
#endif
    assure_size(out, size);
    fill_indent(out, depth);
    if ('@' == *attr) {
	attr++;
	oj_dump_cstr(attr, strlen(attr), 0, 0, out);
    } else {
	char	buf[32];

	*buf = '~';
	strncpy(buf + 1, attr, sizeof(buf) - 2);
	buf[sizeof(buf) - 1] = '\0';
	oj_dump_cstr(buf, strlen(buf), 0, 0, out);
    }
    *out->cur++ = ':';
    oj_dump_custom_val(value, depth, out, true);
    out->depth = depth;
    *out->cur++ = ',';
    
    return ST_CONTINUE;
}

static void
dump_obj_attrs(VALUE obj, VALUE clas, slot_t id, int depth, Out out) {
    size_t	size = 0;
    int		d2 = depth + 1;
    int		cnt;

    assure_size(out, 2);
    *out->cur++ = '{';
    if (Qundef != clas) {
	size_t		sep_len = out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2;
	const char	*classname = rb_obj_classname(obj);
	size_t		len = strlen(classname);

	size = d2 * out->indent + 10 + len + out->opts->create_id_len + sep_len;
	assure_size(out, size);
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
    }
    cnt = (int)rb_ivar_count(obj);
    if (Qundef != clas && 0 < cnt) {
	*out->cur++ = ',';
    }
    if (0 == cnt && Qundef == clas) {
	// Might be something special like an Enumerable.
	if (Qtrue == rb_obj_is_kind_of(obj, oj_enumerable_class)) {
	    out->cur--;
	    oj_dump_custom_val(rb_funcall(obj, rb_intern("entries"), 0), depth, out, false);
	    return;
	}
    }
    out->depth = depth + 1;
    rb_ivar_foreach(obj, dump_attr_cb, (VALUE)out);
    if (',' == *(out->cur - 1)) {
	out->cur--; // backup to overwrite last comma
    }
#if HAS_EXCEPTION_MAGIC
    if (rb_obj_is_kind_of(obj, rb_eException)) {
	volatile VALUE	rv;

	if (',' != *(out->cur - 1)) {
	    *out->cur++ = ',';
	}
	// message
	assure_size(out, 2);
	fill_indent(out, d2);
	oj_dump_cstr("~mesg", 5, 0, 0, out);
	*out->cur++ = ':';
	rv = rb_funcall2(obj, rb_intern("message"), 0, 0);
	oj_dump_custom_val(rv, d2, out, true);
	assure_size(out, size + 2);
	*out->cur++ = ',';
	// backtrace
	fill_indent(out, d2);
	oj_dump_cstr("~bt", 3, 0, 0, out);
	*out->cur++ = ':';
	rv = rb_funcall2(obj, rb_intern("backtrace"), 0, 0);
	oj_dump_custom_val(rv, d2, out, true);
	assure_size(out, 2);
    }
#endif
    out->depth = depth;

    fill_indent(out, depth);
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
dump_obj(VALUE obj, int depth, Out out, bool as_ok) {
    long	id = oj_check_circular(obj, out);
    VALUE	clas;
    
    if (0 > id) {
	oj_dump_nil(Qnil, depth, out, false);
    } else if (Qnil != (clas = dump_common(obj, depth, out))) {
	dump_obj_attrs(obj, clas, 0, depth, out);
    }
    *out->cur = '\0';
}

static void
dump_array(VALUE a, int depth, Out out, bool as_ok) {
    size_t	size;
    int		i, cnt;
    int		d2 = depth + 1;
    long	id = oj_check_circular(a, out);

    if (0 > id) {
	oj_dump_nil(Qnil, depth, out, false);
	return;
    }
    cnt = (int)RARRAY_LEN(a);
    *out->cur++ = '[';
    assure_size(out, 2);
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
	    oj_dump_custom_val(rb_ary_entry(a, i), d2, out, true);
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
    long	id = oj_check_circular(obj, out);
    VALUE	clas;
    
    if (0 > id) {
	oj_dump_nil(Qnil, depth, out, false);
    } else if (Qnil != (clas = dump_common(obj, depth, out))) {
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
	if (clas == rb_cRange) {
	    *out->cur++ = '"';
	    oj_dump_custom_val(rb_funcall(obj, oj_begin_id, 0), d3, out, false);
	    assure_size(out, 3);
	    *out->cur++ = '.';
	    *out->cur++ = '.';
	    if (Qtrue == rb_funcall(obj, oj_exclude_end_id, 0)) {
		*out->cur++ = '.';
	    }
	    oj_dump_custom_val(rb_funcall(obj, oj_end_id, 0), d3, out, false);
	    *out->cur++ = '"';

	    return;
	}
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
	    oj_dump_custom_val(v, d3, out, true);
	    *out->cur++ = ',';
	}
	out->cur--;
	*out->cur++ = '}';
	*out->cur = '\0';
    }
}

static void
dump_data(VALUE obj, int depth, Out out, bool as_ok) {
    long	id = oj_check_circular(obj, out);
    VALUE	clas;

    if (0 > id) {
	oj_dump_nil(Qnil, depth, out, false);
    } else if (Qnil != (clas = dump_common(obj, depth, out))) {
	dump_obj_attrs(obj, clas, id, depth, out);
    }
}

static void
dump_regexp(VALUE obj, int depth, Out out, bool as_ok) {
    // TBD like object mode but without class or maybe option for create id of ^c or something
    oj_dump_obj_to_s(obj, out);
}

static void
dump_complex(VALUE obj, int depth, Out out, bool as_ok) {
    complex_dump(obj, depth, out);
}

static void
dump_rational(VALUE obj, int depth, Out out, bool as_ok) {
    rational_dump(obj, depth, out);
}

static DumpFunc	custom_funcs[] = {
    NULL,	 	// RUBY_T_NONE     = 0x00,
    dump_obj,		// RUBY_T_OBJECT   = 0x01,
    oj_dump_class, 	// RUBY_T_CLASS    = 0x02,
    oj_dump_class,	// RUBY_T_MODULE   = 0x03,
    oj_dump_float, 	// RUBY_T_FLOAT    = 0x04,
    oj_dump_str, 	// RUBY_T_STRING   = 0x05,
    dump_regexp,	// RUBY_T_REGEXP   = 0x06,
    dump_array,		// RUBY_T_ARRAY    = 0x07,
    dump_hash,	 	// RUBY_T_HASH     = 0x08,
    dump_struct,	// RUBY_T_STRUCT   = 0x09,
    oj_dump_bignum,	// RUBY_T_BIGNUM   = 0x0a,
    NULL, 		// RUBY_T_FILE     = 0x0b,
    dump_data,		// RUBY_T_DATA     = 0x0c,
    NULL, 		// RUBY_T_MATCH    = 0x0d,
    dump_complex, 	// RUBY_T_COMPLEX  = 0x0e,
    dump_rational, 	// RUBY_T_RATIONAL = 0x0f,
    NULL, 		// 0x10
    oj_dump_nil, 	// RUBY_T_NIL      = 0x11,
    oj_dump_true, 	// RUBY_T_TRUE     = 0x12,
    oj_dump_false,	// RUBY_T_FALSE    = 0x13,
    oj_dump_sym,	// RUBY_T_SYMBOL   = 0x14,
    oj_dump_fixnum,	// RUBY_T_FIXNUM   = 0x15,
};

void
oj_dump_custom_val(VALUE obj, int depth, Out out, bool as_ok) {
    int	type = rb_type(obj);
    
    if (MAX_DEPTH < depth) {
	rb_raise(rb_eNoMemError, "Too deeply nested.\n");
    }
    if (0 < type && type <= RUBY_T_FIXNUM) {
	DumpFunc	f = custom_funcs[type];

	if (NULL != f) {
	    f(obj, depth, out, true);
	    return;
	}
    }
    oj_dump_nil(Qnil, depth, out, false);
}
