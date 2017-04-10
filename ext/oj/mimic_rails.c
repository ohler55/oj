/* mimic_rails.c
 * Copyright (c) 2017, Peter Ohler
 * All rights reserved.
 */

#include "dump.h"
#include "mimic_rails.h"
#include "encode.h"

// TBD keep static array of strings and functions to help with rails optimization
typedef struct _Encoder {
    struct _ROptTable	ropts;
    struct _Options	opts;
    VALUE		arg;
} *Encoder;

static struct _ROptTable	ropts = { 0, 0, NULL };

static VALUE	encoder_class = Qnil;

static ROpt	create_opt(ROptTable rot, VALUE clas);

ROpt
oj_rails_get_opt(ROptTable rot, VALUE clas) {
    if (NULL == rot) {
	rot = &ropts;
    }
    if (0 < rot->len) {
	int	lo = 0;
	int	hi = rot->len - 1;
	int	mid;
	VALUE	v;

	if (clas < rot->table->clas || rot->table[hi].clas < clas) {
	    return NULL;
	}
	if (rot->table[lo].clas == clas) {
	    return rot->table;
	}
	if (rot->table[hi].clas == clas) {
	    return &rot->table[hi];
	}
	while (2 <= hi - lo) {
	    mid = (hi + lo) / 2;
	    v = rot->table[mid].clas;
	    if (v == clas) {
		return &rot->table[mid];
	    }
	    if (v < clas) {
		lo = mid;
	    } else {
		hi = mid;
	    }
	}
    }
    return NULL;
}

static ROptTable
copy_opts(ROptTable src, ROptTable dest) {
    dest->len = src->len;
    dest->alen = src->alen;
    if (NULL == src->table) {
	dest->table = NULL;
    } else {
	dest->table = ALLOC_N(struct _ROpt, dest->alen);
	memcpy(dest->table, src->table, sizeof(struct _ROpt) * dest->alen);
    }
    return NULL;
}

static int
dump_attr_cb(ID key, VALUE value, Out out) {
    int		depth = out->depth;
    size_t	size = depth * out->indent + 1;
    const char	*attr = rb_id2name(key);

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
    oj_dump_rails_val(value, depth, out, true);
    out->depth = depth;
    *out->cur++ = ',';
    
    return ST_CONTINUE;
}

static void
dump_obj_attrs(VALUE obj, int depth, Out out, bool as_ok) {
    assure_size(out, 2);
    *out->cur++ = '{';
    out->depth = depth + 1;
    rb_ivar_foreach(obj, dump_attr_cb, (VALUE)out);
    if (',' == *(out->cur - 1)) {
	out->cur--; // backup to overwrite last comma
    }
    out->depth = depth;
    fill_indent(out, depth);
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
dump_struct(VALUE obj, int depth, Out out, bool as_ok) {
    int			d3 = depth + 2;
    size_t		size = d3 * out->indent + 2;
    size_t		sep_len = out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2;
    volatile VALUE	ma;
    volatile VALUE	v;
    int			cnt;
    int			i;
    int			len;
    const char		*name;

#ifdef RSTRUCT_LEN
#if UNIFY_FIXNUM_AND_BIGNUM
    cnt = (int)NUM2LONG(RSTRUCT_LEN(obj));
#else // UNIFY_FIXNUM_AND_INTEGER
    cnt = (int)RSTRUCT_LEN(obj);
#endif // UNIFY_FIXNUM_AND_INTEGER
#else
    // This is a bit risky as a struct in C ruby is not the same as a Struct
    // class in interpreted Ruby so length() may not be defined.
    cnt = FIX2INT(rb_funcall(obj, oj_length_id, 0));
#endif
    ma = rb_struct_s_members(rb_obj_class(obj));
    assure_size(out, 2);
    *out->cur++ = '{';
    for (i = 0; i < cnt; i++) {
	name = rb_id2name(SYM2ID(rb_ary_entry(ma, i)));
	len = strlen(name);
	assure_size(out, size + sep_len + 6);
	if (0 < i) {
	    *out->cur++ = ',';
	}
	fill_indent(out, d3);
	*out->cur++ = '"';
	memcpy(out->cur, name, len);
	out->cur += len;
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
#ifdef RSTRUCT_LEN
	v = RSTRUCT_GET(obj, i);
#else
	v = rb_struct_aref(obj, INT2FIX(i));
#endif
	oj_dump_rails_val(v, d3, out, true);
    }
    fill_indent(out, depth);
    *out->cur++ = '}';
    *out->cur = '\0';
}


static ID	to_a_id = 0;

static void
dump_enumerable(VALUE obj, int depth, Out out, bool as_ok) {
    if (0 == to_a_id) {
	to_a_id = rb_intern("to_a");
    }
    oj_dump_rails_val(rb_funcall(obj, to_a_id, 0), depth, out, false);
}

static void
dump_bigdecimal(VALUE obj, int depth, Out out, bool as_ok) {
    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);
    const char		*str = rb_string_value_ptr((VALUE*)&rstr);

    if ('I' == *str || 'N' == *str || ('-' == *str && 'I' == str[1])) {
	oj_dump_nil(Qnil, depth, out, false);
    } else {
	oj_dump_cstr(str, RSTRING_LEN(rstr), 0, 0, out);
    }
}

static void
dump_time(VALUE obj, int depth, Out out, bool as_ok) {
    oj_dump_xml_time(obj, out);
}

static void
dump_to_s(VALUE obj, int depth, Out out, bool as_ok) {
    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
}

typedef struct _NamedFunc {
    const char	*name;
    DumpFunc	func;
} *NamedFunc;

static struct _NamedFunc	dump_map[] = {
    { "BigDecimal", dump_bigdecimal },
    { "Range", dump_to_s },
    { "Regexp", dump_to_s },
    { "Time", dump_time },
    { NULL, NULL },
};

static ROpt
create_opt(ROptTable rot, VALUE clas) {
    ROpt	ro;
    NamedFunc	nf;
    const char	*classname = rb_class2name(clas);
    int		olen = rot->len;
    
    rot->len++;
    if (NULL == rot->table) {
	rot->alen = 256;
	rot->table = ALLOC_N(struct _ROpt, rot->alen);
	memset(rot->table, 0, sizeof(struct _ROpt) * rot->alen);
    } else if (rot->alen <= rot->len) {
	rot->alen *= 2;
	REALLOC_N(rot->table, struct _ROpt, rot->alen);
	memset(rot->table + olen, 0, sizeof(struct _ROpt) * olen);
    }
    if (0 == olen) {
	ro = rot->table;
    } else if (rot->table[olen - 1].clas < clas) {
	ro = &rot->table[olen];
    } else {
	int	i;
	
	for (i = 0, ro = rot->table; i < olen; i++, ro++) {
	    if (clas < ro->clas) {
		memmove(ro + 1, ro, sizeof(struct _ROpt) * (olen - i));
		break;
	    }
	}
    }
    ro->clas = clas;
    ro->on = true;
    ro->dump = dump_obj_attrs;
    for (nf = dump_map; NULL != nf->name; nf++) {
	if (0 == strcmp(nf->name, classname)) {
	    ro->dump = nf->func;
	    break;
	}
    }
    if (ro->dump == dump_obj_attrs) {
	if (Qtrue == rb_class_inherited_p(clas, rb_cStruct)) { // check before enumerable
	    ro->dump = dump_struct;
	} else if (Qtrue == rb_class_inherited_p(clas, rb_mEnumerable)) {
	    ro->dump = dump_enumerable;
	} else if (Qtrue == rb_class_inherited_p(clas, rb_eException)) {
	    ro->dump = dump_to_s;
	}
    }
    return NULL;
}

static void
encoder_free(void *ptr) {
    if (NULL != ptr) {
	Encoder	e = (Encoder)ptr;

	if (NULL != e->ropts.table) {
	    xfree(e->ropts.table);
	}
	xfree(ptr);
    }
}

static void
encoder_mark(void *ptr) {
    if (NULL != ptr) {
	Encoder	e = (Encoder)ptr;

	if (Qnil != e->arg) {
	    rb_gc_mark(e->arg);
	}
    }
}

/* Document-class: Oj::Rails::Encoder
 * @!method new(options=nil)
 *
 * Creates a new Encoder.
 * @param options [Hash] formatting options
 */
static VALUE
encoder_new(int argc, VALUE *argv, VALUE self) {
    Encoder	e = ALLOC(struct _Encoder);

    e->opts = oj_default_options;
    e->arg = Qnil;
    copy_opts(&ropts, &e->ropts);
    
    if (1 <= argc && Qnil != *argv) {
	oj_parse_options(*argv, &e->opts);
	e->arg = *argv;
    }
    return Data_Wrap_Struct(encoder_class, encoder_mark, encoder_free, e);
}

static void
optimize(int argc, VALUE *argv, ROptTable rot, bool on) {
    ROpt	ro;
    
    if (0 == argc) {
	int	i;
	
	for (i = 0; i < rot->len; i++) {
	    rot->table[i].on = on;
	}
    }
    for (; 0 < argc; argc--, argv++) {
	if (rb_cHash == *argv) {
	    oj_rails_hash_opt = on;
	} else if (rb_cArray == *argv) {
	    oj_rails_array_opt = on;
	} else if (NULL != (ro = oj_rails_get_opt(rot, *argv)) ||
		   NULL != (ro = create_opt(rot, *argv))) {
	    ro->on = on;
	}
	// TBD recurse if there are subclasses
    }
}
/* @!method optimize(*classes)
 * 
 * Use Oj rails optimized routines to encode the specified classes. This
 * ignores the as_json() method on the class and uses an internal encoding
 * instead. Passing in no classes indicates all should use the optimized
 * version of encoding for all previously optimized classes. Passing in the
 * Object class set a global switch that will then use the optimized behavior
 * for all classes.
 * 
 * @param classes [Class] a list of classes to optimize
 */
static VALUE
encoder_optimize(int argc, VALUE *argv, VALUE self) {
    Encoder	e = (Encoder)DATA_PTR(self);

    optimize(argc, argv, &e->ropts, true);

    return Qnil;
}

static VALUE
rails_optimize(int argc, VALUE *argv, VALUE self) {
    optimize(argc, argv, &ropts, true);

    return Qnil;
}

/* @!method deoptimize(*classes)
 * 
 * Turn off Oj rails optimization on the specified classes.
 *
 * @param classes [Class] a list of classes to deoptimize
 */
static VALUE
encoder_deoptimize(int argc, VALUE *argv, VALUE self) {
    Encoder	e = (Encoder)DATA_PTR(self);

    optimize(argc, argv, &e->ropts, false);

    return Qnil;
}

static VALUE
rails_deoptimize(int argc, VALUE *argv, VALUE self) {
    optimize(argc, argv, &ropts, false);

    return Qnil;
}

/* @!method optimized?(clas)
 * 
 * @param clas [Class] Class to check
 *
 * @return true if the class is being optimized for rails and false otherwise
 */
static VALUE
encoder_optimized(VALUE self, VALUE clas) {
    Encoder	e = (Encoder)DATA_PTR(self);
    ROpt	ro = oj_rails_get_opt(&e->ropts, clas);

    if (NULL == ro) {
	return Qfalse;
    }
    return (ro->on) ? Qtrue : Qfalse;
}

static VALUE
rails_optimized(VALUE self, VALUE clas) {
    ROpt	ro = oj_rails_get_opt(&ropts, clas);

    if (NULL == ro) {
	return Qfalse;
    }
    return (ro->on) ? Qtrue : Qfalse;
}

typedef struct _OO {
    Out		out;
    VALUE	obj;
} *OO;

static VALUE
protect_dump(VALUE ov) {
    OO	oo = (OO)ov;

    oj_dump_rails_val(oo->obj, 0, oo->out, true);

    return Qnil;
}

static VALUE
encode(VALUE obj, ROptTable ropts, Options opts, int argc, VALUE *argv) {
    char		buf[4096];
    struct _Out		out;
    struct _Options	copts = *opts;
    VALUE		rstr = Qnil;
    struct _OO		oo;
    int			line = 0;

    oo.out = &out;
    oo.obj = obj;
    copts.str_rx.head = NULL;
    copts.str_rx.tail = NULL;
    copts.mode = RailsMode;
    copts.escape_mode = JXEsc;
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
    out.ropts = ropts;
    if (Yes == copts.circular) {
	oj_cache8_new(&out.circ_cache);
    }
    //oj_dump_rails_val(*argv, 0, &out, true);
    rb_protect(protect_dump, (VALUE)&oo, &line);

    if (0 == line) {
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

	if (0 == out.buf) {
	    rb_raise(rb_eNoMemError, "Not enough memory.");
	}
	rstr = rb_str_new2(out.buf);
	rstr = oj_encode(rstr);
    }
    if (Yes == copts.circular) {
	oj_cache8_delete(out.circ_cache);
    }
    if (out.allocated) {
	xfree(out.buf);
    }
    if (0 != line) {
	rb_jump_tag(line);
    }
    return rstr;
}

/* @!method encode(obj)
 * 
 * @param obj [Object] object to encode
 *
 * @return encoded object as a JSON string.
 */
static VALUE
encoder_encode(VALUE self, VALUE obj) {
    Encoder	e = (Encoder)DATA_PTR(self);

    if (Qnil != e->arg) {
	VALUE	argv[1] = { e->arg };
	
	return encode(obj, &e->ropts, &e->opts, 1, argv);
    }
    return encode(obj, &e->ropts, &e->opts, 0, NULL);
}

/* Document-class: Oj::Rails::Encoder
 * @!method encode(obj, opts=nil)
 * 
 * Encode obj as a JSON String.
 * 
 * @param obj [Object|Hash|Array] object to convert to a JSON String
 * @param opts [Hash] options
 *
 * @return [String]
 */
static VALUE
rails_encode(int argc, VALUE *argv, VALUE self) {
    if (1 > argc) {
	rb_raise(rb_eArgError, "wrong number of arguments (0 for 1).");
    }
    if (1 == argc) {
	return encode(*argv, NULL, &oj_default_options, 0, NULL);
    } else {
	return encode(*argv, NULL, &oj_default_options, argc - 1, argv + 1);
    }
}

/* Document-module: Oj::Rails
 * 
 * Module that provides rails and active support compatibility.
 */
void
oj_mimic_rails_init(VALUE oj) {
    VALUE	rails = rb_define_module_under(oj, "Rails");
    
    rb_define_module_function(rails, "encode", rails_encode, -1);

    encoder_class = rb_define_class_under(rails, "Encoder", rb_cObject);
    rb_define_module_function(encoder_class, "new", encoder_new, -1);
    rb_define_module_function(rails, "optimize", rails_optimize, -1);
    rb_define_module_function(rails, "deoptimize", rails_deoptimize, -1);
    rb_define_module_function(rails, "optimized?", rails_optimized, 1);

    rb_define_method(encoder_class, "encode", encoder_encode, 1);
    rb_define_method(encoder_class, "optimize", encoder_optimize, -1);
    rb_define_method(encoder_class, "deoptimize", encoder_deoptimize, -1);
    rb_define_method(encoder_class, "optimized?", encoder_optimized, 1);
}
