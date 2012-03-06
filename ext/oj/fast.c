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

typedef struct _KeyVal {
    const char		*key;
    struct _Leaf	*val;
} *KeyVal;

typedef struct _Leaf {
    struct _Leaf	*next;
    union {
	const char	*str;
	KeyVal		entries;
	struct _Leaf	*elements; // array elements
    };
    int			type;
    VALUE		value;
} *Leaf;

typedef struct _Fast {
    const char		*str;
    struct _Leaf	doc;
} *Fast;

typedef struct _ParseInfo {
    char	*str;		/* buffer being read from */
    char	*s;		/* current position in buffer */
#ifdef HAVE_RUBY_ENCODING_H
    rb_encoding	*encoding;
#else
    void	*encoding;
#endif
} *ParseInfo;

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
    Leaf	h = 0; // TBD create Hash Leaf
    const char	*key = 0;
    Leaf	val = 0;

    pi->s++;
    next_non_white(pi);
    if ('}' == *pi->s) {
	pi->s++;
	// TBD create and return Array Leaf
	return 0;
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
	val = read_next(pi);
#if 0
	if (0 == (val = read_next(pi))) {
	    printf("*** '%s'\n", pi->s);
	    raise_error("unexpected character", pi->str, pi->s);
	}
#endif
	// TBD add key/val to leaf
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
    }
    return h;
}

static Leaf
read_array(ParseInfo pi) {
    Leaf	a = 0; // TBD allocate Leaf
    //Leaf	e;

    pi->s++;
    next_non_white(pi);
    if (']' == *pi->s) {
	pi->s++;
	return a;
    }
    while (1) {
	next_non_white(pi);
	read_next(pi);
#if 0
	if (0 == (e = read_next(pi))) {
	    raise_error("unexpected character", pi->str, pi->s);
	}
#endif
	// TBD add to array
	next_non_white(pi);
	if (',' == *pi->s) {
	    pi->s++;
	} else if (']' == *pi->s) {
	    pi->s++;
	    break;
	} else {
	    raise_error("invalid format, expected , or ] while in an array", pi->str, pi->s);
	}
    }
    return a;
}

static Leaf
read_str(ParseInfo pi) {
    char	*text;
    Leaf	leaf = 0; // TBD create leaf

    text = read_quoted_value(pi);

    return leaf;
}

#ifdef RUBINIUS
#define NUM_MAX 0x07FFFFFF
#else
#define NUM_MAX (FIXNUM_MAX >> 8)
#endif

// TBD change to read into a string only
static Leaf
read_num(ParseInfo pi) {
    char	c;

    for (; 1; pi->s++) {
	c = *pi->s;
	if (!(('0' <= c && c <= '9') || '-' == c || 'e' == c || 'E' == c || '.' == c)) {
	    break;
	}
    }
    return 0;
}

static Leaf
read_true(ParseInfo pi) {
    pi->s++;
    if ('r' != *pi->s || 'u' != *(pi->s + 1) || 'e' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'true'", pi->str, pi->s);
    }
    pi->s += 3;

    return 0; // TBD return Leaf
}

static Leaf
read_false(ParseInfo pi) {
    pi->s++;
    if ('a' != *pi->s || 'l' != *(pi->s + 1) || 's' != *(pi->s + 2) || 'e' != *(pi->s + 3)) {
	raise_error("invalid format, expected 'false'", pi->str, pi->s);
    }
    pi->s += 4;

    return 0; // TBD return Leaf
}

static Leaf
read_nil(ParseInfo pi) {
    pi->s++;
    if ('u' != *pi->s || 'l' != *(pi->s + 1) || 'l' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'nil'", pi->str, pi->s);
    }
    pi->s += 3;

    return 0; // TBD return Leaf
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

static VALUE
fast_initialize(VALUE self, VALUE str) {
    struct _ParseInfo	pi;
    Leaf	content;

    Check_Type(str, T_STRING);

    pi.str = strdup(StringValuePtr(str));
    pi.s = pi.str;
    pi.encoding = 0;
#ifdef HAVE_RUBY_ENCODING_H
    //    pi.encoding = ('\0' == *options->encoding) ? 0 : rb_enc_find(options->encoding);
#else
    pi.encoding = 0;
#endif
    content = read_next(&pi);

    return Qnil;
}

static VALUE
fast_where(VALUE self) {
    return Qnil;
}

static VALUE
fast_home(VALUE self) {
    return Qnil;
}

static VALUE
fast_type(int argc, VALUE *argv, VALUE self) {
    return Qnil;
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

void
oj_init_fast() {
    oj_fast_class = rb_define_class_under(Oj, "Fast", rb_cObject);
    rb_define_method(oj_fast_class, "initialize", fast_initialize, 1);
    rb_define_method(oj_fast_class, "where?", fast_where, 0);
    rb_define_method(oj_fast_class, "home", fast_home, 0);
    rb_define_method(oj_fast_class, "type", fast_type, -1);
    rb_define_method(oj_fast_class, "value_at", fast_value_at, -1);
    rb_define_method(oj_fast_class, "locate", fast_locate, -1);
    rb_define_method(oj_fast_class, "move", fast_move, -1);
    rb_define_method(oj_fast_class, "each_branch", fast_each_branch, -1);
    rb_define_method(oj_fast_class, "each_leaf", fast_each_leaf, -1);
    rb_define_method(oj_fast_class, "jsonpath_to_path", fast_jsonpath_to_path, -1);
}
