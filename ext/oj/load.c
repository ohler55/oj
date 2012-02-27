/* load.c
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

enum {
    TIME_HINT = 0x0100,
};

typedef struct _ParseInfo {
    char	*str;		/* buffer being read from */
    char	*s;		/* current position in buffer */
#ifdef HAVE_RUBY_ENCODING_H
    rb_encoding	*encoding;
#else
    void	*encoding;
#endif
    Options	options;
} *ParseInfo;

static VALUE	classname2class(const char *name, ParseInfo pi);
static VALUE	read_next(ParseInfo pi, int hint);
static VALUE	read_obj(ParseInfo pi);
static VALUE	read_array(ParseInfo pi);
static VALUE	read_str(ParseInfo pi, int hint);
static VALUE	read_num(ParseInfo pi);
static VALUE	read_time(ParseInfo pi);
static VALUE	read_true(ParseInfo pi);
static VALUE	read_false(ParseInfo pi);
static VALUE	read_nil(ParseInfo pi);
static void	next_non_white(ParseInfo pi);
static char*	read_quoted_value(ParseInfo pi);


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

inline static VALUE
resolve_classname(VALUE mod, const char *class_name, int auto_define) {
    VALUE       clas;
    ID          ci = rb_intern(class_name);

    if (rb_const_defined_at(mod, ci) || !auto_define) {
	clas = rb_const_get_at(mod, ci);
    } else {
	clas = rb_define_class_under(mod, class_name, oj_bag_class);
    }
    return clas;
}

inline static VALUE
classname2obj(const char *name, ParseInfo pi) {
    VALUE   clas = classname2class(name, pi);
    
    if (Qundef == clas) {
        return Qnil;
    } else {
        return rb_obj_alloc(clas);
    }
}

static VALUE
classname2class(const char *name, ParseInfo pi) {
    VALUE       clas;
    VALUE       *slot;
    int		auto_define = (Yes == pi->options->auto_define);

    if (Qundef == (clas = oj_cache_get(oj_class_cache, name, &slot))) {
        char            class_name[1024];
        char            *s;
        const char      *n = name;

        clas = rb_cObject;
        for (s = class_name; '\0' != *n; n++) {
            if (':' == *n) {
                *s = '\0';
                n++;
		if (':' != *n) {
                    raise_error("Invalid classname, expected another ':'", pi->str, pi->s);
		}
                if (Qundef == (clas = resolve_classname(clas, class_name, auto_define))) {
                    return Qundef;
                }
                s = class_name;
            } else {
                *s++ = *n;
            }
        }
        *s = '\0';
        if (Qundef != (clas = resolve_classname(clas, class_name, auto_define))) {
            *slot = clas;
        }
    }
    return clas;
}

VALUE
oj_parse(char *json, Options options) {
    VALUE		obj;
    struct _ParseInfo	pi;

    if (0 == json) {
	raise_error("Invalid arg, xml string can not be null", json, 0);
    }
    /* initialize parse info */
    pi.str = json;
    pi.s = json;
#ifdef HAVE_RUBY_ENCODING_H
    pi.encoding = ('\0' == *options->encoding) ? 0 : rb_enc_find(options->encoding);
#else
    pi.encoding = 0;
#endif
    pi.options = options;
    if (Qundef == (obj = read_next(&pi, 0))) {
	raise_error("no object read", pi.str, pi.s);
    }
    next_non_white(&pi);	// skip white space
    if ('\0' != *pi.s) {
	raise_error("invalid format, extra characters", pi.str, pi.s);
    }
    return obj;
}

static VALUE
read_next(ParseInfo pi, int hint) {
    VALUE	obj;

    next_non_white(pi);	// skip white space
    switch (*pi->s) {
    case '{':
	obj = read_obj(pi);
	break;
    case '[':
	obj = read_array(pi);
	break;
    case '"':
	obj = read_str(pi, hint);
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
	if (TIME_HINT == hint) {
	    obj = read_time(pi);
	} else {
	    obj = read_num(pi);
	}
    break;
    case 't':
	obj = read_true(pi);
	break;
    case 'f':
	obj = read_false(pi);
	break;
    case 'n':
	obj = read_nil(pi);
	break;
    case '\0':
	obj = Qundef;
	break;
    default:
	obj = Qundef;
	break;
    }
    return obj;
}

static VALUE
read_obj(ParseInfo pi) {
    VALUE	obj = Qundef;
    VALUE	key = Qundef;
    VALUE	val = Qundef;
    const char	*ks;
    int		obj_type = T_NONE;
    
    pi->s++;
    next_non_white(pi);
    if ('}' == *pi->s) {
	pi->s++;
	return rb_hash_new();
    }
    while (1) {
	next_non_white(pi);
	ks = 0;
	key = Qundef;
	val = Qundef;
	if ('"' != *pi->s || Qundef == (key = read_str(pi, 0))) {
	    raise_error("unexpected character", pi->str, pi->s);
	}
	next_non_white(pi);
	if (':' == *pi->s) {
	    pi->s++;
	} else {
	    raise_error("invalid format, expected :", pi->str, pi->s);
	}
	if (T_STRING == rb_type(key)) {
	    ks = StringValuePtr(key);
	} else {
	    ks = 0;
	}
	if (0 != ks && Qundef == obj && ObjectMode == pi->options->mode) {
	    if ('^' == *ks && '\0' == ks[2]) { // special directions
		switch (ks[1]) {
		case 't': // Time
		    obj = read_next(pi, TIME_HINT); // raises if can not convert to Time
		    key = Qundef;
		    break;
		case 'c': // Class
		    obj = read_next(pi, T_CLASS);
		    key = Qundef;
		    break;
		case 's': // Symbol
		    obj = read_next(pi, T_SYMBOL);
		    key = Qundef;
		    break;
		case 'o': // Object
		    obj = read_next(pi, T_OBJECT);
		    obj_type = T_OBJECT;
		    key = Qundef;
		    break;
		case 'i': // Id for circular reference
		    // TBD
		default:
		    // handle later
		    break;
		}
	    }
	}
	if (Qundef != key) {
	    if (Qundef == val && Qundef == (val = read_next(pi, 0))) {
		raise_error("unexpected character", pi->str, pi->s);
	    }
	    if (ObjectMode == pi->options->mode &&
		0 != ks && '^' == *ks && '#' == ks[1] &&
		(T_NONE == obj_type || T_HASH == obj_type) &&
		rb_type(val) == T_ARRAY && 2 == RARRAY_LEN(val)) {  // Hash entry
		VALUE	*np = RARRAY_PTR(val);

		key = *np;
		val = *(np + 1);
	    }
	    if (Qundef == obj) {
		obj = rb_hash_new();
		obj_type = T_HASH;
	    }
	    if (T_OBJECT == obj_type) {
		VALUE       *slot;
		ID          var_id;

		if (Qundef == (var_id = oj_cache_get(oj_attr_cache, ks, &slot))) {
		    char	attr[1024];

		    if ('~' == *ks) {
			strncpy(attr, ks + 1, sizeof(attr) - 1);
		    } else {
			*attr = '@';
			strncpy(attr + 1, ks, sizeof(attr) - 2);
		    }
		    attr[sizeof(attr) - 1] = '\0';
		    var_id = rb_intern(attr);
		    *slot = var_id;
		}
		rb_ivar_set(obj, var_id, val);
	    } else if (T_HASH == obj_type) {
		rb_hash_aset(obj, key, val);
	    } else {
		raise_error("invalid Object format, too many Hash entries.", pi->str, pi->s);
	    }
	}
	next_non_white(pi);
	if ('}' == *pi->s) {
	    pi->s++;
	    break;
	} else if (',' == *pi->s) {
	    pi->s++;
	} else {
	    //printf("*** '%s'\n", pi->s);
	    raise_error("invalid format, expected , or } while in an object", pi->str, pi->s);
	}
    }
    return obj;
}

static VALUE
read_array(ParseInfo pi) {
    VALUE	a = rb_ary_new();
    VALUE	e;

    pi->s++;
    next_non_white(pi);
    if (']' == *pi->s) {
	pi->s++;
	return a;
    }
    while (1) {
	if (Qundef == (e = read_next(pi, 0))) {
	    raise_error("unexpected character", pi->str, pi->s);
	}
        rb_ary_push(a, e);
	next_non_white(pi);	// skip white space
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

static VALUE
read_str(ParseInfo pi, int hint) {
    char	*text = read_quoted_value(pi);
    VALUE	obj;

    if (ObjectMode != pi->options->mode) {
	hint = T_STRING;
    }
    switch (hint) {
    case T_CLASS:
	obj = classname2class(text, pi);
	break;
    case T_OBJECT:
	obj = classname2obj(text, pi);
	break;
    case T_STRING:
	obj = rb_str_new2(text);
#ifdef HAVE_RUBY_ENCODING_H
	if (0 != pi->encoding) {
	    rb_enc_associate(obj, pi->encoding);
	}
#endif
	break;
    case T_SYMBOL:
#ifdef HAVE_RUBY_ENCODING_H
	if (0 != pi->encoding) {
	    obj = rb_str_new2(text);
	    rb_enc_associate(obj, pi->encoding);
	    obj = rb_funcall(obj, oj_to_sym_id, 0);
	} else {
	    obj = ID2SYM(rb_intern(text));
	}
#else
	obj = ID2SYM(rb_intern(text));
#endif
	break;
    case 0:
    default:
	if (':' == *text) {
	    if (':' == text[1]) { // escaped :, it s string
		obj = rb_str_new2(text + 1);
#ifdef HAVE_RUBY_ENCODING_H
		if (0 != pi->encoding) {
		    rb_enc_associate(obj, pi->encoding);
		}
#endif
	    } else { // Symbol
#ifdef HAVE_RUBY_ENCODING_H
		if (0 != pi->encoding) {
		    obj = rb_str_new2(text + 1);
		    rb_enc_associate(obj, pi->encoding);
		    obj = rb_funcall(obj, oj_to_sym_id, 0);
		} else {
		    obj = ID2SYM(rb_intern(text + 1));
		}
#else
		obj = ID2SYM(rb_intern(text + 1));
#endif
	    }
	} else {
	    obj = rb_str_new2(text);
#ifdef HAVE_RUBY_ENCODING_H
	    if (0 != pi->encoding) {
		rb_enc_associate(obj, pi->encoding);
	    }
#endif
	}
	break;
    }
    return obj;
}

#ifdef RUBINIUS
#define NUM_MAX 0x07FFFFFF
#else
#define NUM_MAX (FIXNUM_MAX >> 8)
#endif

static VALUE
read_num(ParseInfo pi) {
    char	*start = pi->s;
    int64_t	n = 0;
    long	a = 0;
    long	div = 1;
    long	e = 0;
    int		neg = 0;
    int		eneg = 0;
    int		big = 0;

    if ('-' == *pi->s) {
	pi->s++;
	neg = 1;
    } else if ('+' == *pi->s) {
	pi->s++;
    }
    for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	n = n * 10 + (*pi->s - '0');
	if (NUM_MAX <= n) {
	    big = 1;
	}
    }
    if (big) {
	char	c = *pi->s;
	VALUE	num;
	
	*pi->s = '\0';
	num = rb_cstr_to_inum(start, 10, 0);
	*pi->s = c;

	return num;
    }
    if ('.' == *pi->s) {
	pi->s++;
	for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	    a = a * 10 + (*pi->s - '0');
	    div *= 10;
	}
    }
    if ('e' == *pi->s || 'E' == *pi->s) {
	pi->s++;
	if ('-' == *pi->s) {
	    pi->s++;
	    eneg = 1;
	} else if ('+' == *pi->s) {
	    pi->s++;
	}
	for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	    e = e * 10 + (*pi->s - '0');
	}
    }
    if (0 == e && 0 == a && 1 == div) {
	if (neg) {
	    n = -n;
	}
	return LONG2NUM(n);
    } else {
	double	d = (double)n + (double)a / (double)div;

	if (neg) {
	    d = -d;
	}
	if (0 != e) {
	    if (eneg) {
		e = -e;
	    }
	    d *= pow(10.0, e);
	}
	return DBL2NUM(d);
    }
}

static VALUE
read_time(ParseInfo pi) {
    VALUE       args[2];
    long        v = 0;
    long        v2 = 0;

    for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	v = v * 10 + (*pi->s - '0');
    }
    if ('.' == *pi->s) {
	pi->s++;
	for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	    v2 = v2 * 10 + (*pi->s - '0');
	}
    }
    args[0] = LONG2NUM(v);
    args[1] = LONG2NUM(v2);

    return rb_funcall2(oj_time_class, oj_at_id, 2, args);
}

static VALUE
read_true(ParseInfo pi) {
    pi->s++;
    if ('r' != *pi->s || 'u' != *(pi->s + 1) || 'e' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'true'", pi->str, pi->s);
    }
    pi->s += 3;

    return Qtrue;
}

static VALUE
read_false(ParseInfo pi) {
    pi->s++;
    if ('a' != *pi->s || 'l' != *(pi->s + 1) || 's' != *(pi->s + 2) || 'e' != *(pi->s + 3)) {
	raise_error("invalid format, expected 'false'", pi->str, pi->s);
    }
    pi->s += 4;

    return Qfalse;
}

static VALUE
read_nil(ParseInfo pi) {
    pi->s++;
    if ('u' != *pi->s || 'l' != *(pi->s + 1) || 'l' != *(pi->s + 2)) {
	raise_error("invalid format, expected 'nil'", pi->str, pi->s);
    }
    pi->s += 3;

    return Qnil;
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
    // TBD can whole string be read in and then eval-ed by ruby of there is a special character
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
		// TBD if first character is 00 then skip it
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
