/* dump_object.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"

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
dump_hash_class(VALUE obj, VALUE clas, int depth, Out out) {
    int		cnt;
    size_t	size;

    cnt = (int)RHASH_SIZE(obj);
    size = depth * out->indent + 2;
    assure_size(out, 2);
    if (0 == cnt) {
	*out->cur++ = '{';
	*out->cur++ = '}';
    } else {
	long	id = oj_check_circular(obj, out);

	if (0 > id) {
	    return;
	}
	*out->cur++ = '{';
	if (0 < id) {
	    assure_size(out, size + 16);
	    fill_indent(out, depth + 1);
	    *out->cur++ = '"';
	    *out->cur++ = '^';
	    *out->cur++ = 'i';
	    *out->cur++ = '"';
	    *out->cur++ = ':';
	    dump_ulong(id, out);
	    *out->cur++ = ',';
	}
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
dump_hash(VALUE obj, int depth, Out out, bool as_ok) {
    dump_hash_class(obj, rb_obj_class(obj), depth, out);
}

#if HAS_IVAR_HELPERS
static int
dump_attr_cb(ID key, VALUE value, Out out) {
    int		depth = out->depth;
    size_t	size = depth * out->indent + 1;
    const char	*attr = rb_id2name(key);

    if (out->omit_nil && Qnil == value) {
	return ST_CONTINUE;
    }
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
    oj_dump_comp_val(value, depth, out, 0, 0, true);
    out->depth = depth;
    *out->cur++ = ',';
    
    return ST_CONTINUE;
}
#endif

static void
dump_obj_attrs(VALUE obj, VALUE clas, slot_t id, int depth, Out out) {
    size_t	size = 0;
    int		d2 = depth + 1;
    int		cnt;

    assure_size(out, 2);
    *out->cur++ = '{';
    {
#if HAS_IVAR_HELPERS
	cnt = (int)rb_ivar_count(obj);
#else
	volatile VALUE	vars = rb_funcall2(obj, oj_instance_variables_id, 0, 0);
	VALUE		*np = RARRAY_PTR(vars);
	ID		vid;
	const char	*attr;
	int		i;
	int		first = 1;

	cnt = (int)RARRAY_LEN(vars);
#endif
	if (Qundef != clas && 0 < cnt) {
	    *out->cur++ = ',';
	}
	if (0 == cnt && Qundef == clas) {
	    // Might be something special like an Enumerable.
	    if (Qtrue == rb_obj_is_kind_of(obj, oj_enumerable_class)) {
		out->cur--;
		oj_dump_compat_val(rb_funcall(obj, rb_intern("entries"), 0), depth, out, false);
		return;
	    }
	}
	out->depth = depth + 1;
#if HAS_IVAR_HELPERS
	rb_ivar_foreach(obj, dump_attr_cb, (VALUE)out);
	if (',' == *(out->cur - 1)) {
	    out->cur--; // backup to overwrite last comma
	}
#else
	size = d2 * out->indent + 1;
	for (i = cnt; 0 < i; i--, np++) {
	    VALUE	value;
	    
	    vid = rb_to_id(*np);
	    attr = rb_id2name(vid);
	    value = rb_ivar_get(obj, vid);
	    if (out->omit_nil && Qnil == value) {
		continue;
	    }
	    if (first) {
		first = 0;
	    } else {
		*out->cur++ = ',';
	    }
	    assure_size(out, size);
	    fill_indent(out, d2);
	    if ('@' == *attr) {
		attr++;
		oj_dump_cstr(attr, strlen(attr), 0, 0, out);
	    } else {
		char	buf[32];

		*buf = '~';
		strncpy(buf + 1, attr, sizeof(buf) - 2);
		buf[sizeof(buf) - 1] = '\0';
		oj_dump_cstr(buf, strlen(attr) + 1, 0, 0, out);
	    }
	    *out->cur++ = ':';
	    oj_dump_compat_val(value, d2, out, 0, 0, true);
	    assure_size(out, 2);
	}
#endif
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
	    oj_dump_compat_val(rv, d2, out, true);
	    assure_size(out, size + 2);
	    *out->cur++ = ',';
	    // backtrace
	    fill_indent(out, d2);
	    oj_dump_cstr("~bt", 3, 0, 0, out);
	    *out->cur++ = ':';
	    rv = rb_funcall2(obj, rb_intern("backtrace"), 0, 0);
	    oj_dump_compat_val(rv, d2, out, true);
	    assure_size(out, 2);
	}
#endif
	out->depth = depth;
    }
    fill_indent(out, depth);
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
dump_obj(VALUE obj, int depth, Out out, bool as_ok) {
    if (as_ok && Yes == out->opts->to_json && rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);

	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    oj_dump_compat_val(h, depth, out, false);
	} else {
	    dump_hash_class(h, Qundef, depth, out);
	}
    } else if (as_ok && Yes == out->opts->as_json && rb_respond_to(obj, oj_as_json_id)) {
	volatile VALUE	aj;

#if HAS_METHOD_ARITY
	// Some classes elect to not take an options argument so check the arity
	// of as_json.
	switch (rb_obj_method_arity(obj, oj_as_json_id)) {
	case 0:
	    aj = rb_funcall2(obj, oj_as_json_id, 0, 0);
	    break;
	case 1:
	    if (1 <= out->argc) {
		aj = rb_funcall2(obj, oj_as_json_id, 1, (VALUE*)out->argv);
	    } else {
		VALUE	nothing [1];

		nothing[0] = Qnil;
		aj = rb_funcall2(obj, oj_as_json_id, 1, nothing);
	    }
	    break;
	default:
	    aj = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
	    break;
	}
#else
	aj = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
#endif
	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), false, false, out);
	} else {
	    oj_dump_compat_val(aj, depth, out, false);
	}
    } else if (Yes == out->opts->to_json && rb_respond_to(obj, oj_to_json_id)) {
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
    } else {
	VALUE	clas = rb_obj_class(obj);

	if (oj_bigdecimal_class == clas) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);
	    const char		*str = rb_string_value_ptr((VALUE*)&rstr);
	    int			len = RSTRING_LEN(rstr);
	    
	    if (Yes == out->opts->bigdec_as_num) {
		oj_dump_raw(str, len, out);
	    } else if (0 == strcasecmp("Infinity", str)) {
		str = oj_nan_str(obj, out->opts->dump_opts.nan_dump, out->opts->mode, true, &len);
		oj_dump_raw(str, len, out);
	    } else if (0 == strcasecmp("-Infinity", str)) {
		str = oj_nan_str(obj, out->opts->dump_opts.nan_dump, out->opts->mode, false, &len);
		oj_dump_raw(str, len, out);
	    } else {
		oj_dump_cstr(str, len, 0, 0, out);
	    }
#if (defined T_RATIONAL && defined RRATIONAL)
	} else if (oj_datetime_class == clas || oj_date_class == clas || rb_cRational == clas) {
#else
	} else if (oj_datetime_class == clas || oj_date_class == clas) {
#endif
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    dump_obj_attrs(obj, Qundef, 0, depth, out);
	}
    }
    *out->cur = '\0';
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
	    oj_dump_comp_val(rb_ary_entry(a, i), d2, out, 0, 0, true);
	    if (i < cnt) {
		*out->cur++ = ',';
	    }
	}
	size = depth * out->indent + 1;
	assure_size(out, size);
	if (out->opts->dump_opts.use) {
	    //printf("*** d2: %u  indent: %u '%s'\n", d2, out->opts->dump_opts->indent_size, out->opts->dump_opts->indent);
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
    if (as_ok && Yes == out->opts->to_json && rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);
 
	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it
	    // returns an Array and not a Hash. To get around that any value
	    // returned will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    oj_dump_comp_val(h, depth, out, 0, 0, false);
	}
	dump_hash_class(h, Qundef, depth, out);
    } else if (as_ok && Yes == out->opts->as_json && rb_respond_to(obj, oj_as_json_id)) {
	volatile VALUE	aj;

#if HAS_METHOD_ARITY
	// Some classes elect to not take an options argument so check the arity
	// of as_json.
	switch (rb_obj_method_arity(obj, oj_as_json_id)) {
	case 0:
	    aj = rb_funcall2(obj, oj_as_json_id, 0, 0);
	    break;
	case 1:
	    if (1 <= out->argc) {
		aj = rb_funcall2(obj, oj_as_json_id, 1, out->argv);
	    } else {
		VALUE	nothing [1];

		nothing[0] = Qnil;
		aj = rb_funcall2(obj, oj_as_json_id, 1, nothing);
	    }
	    break;
	default:
	    aj = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
	    break;
	}
#else
	aj = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
#endif
	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), false, false, out);
	} else {
	    oj_dump_compat_val(aj, depth, out, false);
	}
    } else if (Yes == out->opts->to_json && rb_respond_to(obj, oj_to_json_id)) {
	volatile VALUE	rs = rb_funcall(obj, oj_to_json_id, 0);
	const char	*s;
	int		len;

	s = rb_string_value_ptr((VALUE*)&rs);
	len = (int)RSTRING_LEN(rs);
	assure_size(out, len);
	memcpy(out->cur, s, len);
	out->cur += len;
    } else {
	VALUE		clas = rb_obj_class(obj);
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
	    oj_dump_comp_val(rb_funcall(obj, oj_begin_id, 0), d3, out, 0, 0, false);
	    assure_size(out, 3);
	    *out->cur++ = '.';
	    *out->cur++ = '.';
	    if (Qtrue == rb_funcall(obj, oj_exclude_end_id, 0)) {
		*out->cur++ = '.';
	    }
	    oj_dump_comp_val(rb_funcall(obj, oj_end_id, 0), d3, out, 0, 0, false);
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
	    oj_dump_comp_val(v, d3, out, 0, 0, true);
	    *out->cur++ = ',';
	}
	out->cur--;
	*out->cur++ = '}';
	*out->cur = '\0';
    }
}

static void
dump_data(VALUE obj, int depth, Out out, bool as_ok) {
    VALUE	clas = rb_obj_class(obj);

    if (as_ok && Yes == out->opts->to_json && rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);

	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    oj_dump_compat_val(h, depth, out, false);
	}
	dump_hash_class(h, Qundef, depth, out);
    } else if (Yes == out->opts->bigdec_as_num && oj_bigdecimal_class == clas) {
	volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);
	const char	*str = rb_string_value_ptr((VALUE*)&rstr);
	int		len = RSTRING_LEN(rstr);

	if (0 == strcasecmp("Infinity", str)) {
	    str = oj_nan_str(obj, out->opts->dump_opts.nan_dump, out->opts->mode, true, &len);
	    oj_dump_raw(str, len, out);
	} else if (0 == strcasecmp("-Infinity", str)) {
	    str = oj_nan_str(obj, out->opts->dump_opts.nan_dump, out->opts->mode, false, &len);
	    oj_dump_raw(str, len, out);
	} else {
	    oj_dump_raw(str, len, out);
	}
    } else if (as_ok && Yes == out->opts->as_json && rb_respond_to(obj, oj_as_json_id)) {
	volatile VALUE	aj;

#if HAS_METHOD_ARITY
	// Some classes elect to not take an options argument so check the arity
	// of as_json.
	switch (rb_obj_method_arity(obj, oj_as_json_id)) {
	case 0:
	    aj = rb_funcall2(obj, oj_as_json_id, 0, 0);
	    break;
	case 1:
	    if (1 <= out->argc) {
		aj = rb_funcall2(obj, oj_as_json_id, 1, out->argv);
	    } else {
		VALUE	nothing [1];

		nothing[0] = Qnil;
		aj = rb_funcall2(obj, oj_as_json_id, 1, nothing);
	    }
	    break;
	default:
	    aj = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
	    break;
	}
#else
	aj = rb_funcall2(obj, oj_as_json_id, out->argc, out->argv);
#endif
	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    oj_dump_comp_val(aj, depth, out, 0, 0, false);
	}
    } else if (Yes == out->opts->to_json && rb_respond_to(obj, oj_to_json_id)) {
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
    } else {
	if (rb_cTime == clas) {
	    oj_dump_xml_time(obj, out);
	} else if (oj_bigdecimal_class == clas) {
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
		oj_dump_cstr(str, len, 0, 0, out);
	    }
	} else if (oj_datetime_class == clas) {
	    volatile VALUE	rstr;

	    switch (out->opts->time_format) {
	    case XmlTime:
		rstr = rb_funcall(obj, rb_intern("xmlschema"), 1, INT2FIX(out->opts->sec_prec));
		break;
	    case UnixZTime:
	    case UnixTime:
	    case RubyTime:
	    default:
		rstr = rb_funcall(obj, oj_to_s_id, 0);
	    }
	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	}
    }
}

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

static DumpFunc	compat_funcs[] = {
    NULL,	 	// RUBY_T_NONE   = 0x00,
    dump_obj,		// RUBY_T_OBJECT = 0x01,
    oj_dump_class, 	// RUBY_T_CLASS  = 0x02,
    oj_dump_class,	// RUBY_T_MODULE = 0x03,
    oj_dump_float, 	// RUBY_T_FLOAT  = 0x04,
    oj_dump_str, 	// RUBY_T_STRING = 0x05,
    dump_regexp,	// RUBY_T_REGEXP = 0x06,
    dump_array,		// RUBY_T_ARRAY  = 0x07,
    dump_hash,	 	// RUBY_T_HASH   = 0x08,
    dump_struct,	// RUBY_T_STRUCT = 0x09,
    oj_dump_bignum,	// RUBY_T_BIGNUM = 0x0a,
    NULL, 		// RUBY_T_FILE   = 0x0b,
    dump_data,		// RUBY_T_DATA   = 0x0c,
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
    
    if (MAX_DEPTH < depth) {
	rb_raise(rb_eNoMemError, "Too deeply nested.\n");
    }
    if (0 < type && type <= RUBY_T_FIXNUM) {
	DumpFunc	f = compat_funcs[type];

	if (NULL != f) {
	    f(obj, depth, out, Yes == out->opts->as_json);
	    return;
	}
    }
    oj_dump_nil(Qnil, depth, out, false);
}
