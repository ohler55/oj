/* fast.c
 * Copyright (c) 2012, Peter Ohler
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  - Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 *  - Neither the name of Peter Ohler nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
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
#include <stdio.h>
#include <string.h>
#include <math.h>

#include "ruby.h"
#include "oj.h"

#define INIT_PATH_DEPTH	32

typedef struct _Leaf {
    struct _Leaf	*next;
    union {
	const char	*key;      // hash key
	size_t		index;     // array index
    };
    char		*str;      // pointer to location in json string
    struct _Leaf	*elements; // array and hash elements
    int			type;
    VALUE		value;
} *Leaf;

//#define BATCH_SIZE	(4096 / sizeof(struct _Leaf) - 1)
#define BATCH_SIZE	80

typedef struct _Batch {
    struct _Batch	*next;
    int			next_avail;
    struct _Leaf	leaves[BATCH_SIZE];
} *Batch;

typedef struct _Fast {
    Leaf		doc;
    Leaf		where_array[INIT_PATH_DEPTH];
    Leaf		*where;
    size_t		where_len; // length of allocated if longer than where_array
#ifdef HAVE_RUBY_ENCODING_H
    rb_encoding		*encoding;
#else
    void		*encoding;
#endif
    Batch		batches;
    struct _Batch	batch0;
} *Fast;

typedef struct _ParseInfo {
    char	*str;		/* buffer being read from */
    char	*s;		/* current position in buffer */
    Fast	fast;
} *ParseInfo;

static void	leaf_init(Leaf leaf, char *str, int type);
static Leaf	leaf_new(Fast fast, char *str, int type);
static void	leaf_append_element(Leaf parent, Leaf element);
static VALUE	leaf_value(Fast fast, Leaf leaf);
static void	leaf_fixnum_value(Leaf leaf);
static void	leaf_float_value(Leaf leaf);
static void	leaf_array_value(Fast fast, Leaf leaf);
static void	leaf_hash_value(Fast fast, Leaf leaf);

static Leaf	read_next(ParseInfo pi);
static Leaf	read_obj(ParseInfo pi);
static Leaf	read_array(ParseInfo pi);
static Leaf	read_str(ParseInfo pi);
static Leaf	read_num(ParseInfo pi);
static Leaf	read_true(ParseInfo pi);
static Leaf	read_false(ParseInfo pi);
static Leaf	read_nil(ParseInfo pi);
static void	next_non_white(ParseInfo pi);
static char*	read_quoted_value(ParseInfo pi);

static void	fast_init(Fast f);
static void	fast_free(Fast f);
static VALUE	fast_where(VALUE self);
static VALUE	fast_home(VALUE self);
static VALUE	fast_type(int argc, VALUE *argv, VALUE self);
static VALUE	fast_value_at(int argc, VALUE *argv, VALUE self);
static VALUE	fast_locate(int argc, VALUE *argv, VALUE self);
static VALUE	fast_move(int argc, VALUE *argv, VALUE self);
static VALUE	fast_each_branch(int argc, VALUE *argv, VALUE self);
static void	each_leaf(Fast f, Leaf leaf, VALUE *args);
static VALUE	fast_each_leaf(int argc, VALUE *argv, VALUE self);
static VALUE	fast_dump(VALUE self);

VALUE	oj_fast_class = 0;

inline static void
next_non_white(ParseInfo pi) {
    for (; 1; pi->s++) {
	switch(*pi->s) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	    break;
	default:
	    return;
	}
    }
}

inline static void
next_white(ParseInfo pi) {
    for (; 1; pi->s++) {
	switch(*pi->s) {
	case ' ':
	case '\t':
	case '\f':
	case '\n':
	case '\r':
	case '\0':
	    return;
	default:
	    break;
	}
    }
}

inline static void
leaf_init(Leaf leaf, char *str, int type) {
    leaf->next = 0;
    leaf->key = 0;
    leaf->str = str;
    leaf->elements = 0;
    leaf->type = type;
    leaf->value = Qundef;
}

static Leaf
leaf_new(Fast fast, char *str, int type) {
    Leaf	leaf;

    if (0 == fast->batches || BATCH_SIZE == fast->batches->next_avail) {
	Batch	b = ALLOC(struct _Batch);

	b->next = fast->batches;
	fast->batches = b;
	b->next_avail = 0;
    }
    leaf = &fast->batches->leaves[fast->batches->next_avail];
    fast->batches->next_avail++;
    leaf_init(leaf, str, type);

    return leaf;
}

static void
leaf_append_element(Leaf parent, Leaf element) {
    if (0 == parent->elements) {
	parent->elements = element;
	element->next = element;
    } else {
	element->next = parent->elements->next;
	parent->elements->next = element;
	parent->elements = element;
    }
}

static VALUE
leaf_value(Fast fast, Leaf leaf) {
    if (Qundef == leaf->value) {
	switch (leaf->type) {
	case T_NIL:
	    leaf->value = Qnil;
	    break;
	case T_TRUE:
	    leaf->value = Qtrue;
	    break;
	case T_FALSE:
	    leaf->value = Qfalse;
	    break;
	case T_FIXNUM:
	    leaf_fixnum_value(leaf);
	    break;
	case T_FLOAT:
	    leaf_float_value(leaf);
	    break;
	case T_STRING:
	    leaf->value = rb_str_new2(leaf->str);
#ifdef HAVE_RUBY_ENCODING_H
	    if (0 != fast->encoding) {
		rb_enc_associate(leaf->value, fast->encoding);
	    }
#endif
	    break;
	case T_ARRAY:
	    leaf_array_value(fast, leaf);
	    break;
	case T_HASH:
	    leaf_hash_value(fast, leaf);
	    break;
	default:
	    rb_raise(rb_eTypeError, "Unexpected type %02x.", leaf->type);
	    break;
	}
    }
    return leaf->value;
}

#ifdef RUBINIUS
#define NUM_MAX 0x07FFFFFF
#else
#define NUM_MAX (FIXNUM_MAX >> 8)
#endif


static void
leaf_fixnum_value(Leaf leaf) {
    char	*s = leaf->str;
    int64_t	n = 0;
    int		neg = 0;
    int		big = 0;

    if ('-' == *s) {
	s++;
	neg = 1;
    } else if ('+' == *s) {
	s++;
    }
    for (; '0' <= *s && *s <= '9'; s++) {
	n = n * 10 + (*s - '0');
	if (NUM_MAX <= n) {
	    big = 1;
	}
    }
    if (big) {
	char	c = *s;
	
	*s = '\0';
	leaf->value = rb_cstr_to_inum(leaf->str, 10, 0);
	*s = c;
    } else {
	if (neg) {
	    n = -n;
	}
	leaf->value = LONG2NUM(n);
    }
}

#if 1
static void
leaf_float_value(Leaf leaf) {
    leaf->value = DBL2NUM(rb_cstr_to_dbl(leaf->str, 1));
}
#else
static void
leaf_float_value(Leaf leaf) {
    char	*s = leaf->str;
    int64_t	n = 0;
    long	a = 0;
    long	div = 1;
    long	e = 0;
    int		neg = 0;
    int		eneg = 0;
    int		big = 0;

    if ('-' == *s) {
	s++;
	neg = 1;
    } else if ('+' == *s) {
	s++;
    }
    for (; '0' <= *s && *s <= '9'; s++) {
	n = n * 10 + (*s - '0');
	if (NUM_MAX <= n) {
	    big = 1;
	}
    }
    if (big) {
	char	c = *s;
	
	*s = '\0';
	leaf->value = rb_cstr_to_inum(leaf->str, 10, 0);
	*s = c;
    } else {
	double	d;

	if ('.' == *s) {
	    s++;
	    for (; '0' <= *s && *s <= '9'; s++) {
		a = a * 10 + (*s - '0');
		div *= 10;
	    }
	}
	if ('e' == *s || 'E' == *s) {
	    s++;
	    if ('-' == *s) {
		s++;
		eneg = 1;
	    } else if ('+' == *s) {
		s++;
	    }
	    for (; '0' <= *s && *s <= '9'; s++) {
		e = e * 10 + (*s - '0');
	    }
	}
	d = (double)n + (double)a / (double)div;
	if (neg) {
	    d = -d;
	}
	if (0 != e) {
	    if (eneg) {
		e = -e;
	    }
	    d *= pow(10.0, e);
	}
	leaf->value = DBL2NUM(d);
    }
}
#endif

static void
leaf_array_value(Fast fast, Leaf leaf) {
    VALUE	a = rb_ary_new();

    if (0 != leaf->elements) {
	Leaf	first = leaf->elements->next;
	Leaf	e = first;

	do {
	    rb_ary_push(a, leaf_value(fast, e));
	    e = e->next;
	} while (e != first);
    }
    leaf->value = a;
}

static void
leaf_hash_value(Fast fast, Leaf leaf) {
    VALUE	h = rb_hash_new();

    if (0 != leaf->elements) {
	Leaf	first = leaf->elements->next;
	Leaf	e = first;
	VALUE	key;

	do {
	    key = rb_str_new2(e->key);
#ifdef HAVE_RUBY_ENCODING_H
	    if (0 != fast->encoding) {
		rb_enc_associate(key, fast->encoding);
	    }
#endif
	    rb_hash_aset(h, key, leaf_value(fast, e));
	    e = e->next;
	} while (e != first);
    }
    leaf->value = h;
}

static Leaf
read_next(ParseInfo pi) {
    Leaf	leaf = 0;

    next_non_white(pi);	// skip white space
    switch (*pi->s) {
    case '{':
	leaf = read_obj(pi);
	break;
    case '[':
	leaf = read_array(pi);
	break;
    case '"':
	leaf = read_str(pi);
	break;
    case '+':
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
	leaf = read_num(pi);
	break;
    case 't':
	leaf = read_true(pi);
	break;
    case 'f':
	leaf = read_false(pi);
	break;
    case 'n':
	leaf = read_nil(pi);
	break;
    case '\0':
    default:
	break; // returns 0
    }
    return leaf;
}

static Leaf
read_obj(ParseInfo pi) {
    Leaf	h = leaf_new(pi->fast, pi->s, T_HASH);
    char	*end;
    const char	*key = 0;
    Leaf	val = 0;

    pi->s++;
    next_non_white(pi);
    if ('}' == *pi->s) {
	pi->s++;
	return h;
    }
    while (1) {
	next_non_white(pi);
	key = 0;
	val = 0;
	if ('"' != *pi->s || 0 == (key = read_quoted_value(pi))) {
	    raise_error("unexpected character", pi->str, pi->s);
	}
	next_non_white(pi);
	if (':' == *pi->s) {
	    pi->s++;
	} else {
	    raise_error("invalid format, expected :", pi->str, pi->s);
	}
	if (0 == (val = read_next(pi))) {
	    //printf("*** '%s'\n", pi->s);
	    raise_error("unexpected character", pi->str, pi->s);
	}
	end = pi->s;
	val->key = key;
	leaf_append_element(h, val);
	next_non_white(pi);
	if ('}' == *pi->s) {
	    pi->s++;
	    *end = '\0';
	    break;
	} else if (',' == *pi->s) {
	    pi->s++;
	} else {
	    printf("*** '%s'\n", pi->s);
	    raise_error("invalid format, expected , or } while in an object", pi->str, pi->s);
	}
	*end = '\0';
    }
    return h;
}

static Leaf
read_array(ParseInfo pi) {
    Leaf	a = leaf_new(pi->fast, pi->s, T_ARRAY);
    Leaf	e;
    char	*end;
    size_t	cnt = 0;

    pi->s++;
    next_non_white(pi);
    if (']' == *pi->s) {
	pi->s++;
	return a;
    }
    while (1) {
	next_non_white(pi);
	if (0 == (e = read_next(pi))) {
	    raise_error("unexpected character", pi->str, pi->s);
	}
	e->index = cnt++;
	leaf_append_element(a, e);
	end = pi->s;
	next_non_white(pi);
	if (',' == *pi->s) {
	    pi->s++;
	} else if (']' == *pi->s) {
	    pi->s++;
	    *end = '\0';
	    break;
	} else {
	    raise_error("invalid format, expected , or ] while in an array", pi->str, pi->s);
	}
	*end = '\0';
    }
    return a;
}

static Leaf
read_str(ParseInfo pi) {
    return leaf_new(pi->fast, read_quoted_value(pi), T_STRING);
}

static Leaf
read_num(ParseInfo pi) {
    char	*start = pi->s;
    int		type = T_FIXNUM;

    if ('-' == *pi->s) {
	pi->s++;
    }
    // digits
    for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
    }
    if ('.' == *pi->s) {
	type = T_FLOAT;
	pi->s++;
	for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	}
    }
    if ('e' == *pi->s || 'E' == *pi->s) {
	pi->s++;
	if ('-' == *pi->s || '+' == *pi->s) {
	    pi->s++;
	}
	for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	}
    }
    return leaf_new(pi->fast, start, type);
}

static Leaf
read_true(ParseInfo pi) {
    Leaf	leaf = leaf_new(pi->fast, pi->s, T_TRUE);

    pi->s++;
    if ('r' != *pi->s || 'u' != *(pi->s + 1) || 'e' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'true'", pi->str, pi->s);
    }
    pi->s += 3;

    return leaf;
}

static Leaf
read_false(ParseInfo pi) {
    Leaf	leaf = leaf_new(pi->fast, pi->s, T_FALSE);

    pi->s++;
    if ('a' != *pi->s || 'l' != *(pi->s + 1) || 's' != *(pi->s + 2) || 'e' != *(pi->s + 3)) {
	raise_error("invalid format, expected 'false'", pi->str, pi->s);
    }
    pi->s += 4;

    return leaf;
}

static Leaf
read_nil(ParseInfo pi) {
    Leaf	leaf = leaf_new(pi->fast, pi->s, T_NIL);

    pi->s++;
    if ('u' != *pi->s || 'l' != *(pi->s + 1) || 'l' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'nil'", pi->str, pi->s);
    }
    pi->s += 3;

    return leaf;
}

static char
read_hex(ParseInfo pi, char *h) {
    uint8_t	b = 0;

    if ('0' <= *h && *h <= '9') {
	b = *h - '0';
    } else if ('A' <= *h && *h <= 'F') {
	b = *h - 'A' + 10;
    } else if ('a' <= *h && *h <= 'f') {
	b = *h - 'a' + 10;
    } else {
	pi->s = h;
	raise_error("invalid hex character", pi->str, pi->s);
    }
    h++;
    b = b << 4;
    if ('0' <= *h && *h <= '9') {
	b += *h - '0';
    } else if ('A' <= *h && *h <= 'F') {
	b += *h - 'A' + 10;
    } else if ('a' <= *h && *h <= 'f') {
	b += *h - 'a' + 10;
    } else {
	pi->s = h;
	raise_error("invalid hex character", pi->str, pi->s);
    }
    return (char)b;
}

/* Assume the value starts immediately and goes until the quote character is
 * reached again. Do not read the character after the terminating quote.
 */
static char*
read_quoted_value(ParseInfo pi) {
    char	*value = 0;
    char	*h = pi->s; // head
    char	*t = h;     // tail
    
    h++;	// skip quote character
    t++;
    value = h;
    for (; '"' != *h; h++, t++) {
	if ('\0' == *h) {
	    pi->s = h;
	    raise_error("quoted string not terminated", pi->str, pi->s);
	} else if ('\\' == *h) {
	    h++;
	    switch (*h) {
	    case 'n':	*t = '\n';	break;
	    case 'r':	*t = '\r';	break;
	    case 't':	*t = '\t';	break;
	    case 'f':	*t = '\f';	break;
	    case 'b':	*t = '\b';	break;
	    case '"':	*t = '"';	break;
	    case '/':	*t = '/';	break;
	    case '\\':	*t = '\\';	break;
	    case 'u':
		h++;
		*t = read_hex(pi, h);
		h += 2;
		if ('\0' != *t) {
		    t++;
		}
		*t = read_hex(pi, h);
		h++;
		break;
	    default:
		pi->s = h;
		raise_error("invalid escaped character", pi->str, pi->s);
		break;
	    }
	} else if (t != h) {
	    *t = *h;
	}
    }
    *t = '\0'; // terminate value
    pi->s = h + 1;

    return value;
}

inline static void
fast_init(Fast f) {
    f->where = f->where_array;
    f->where_len = 0;
    *f->where = 0;
    f->doc = 0;
    f->batches = &f->batch0;
    f->batch0.next = 0;
    f->batch0.next_avail = 0;
}

static void
fast_free(Fast f) {
    if (0 != f) {
	Batch	b;

	while (0 != (b = f->batches)) {
	    f->batches = f->batches->next;
	    if (&f->batch0 != b) {
		xfree(b);
	    }
	}
	if (f->where_array != f->where) {
	    free(f->where);
	}
	//xfree(f);
    }
}

static VALUE
fast_open(VALUE clas, VALUE str) {
    struct _ParseInfo	pi;
    VALUE		result = Qnil;
    VALUE		fobj;
    size_t		len;
    struct _Fast	fast;

    Check_Type(str, T_STRING);
    if (!rb_block_given_p()) {
	rb_raise(rb_eArgError, "Block or Proc is required.");
    }
    len = RSTRING_LEN(str);
    pi.str = ALLOCA_N(char, len + 1);
    //pi.str = ALLOC_N(char, len + 1);
    memcpy(pi.str, StringValuePtr(str), len + 1);
    pi.s = pi.str;
    fobj = rb_obj_alloc(clas);
    fast_init(&fast);
#ifdef HAVE_RUBY_ENCODING_H
    fast.encoding = ('\0' == *oj_default_options.encoding) ? 0 : rb_enc_find(oj_default_options.encoding);
#else
    fast.encoding = 0;
#endif
    pi.fast = &fast;
    DATA_PTR(fobj) = pi.fast;
    pi.fast->doc = read_next(&pi); // parse
    result = rb_yield(fobj); // caller processing
    DATA_PTR(fobj) = 0;
    fast_free(pi.fast);
    //xfree(pi.str);
    
    return result;
}

static VALUE
fast_where(VALUE self) {
    Fast	f = DATA_PTR(self);

    if (0 == *f->where) {
	return oj_slash_string;
    } else {
	// TBD build path from where
    }
    return Qnil;
}

static VALUE
fast_home(VALUE self) {
    Fast	f = DATA_PTR(self);

    *f->where = 0;

    return oj_slash_string;
}

static VALUE
fast_type(int argc, VALUE *argv, VALUE self) {
    Fast	f = DATA_PTR(self);
    VALUE	type = Qnil;

    if (0 != f->doc) {
	Leaf	leaf = f->doc;
	//Leaf	leaf = get_leaf(f->doc, path);

	// TBD if there is an arg, check it is a string and then follow path
	switch (leaf->type) {
	case T_NIL:	type = rb_cNilClass;	break;
	case T_TRUE:	type = rb_cTrueClass;	break;
	case T_FALSE:	type = rb_cFalseClass;	break;
	case T_STRING:	type = rb_cString;	break;
	case T_FIXNUM:	type = rb_cFixnum;	break;
	case T_FLOAT:	type = rb_cFloat;	break;
	case T_ARRAY:	type = rb_cArray;	break;
	case T_HASH:	type = rb_cHash;	break;
	default:				break;
	}
    }
    return type;
}

static VALUE
fast_value_at(int argc, VALUE *argv, VALUE self) {
    Fast	f = DATA_PTR(self);
    VALUE	val = Qnil;
    const char	*path = 0;

    if (1 <= argc) {
	Check_Type(*argv, T_STRING);
	path = StringValuePtr(*argv);
	if (2 == argc) {
	    val = argv[1];
	}
    }
    if (0 != f->doc) {
	Leaf	leaf = f->doc;

	// TBD use path
	val = leaf_value(f, leaf);
    }
    return val;
}

static VALUE
fast_locate(int argc, VALUE *argv, VALUE self) {
    // TBD
    return Qnil;
}

static VALUE
fast_move(int argc, VALUE *argv, VALUE self) {
    // TBD
    return Qnil;
}

static VALUE
fast_each_branch(int argc, VALUE *argv, VALUE self) {
    // TBD
    return Qnil;
}

static void
each_leaf(Fast f, Leaf leaf, VALUE *args) {
    if (T_ARRAY == leaf->type || T_HASH == leaf->type) {
	if (0 != leaf->elements) {
	    Leaf	first = leaf->elements->next;
	    Leaf	e = first;

	    do {
		each_leaf(f, e, args);
		e = e->next;
	    } while (e != first);
	}
    } else {
	args[1] = leaf_value(f, leaf);
	rb_yield_values2(2, args);
    }
}

static VALUE
fast_each_leaf(int argc, VALUE *argv, VALUE self) {
    if (rb_block_given_p()) {
	Fast	f = DATA_PTR(self);
	VALUE	args[2];

	args[0] = self;
	args[1] = Qnil;
	each_leaf(f, f->doc, args);
    }
    return Qnil;
}

// TBD improve later to be more direct for higher performance
static VALUE
fast_dump(VALUE self) {
    Fast	f = DATA_PTR(self);
    VALUE	obj = leaf_value(f, f->doc);
    const char	*json;
    
    json = oj_write_obj_to_str(obj, &oj_default_options);
    
    return rb_str_new2(json);
}

void
oj_init_fast() {
    oj_fast_class = rb_define_class_under(Oj, "Fast", rb_cObject);
    rb_define_singleton_method(oj_fast_class, "open", fast_open, 1);
    rb_define_singleton_method(oj_fast_class, "parse", fast_open, 1);
    rb_define_method(oj_fast_class, "where?", fast_where, 0);
    rb_define_method(oj_fast_class, "home", fast_home, 0);
    rb_define_method(oj_fast_class, "type", fast_type, -1);
    rb_define_method(oj_fast_class, "value_at", fast_value_at, -1);
    rb_define_method(oj_fast_class, "locate", fast_locate, -1);
    rb_define_method(oj_fast_class, "move", fast_move, -1);
    rb_define_method(oj_fast_class, "each_branch", fast_each_branch, -1);
    rb_define_method(oj_fast_class, "each_leaf", fast_each_leaf, -1);
    rb_define_method(oj_fast_class, "dump", fast_dump, 0);
}
