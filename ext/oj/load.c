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

static VALUE	read_next(ParseInfo pi);
static VALUE	read_obj(ParseInfo pi);
static VALUE	read_array(ParseInfo pi);
static VALUE	read_str(ParseInfo pi);
static VALUE	read_num(ParseInfo pi);
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
    pi.encoding = ('\0' == *options->encoding) ? 0 : rb_enc_find(options->encoding);
    pi.options = options;
    if (Qundef == (obj = read_next(&pi))) {
	raise_error("no object read", pi.str, pi.s);
    }
    next_non_white(&pi);	// skip white space
    if ('\0' != *pi.s) {
	raise_error("invalid format, extra characters", pi.str, pi.s);
    }
    return obj;
}

static VALUE
read_next(ParseInfo pi) {
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
	obj = read_str(pi);
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
	obj = read_num(pi);
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
    
    pi->s++;
    next_non_white(pi);
    if ('}' == *pi->s) {
	pi->s++;
	return rb_hash_new();
    }
    while (1) {
	next_non_white(pi);
	if ('"' != *pi->s || Qundef == (key = read_str(pi))) {
	    raise_error("unexpected character", pi->str, pi->s);
	}
	next_non_white(pi);
	if (':' == *pi->s) {
	    pi->s++;
	} else {
	    raise_error("invalid format, expected :", pi->str, pi->s);
	}
	if (Qundef == (val = read_next(pi))) {
	    raise_error("unexpected character", pi->str, pi->s);
	}
	if (Qundef == obj) {
	    obj = rb_hash_new();
	}
	rb_hash_aset(obj, key, val);
	next_non_white(pi);
	if ('}' == *pi->s) {
	    pi->s++;
	    break;
	} else if (',' == *pi->s) {
	    pi->s++;
	} else {
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
	if (Qundef == (e = read_next(pi))) {
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
read_str(ParseInfo pi) {
    char	*text = read_quoted_value(pi);
    VALUE	s = rb_str_new2(text);

#ifdef HAVE_RUBY_ENCODING_H
    if (0 != pi->encoding) {
        rb_enc_associate(s, pi->encoding);
    }
#endif
    return s;
}

static VALUE
read_num(ParseInfo pi) {
    int64_t	n = 0;
    long	a = 0;
    long	div = 1;
    long	e = 0;
    int		neg = 0;
    int		eneg = 0;

    if ('-' == *pi->s) {
	pi->s++;
	neg = 1;
    } else if ('+' == *pi->s) {
	pi->s++;
    }
    for (; '0' <= *pi->s && *pi->s <= '9'; pi->s++) {
	n = n * 10 + (*pi->s - '0');
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
