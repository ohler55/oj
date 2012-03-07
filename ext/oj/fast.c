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

#define BATCH_SIZE	32
#define INIT_PATH_DEPTH	32

typedef struct _Leaf {
    struct _Leaf	*next;
    union {
	const char	*key;      // hash key
	size_t		index;     // array index
    };
    const char		*str;      // pointer to location in json string
    struct _Leaf	*elements; // array and hash elements
    int			type;
    VALUE		value;
} *Leaf;

typedef struct _Batch {
    struct _Batch	*next;
    int			next_avail;
    struct _Leaf	leaves[BATCH_SIZE];
} *Batch;

typedef struct _Fast {
    Batch	batches;
    Leaf	doc;
    Leaf	where_array[INIT_PATH_DEPTH];
    Leaf	*where;
    size_t	where_len; // length of allocated if longer than where_array
} *Fast;

typedef struct _ParseInfo {
    char	*str;		/* buffer being read from */
    char	*s;		/* current position in buffer */
#ifdef HAVE_RUBY_ENCODING_H
    rb_encoding	*encoding;
#else
    void	*encoding;
#endif
    Fast	fast;
} *ParseInfo;

static void	leaf_init(Leaf leaf, const char *str, int type);
static Leaf	leaf_new(Fast fast, const char *str, int type);
static void	leaf_append_element(Leaf parent, Leaf element);

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

static void	fast_free(void *ptr);
static VALUE	fast_where(VALUE self);
static VALUE	fast_home(VALUE self);
static VALUE	fast_type(int argc, VALUE *argv, VALUE self);
static VALUE	fast_value_at(int argc, VALUE *argv, VALUE self);
static VALUE	fast_locate(int argc, VALUE *argv, VALUE self);
static VALUE	fast_move(int argc, VALUE *argv, VALUE self);
static VALUE	fast_each_branch(int argc, VALUE *argv, VALUE self);
static VALUE	fast_each_leaf(int argc, VALUE *argv, VALUE self);
static VALUE	fast_jsonpath_to_path(int argc, VALUE *argv, VALUE self);
static VALUE	fast_inspect(VALUE self);

VALUE	oj_fast_class = 0;

/* This XML parser is a single pass, destructive, callback parser. It is a
 * single pass parse since it only make one pass over the characters in the
 * XML document string. It is destructive because it re-uses the content of
 * the string for values in the callback and places \0 characters at various
 * places to mark the end of tokens and strings. It is a callback parser like
 * a SAX parser because it uses callback when document elements are
 * encountered.
 *
 * Parsing is very tolerant. Lack of headers and even mispelled element
 * endings are passed over without raising an error. A best attempt is made in
 * all cases to parse the string.
 */

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
leaf_init(Leaf leaf, const char *str, int type) {
    leaf->next = 0;
    leaf->key = 0;
    leaf->str = str;
    leaf->elements = 0;
    leaf->type = type;
    leaf->value = Qundef;
}

static Leaf
leaf_new(Fast fast, const char *str, int type) {
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
	parent->elements = element;
    }
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
	    printf("*** '%s'\n", pi->s);
	    raise_error("unexpected character", pi->str, pi->s);
	}
	end = pi->s;
	val->key = key;
	leaf_append_element(h, val);
	next_non_white(pi);
	if ('}' == *pi->s) {
	    pi->s++;
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
    return leaf_new(pi->fast, pi->s, type);
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

static void
fast_free(void *ptr) {
    if (0 != ptr) {
	Fast	fast = (Fast)ptr;
	Batch	b;

	while (0 != (b = fast->batches)) {
	    fast->batches = fast->batches->next;
	    xfree(b);
	}
	if (fast->where_array != fast->where) {
	    free(fast->where);
	}
	xfree(fast);
    }
}

static VALUE
fast_open(VALUE clas, VALUE str) {
    struct _ParseInfo	pi;
    VALUE		result = Qnil;
    VALUE		fobj;
    size_t		len;

    Check_Type(str, T_STRING);
    len = RSTRING_LEN(str);
    pi.str = ALLOC_N(char, len);
    memcpy(pi.str, StringValuePtr(str), len);
    pi.s = pi.str;
    pi.encoding = 0;
#ifdef HAVE_RUBY_ENCODING_H
    //    pi.encoding = ('\0' == *options->encoding) ? 0 : rb_enc_find(options->encoding);
#else
    pi.encoding = 0;
#endif
    fobj = rb_obj_alloc(clas);
    pi.fast = ALLOC(struct _Fast);
    DATA_PTR(fobj) = pi.fast;
    pi.fast->batches = 0;
    pi.fast->where = pi.fast->where_array;
    pi.fast->where_len = 0;
    *pi.fast->where = 0;
    pi.fast->doc = read_next(&pi);
    if (rb_block_given_p()) {
	result = rb_yield(fobj);
    }
    DATA_PTR(fobj) = 0;
    fast_free(pi.fast);
    xfree(pi.str);
    
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
    return Qnil;
}

static VALUE
fast_locate(int argc, VALUE *argv, VALUE self) {
    return Qnil;
}

static VALUE
fast_move(int argc, VALUE *argv, VALUE self) {
    return Qnil;
}

static VALUE
fast_each_branch(int argc, VALUE *argv, VALUE self) {
    return Qnil;
}

static VALUE
fast_each_leaf(int argc, VALUE *argv, VALUE self) {
    return Qnil;
}

static VALUE
fast_jsonpath_to_path(int argc, VALUE *argv, VALUE self) {
    return Qnil;
}

static VALUE
fast_inspect(VALUE self) {
    return Qnil;
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
    rb_define_method(oj_fast_class, "jsonpath_to_path", fast_jsonpath_to_path, -1);
    rb_define_method(oj_fast_class, "inspect", fast_inspect, 0);
}
