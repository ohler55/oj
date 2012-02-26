/* dump.c
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
#include <errno.h>
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "oj.h"
#if IVAR_HELPERS
#include "ruby/st.h"
#else
#include "st.h"
#endif

typedef unsigned long   ulong;

typedef struct _Str {
    const char  *str;
    size_t      len;
} *Str;

typedef struct _Element {
    struct _Str         clas;
    struct _Str         attr;
    unsigned long       id;
    int                 indent; // < 0 indicates no \n
    int                 closed;
    char                type;
} *Element;

typedef struct _Out {
    void                (*w_start)(struct _Out *out, Element e);
    void                (*w_end)(struct _Out *out, Element e);
    void                (*w_time)(struct _Out *out, VALUE obj);
    char                *buf;
    char                *end;
    char                *cur;
//    Cache8              circ_cache;
//    unsigned long       circ_cnt;
    int                 indent;
    int                 depth; // used by dumpHash
    Options             opts;
    uint32_t		hash_cnt;
} *Out;

static void     dump_obj_to_json(VALUE obj, Options copts, Out out);
static void	raise_strict(VALUE obj);
static void	dump_val(VALUE obj, int depth, Out out);
static void	dump_nil(Out out);
static void	dump_true(Out out);
static void	dump_false(Out out);
static void	dump_fixnum(VALUE obj, Out out);
static void	dump_bignum(VALUE obj, Out out);
static void	dump_float(VALUE obj, Out out);
static void	dump_cstr(const char *str, size_t cnt, int is_sym, Out out);
static void	dump_hex(u_char c, Out out);
static void	dump_str(VALUE obj, Out out);
static void	dump_sym_comp(VALUE obj, Out out);
static void	dump_sym_obj(VALUE obj, Out out);
static void	dump_class_comp(VALUE obj, Out out);
static void	dump_class_obj(VALUE obj, Out out);
static void	dump_array(VALUE obj, int depth, Out out);
static int	hash_cb_strict(VALUE key, VALUE value, Out out);
static int	hash_cb_object(VALUE key, VALUE value, Out out);
static void	dump_hash(VALUE obj, int depth, int mode, Out out);
static void	dump_time(VALUE obj, Out out);
static void	dump_data_comp(VALUE obj, Out out);
static void	dump_data_obj(VALUE obj, Out out);
static void	dump_obj_comp(VALUE obj, int depth, Out out);
static void	dump_obj_obj(VALUE obj, int depth, Out out);
static int	dump_attr_cb(ID key, VALUE value, Out out);
static void	dump_obj_attrs(VALUE obj, int with_class, int depth, Out out);

static void     grow(Out out, size_t len);
static int      is_json_friendly(const u_char *str, size_t len);
static size_t	json_friendly_size(const u_char *str, size_t len);


static const char	hex_chars[17] = "0123456789abcdef";

static char     json_friendly_chars[256] = "\
uuuuuuuuxxxuxxuuuuuuuuuuuuuuuuuu\
ooxooooooooooooxoooooooooooooooo\
ooooooooooooooooooooooooooooxooo\
ooooooooooooooooooooooooooooooou\
uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu\
uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu\
uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu\
uuuuuuuuuuuuuuuuuuuuuuuuuuuuuuuu";

inline static int
is_json_friendly(const u_char *str, size_t len) {
    for (; 0 < len; str++, len--) {
        if ('o' != json_friendly_chars[*str]) {
            return 0;
        }
    }
    return 1;
}

inline static size_t
json_friendly_size(const u_char *str, size_t len) {
    int		cnt = 0;
    
    for (; 0 < len; str++, len--) {
	switch (json_friendly_chars[*str]) {
	case 'o':	cnt++;		break;
	case 'x':	cnt += 2;	break;
	case 'u':	cnt += 6;	break;
	default:			break;
	}
    }
    return cnt;
}

inline static void
fill_indent(Out out, int cnt) {
    if (0 < cnt && 0 < out->indent) {
	cnt *= out->indent;
        *out->cur++ = '\n';
        for (; 0 < cnt; cnt--) {
            *out->cur++ = ' ';
        }
    }
}

static void
grow(Out out, size_t len) {
    size_t  size = out->end - out->buf;
    long    pos = out->cur - out->buf;
    char    *buf;
        
    size *= 2;
    if (size <= len * 2 + pos) {
        size += len;
    }
    if (0 == (buf = (char*)realloc(out->buf, size + 10))) { // 1 extra for terminator character plus extra (paranoid)
        rb_raise(rb_eNoMemError, "Failed to create string. [%d:%s]\n", ENOSPC, strerror(ENOSPC));
    }
    out->buf = buf;
    out->end = buf + size;
    out->cur = out->buf + pos;
}

inline static void
dump_hex(u_char c, Out out) {
    u_char	d = (c >> 4) & 0x0F;

    *out->cur++ = hex_chars[d];
    d = c & 0x0F;
    *out->cur++ = hex_chars[d];
}

static void
dump_nil(Out out) {
    size_t      size = 4;

    if (out->end - out->cur <= (long)size) {
        grow(out, size);
    }
    *out->cur++ = 'n';
    *out->cur++ = 'u';
    *out->cur++ = 'l';
    *out->cur++ = 'l';
    *out->cur = '\0';
}

static void
dump_true(Out out) {
    size_t      size = 4;

    if (out->end - out->cur <= (long)size) {
        grow(out, size);
    }
    *out->cur++ = 't';
    *out->cur++ = 'r';
    *out->cur++ = 'u';
    *out->cur++ = 'e';
    *out->cur = '\0';
}

static void
dump_false(Out out) {
    size_t      size = 5;

    if (out->end - out->cur <= (long)size) {
        grow(out, size);
    }
    *out->cur++ = 'f';
    *out->cur++ = 'a';
    *out->cur++ = 'l';
    *out->cur++ = 's';
    *out->cur++ = 'e';
    *out->cur = '\0';
}

static void
dump_fixnum(VALUE obj, Out out) {
    char        buf[32];
    char        *b = buf + sizeof(buf) - 1;
    long        num = NUM2LONG(obj);
    int         neg = 0;

    if (0 > num) {
        neg = 1;
        num = -num;
    }
    *b-- = '\0';
    if (0 < num) {
        for (; 0 < num; num /= 10, b--) {
            *b = (num % 10) + '0';
        }
        if (neg) {
            *b = '-';
        } else {
            b++;
        }
    } else {
        *b = '0';
    }
    if (out->end - out->cur <= (long)(sizeof(buf) - (b - buf))) {
        grow(out, sizeof(buf) - (b - buf));
    }
    for (; '\0' != *b; b++) {
        *out->cur++ = *b;
    }
    *out->cur = '\0';
}

static void
dump_bignum(VALUE obj, Out out) {
    VALUE	rs = rb_big2str(obj, 10);
    int		cnt = (int)RSTRING_LEN(rs);

    if (out->end - out->cur <= (long)cnt) {
        grow(out, cnt);
    }
    memcpy(out->cur, StringValuePtr(rs), cnt);
    out->cur += cnt;
    *out->cur = '\0';
}

static void
dump_float(VALUE obj, Out out) {
    char	buf[64];
    char	*b;
    int		cnt = sprintf(buf, "%0.16g", RFLOAT_VALUE(obj)); // used sprintf due to bug in snprintf

    if (out->end - out->cur <= (long)cnt) {
        grow(out, cnt);
    }
    for (b = buf; '\0' != *b; b++) {
        *out->cur++ = *b;
    }
    *out->cur = '\0';
}

static void
dump_cstr(const char *str, size_t cnt, int is_sym, Out out) {
    size_t	size = json_friendly_size((u_char*)str, cnt);
    
    if (cnt == size) {
	cnt += 2 + is_sym;
	if (out->end - out->cur <= (long)cnt) {
	    grow(out, cnt);
	}
	*out->cur++ = '"';
	if (is_sym) {
	    *out->cur++ = ':';
	}
	for (; '\0' != *str; str++) {
	    *out->cur++ = *str;
	}
	*out->cur++ = '"';
    } else {
	size += 2;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	*out->cur++ = '"';
	if (is_sym) {
	    *out->cur++ = ':';
	}
	for (; 0 < cnt; cnt--, str++) {
	    switch (json_friendly_chars[(u_char)*str]) {
	    case 'o':
		*out->cur++ = *str;
		break;
	    case 'x':
		*out->cur++ = '\\';
		switch (*str) {
		case '\b':	*out->cur++ = 'b';	break;
		case '\t':	*out->cur++ = 't';	break;
		case '\n':	*out->cur++ = 'n';	break;
		case '\f':	*out->cur++ = 'f';	break;
		case '\r':	*out->cur++ = 'r';	break;
		default:	*out->cur++ = *str;	break;
		}
		break;
	    case 'u':
		*out->cur++ = '\\';
		*out->cur++ = 'u';
		if ((u_char)*str <= 0x7F) {
		    *out->cur++ = '0';
		    *out->cur++ = '0';
		    dump_hex((u_char)*str, out);
		} else { // continuation?
		    // TBD lead with \u00 . grab next char?
		    *out->cur++ = '0';
		    *out->cur++ = '0';
		    dump_hex((u_char)*str, out);
		}
		break;
	    default:
		break; // ignore, should never happen if the table is correct
	    }
	}
	*out->cur++ = '"';
    }
    *out->cur = '\0';
}

static void
dump_str(VALUE obj, Out out) {
    dump_cstr(StringValuePtr(obj), RSTRING_LEN(obj), 0, out);
}

static void
dump_sym_comp(VALUE obj, Out out) {
    const char	*sym = rb_id2name(SYM2ID(obj));
    
    dump_cstr(sym, strlen(sym), 0, out);
}

static void
dump_sym_obj(VALUE obj, Out out) {
    const char	*sym = rb_id2name(SYM2ID(obj));
    size_t	len = strlen(sym);
    
    if (':' == *sym) {
	if (out->end - out->cur <= 6) {
	    grow(out, 6);
	}
	*out->cur++ = '{';
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = 's';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_cstr(sym, len, 0, out);
	*out->cur++ = '}';
	*out->cur = '\0';
    } else {
	dump_cstr(sym, len, 1, out);
    }
}

static void
dump_class_comp(VALUE obj, Out out) {
    const char	*s = rb_class2name(obj);

    dump_cstr(s, strlen(s), 0, out);
}

static void
dump_class_obj(VALUE obj, Out out) {
    const char	*s = rb_class2name(obj);
    size_t	len = strlen(s);

    if (out->end - out->cur <= 6) {
	grow(out, 6);
    }
    *out->cur++ = '{';
    *out->cur++ = '"';
    *out->cur++ = '^';
    *out->cur++ = 'c';
    *out->cur++ = '"';
    *out->cur++ = ':';
    dump_cstr(s, len, 0, out);
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
dump_array(VALUE a, int depth, Out out) {
    VALUE       *np = RARRAY_PTR(a);
    size_t      size = 2;
    int		cnt = (int)RARRAY_LEN(a);
    int		d2 = depth + 1;
    
    if (out->end - out->cur <= (long)size) {
        grow(out, size);
    }
    *out->cur++ = '[';
    if (0 == cnt) {
	*out->cur++ = ']';
    } else {
	size = d2 * out->indent + 2;
	for (; 0 < cnt; cnt--, np++) {
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    dump_val(*np, d2, out);
	    if (1 < cnt) {
		// TBD check size?
		*out->cur++ = ',';
	    }
	}
	size = depth * out->indent + 1;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
	*out->cur++ = ']';
    }
    *out->cur = '\0';
}

static int
hash_cb_strict(VALUE key, VALUE value, Out out) {
    int		depth = out->depth;
    long	size = depth * out->indent + 1;

    if (out->end - out->cur <= size) {
	grow(out, size);
    }
    fill_indent(out, depth);
    if (rb_type(key) == T_STRING) {
	dump_str(key, out);
	*out->cur++ = ':';
	dump_val(value, depth, out);
    } else {
	rb_raise(rb_eTypeError, "In :strict mode all Hash keys must be Strings.");
    }
    out->depth = depth;
    *out->cur++ = ',';

    return ST_CONTINUE;
}

static int
hash_cb_object(VALUE key, VALUE value, Out out) {
    int		depth = out->depth;
    long	size = depth * out->indent + 1;

    if (out->end - out->cur <= size) {
	grow(out, size);
    }
    fill_indent(out, depth);
    // TBD if key is a string else dump with unique key for and entry array
    if (rb_type(key) == T_STRING) {
	dump_str(key, out);
	*out->cur++ = ':';
	dump_val(value, depth, out);
    } else if (rb_type(key) == T_SYMBOL) {
	dump_sym_obj(key, out);
	*out->cur++ = ':';
	dump_val(value, depth, out);
    } else {
	int	d2 = depth + 1;
	long	s2 = size + out->indent + 1;
	int	i;
	int	started = 0;
	u_char	b;

	if (out->end - out->cur <= s2 + 15) {
	    grow(out, s2 + 15);
	}
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = '#';
	out->hash_cnt++;
	for (i = 28; 0 <= i; i -= 4) {
	    b = (u_char)((out->hash_cnt >> i) & 0x0000000F);
	    if ('\0' != b) {
		started = 1;
	    }
	    if (started) {
		*out->cur++ = hex_chars[b];
	    }
	}
	*out->cur++ = '"';
	*out->cur++ = ':';
	*out->cur++ = '[';
	fill_indent(out, d2);
	dump_val(key, d2, out);
	if (out->end - out->cur <= (long)s2) {
	    grow(out, s2);
	}
	*out->cur++ = ',';
	fill_indent(out, d2);
	dump_val(value, d2, out);
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
	*out->cur++ = ']';
    }
    out->depth = depth;
    *out->cur++ = ',';
    
    return ST_CONTINUE;
}

static void
dump_hash(VALUE obj, int depth, int mode, Out out) {
    int	cnt = (int)RHASH_SIZE(obj);

    if (out->end - out->cur <= 2) {
	grow(out, 2);
    }
    *out->cur++ = '{';
    if (0 == cnt) {
	*out->cur++ = '}';
    } else {
	size_t	size = depth * out->indent + 2;

	out->depth = depth + 1;
	if (ObjectMode == mode) {
	    rb_hash_foreach(obj, hash_cb_object, (VALUE)out);
	} else {
	    rb_hash_foreach(obj, hash_cb_strict, (VALUE)out);
	}
	if (',' == *(out->cur - 1)) {
	    out->cur--; // backup to overwrite last comma
	}
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
	*out->cur++ = '}';
    }
    *out->cur = '\0';
}

static void
dump_time(VALUE obj, Out out) {
    char        buf[64];
    char        *b = buf + sizeof(buf) - 1;
    time_t      sec = NUM2LONG(rb_funcall2(obj, oj_tv_sec_id, 0, 0));
    long        usec = NUM2LONG(rb_funcall2(obj, oj_tv_usec_id, 0, 0));
    char        *dot = b - 7;
    long        size;

    *b-- = '\0';
    for (; dot < b; b--, usec /= 10) {
	*b = '0' + (usec % 10);
    }
    *b-- = '.';
    for (; 0 < sec; b--, sec /= 10) {
	*b = '0' + (sec % 10);
    }
    b++;
    size = sizeof(buf) - (b - buf) - 1;
    if (out->end - out->cur <= size) {
	grow(out, size);
    }
    memcpy(out->cur, b, size);
    out->cur += size;
    *out->cur = '\0';
}

static void
dump_data_comp(VALUE obj, Out out) {
    VALUE   clas = rb_obj_class(obj);

    if (rb_cTime == clas) {
	dump_time(obj, out);
    } else {
	dump_nil(out);
    }
}

static void
dump_data_obj(VALUE obj, Out out) {
    VALUE   clas = rb_obj_class(obj);

    if (rb_cTime == clas) {
	if (out->end - out->cur <= 6) {
	    grow(out, 6);
	}
	*out->cur++ = '{';
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = 't';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_time(obj, out);
	*out->cur++ = '}';
	*out->cur = '\0';
    } else {
	dump_nil(out);
    }
}

static void
dump_obj_comp(VALUE obj, int depth, Out out) {
    if (rb_respond_to(obj, oj_to_hash_id)) {
	VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);
 
	if (T_HASH != rb_type(h)) {
	    rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	}
	dump_hash(h, depth, out->opts->mode, out);
    } else if (rb_respond_to(obj, oj_to_json_id)) {
	VALUE		rs = rb_funcall(obj, oj_to_json_id, 0);
	const char	*s = StringValuePtr(rs);
	int		len = (int)RSTRING_LEN(rs);

	if (out->end - out->cur <= len) {
	    grow(out, len);
	}
	memcpy(out->cur, s, len);
	out->cur += len;
    } else {
	dump_obj_attrs(obj, 0, depth, out);
    }
    *out->cur = '\0';
}

inline static void
dump_obj_obj(VALUE obj, int depth, Out out) {
    dump_obj_attrs(obj, 1, depth, out);
}

static int
dump_attr_cb(ID key, VALUE value, Out out) {
    int		depth = out->depth;
    size_t	size = depth * out->indent + 1;
    const char	*attr = rb_id2name(key);

    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
    fill_indent(out, depth);
    if ('@' == *attr) {
	attr++;
	dump_cstr(attr, strlen(attr), 0, out);
    } else {
	char	buf[32];

	*buf = '~';
	strncpy(buf + 1, attr, sizeof(buf) - 2);
	buf[sizeof(buf) - 1] = '\0';
	dump_cstr(buf, strlen(buf), 0, out);
    }
    *out->cur++ = ':';
    dump_val(value, depth, out);
    out->depth = depth;
    *out->cur++ = ',';
    
    return ST_CONTINUE;
}

static void
dump_obj_attrs(VALUE obj, int with_class, int depth, Out out) {
    size_t	size;
    int		d2 = depth + 1;

    if (out->end - out->cur <= 2) {
	grow(out, 2);
    }
    *out->cur++ = '{';
    if (with_class) {
	const char	*class_name = rb_class2name(rb_obj_class(obj));
	int		clen = (int)strlen(class_name);

	size = d2 * out->indent + clen + 10;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, d2);
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = 'o';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_cstr(class_name, clen, 0, out);
    }
    {
	int	cnt;
// use encoding as the indicator for Ruby 1.8.7 or 1.9.x
#if IVAR_HELPERS
	cnt = (int)rb_ivar_count(obj);
#else
	VALUE		vars = rb_funcall2(obj, oj_instance_variables_id, 0, 0);
	VALUE		*np = RARRAY_PTR(vars);
	ID		vid;
	const char	*attr;
	int		i;

	cnt = (int)RARRAY_LEN(vars);
#endif
	if (with_class && 0 < cnt) {
	    *out->cur++ = ',';
	}
	out->depth = depth + 1;
#if IVAR_HELPERS
	rb_ivar_foreach(obj, dump_attr_cb, (VALUE)out);
	out->cur--; // backup to overwrite last comma
#else
	size = d2 * out->indent + 1;
	for (i = cnt; 0 < i; i--, np++) {
	    if (out->end - out->cur <= (long)size) {
	        grow(out, size);
	    }
	    vid = rb_to_id(*np);
	    fill_indent(out, d2);
	    attr = rb_id2name(vid);
	    if ('@' == *attr) {
		attr++;
		dump_cstr(attr, strlen(attr), 0, out);
	    } else {
		char	buf[32];

		*buf = '~';
		strncpy(buf + 1, attr, sizeof(buf) - 2);
		buf[sizeof(buf) - 1] = '\0';
		dump_cstr(buf, strlen(attr) + 1, 0, out);
	    }
	    *out->cur++ = ':';
	    dump_val(rb_ivar_get(obj, vid), d2, out);
	    if (out->end - out->cur <= 2) {
	        grow(out, 2);
	    }
	    if (1 < i) {
	      *out->cur++ = ',';
	    }
	}
#endif
	out->depth = depth;
    }
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
raise_strict(VALUE obj) {
    rb_raise(rb_eTypeError, "Failed to dump %s Object to JSON in strict mode.\n", rb_class2name(rb_obj_class(obj)));
}

static void
dump_val(VALUE obj, int depth, Out out) {
    switch (rb_type(obj)) {
    case T_NIL:		dump_nil(out);			break;
    case T_TRUE:	dump_true(out);			break;
    case T_FALSE:	dump_false(out);		break;
    case T_FIXNUM:	dump_fixnum(obj, out);		break;
    case T_FLOAT:	dump_float(obj, out);		break;
    case T_BIGNUM:	dump_bignum(obj, out);		break;
    case T_STRING:	dump_str(obj, out);		break;
    case T_SYMBOL:
	switch (out->opts->mode) {
	case StrictMode:	raise_strict(obj);		break;
	case NullMode:		dump_nil(out);			break;
	case CompatMode:	dump_sym_comp(obj, out);	break;
	case ObjectMode:
	default:		dump_sym_obj(obj, out);		break;
	}
	break;
    case T_ARRAY:	dump_array(obj, depth, out);	break;
    case T_HASH:	dump_hash(obj, depth, out->opts->mode, out);	break;
    case T_CLASS:
	switch (out->opts->mode) {
	case StrictMode:	raise_strict(obj);		break;
	case NullMode:		dump_nil(out);			break;
	case CompatMode:	dump_class_comp(obj, out);	break;
	case ObjectMode:
	default:		dump_class_obj(obj, out);	break;
	}
	break;
    case T_OBJECT:
	switch (out->opts->mode) {
	case StrictMode:	raise_strict(obj);		break;
	case NullMode:		dump_nil(out);			break;
	case CompatMode:	dump_obj_comp(obj, depth, out);	break;
	case ObjectMode:
	default:		dump_obj_obj(obj, depth, out);	break;
	}
	break;
    case T_DATA:
	switch (out->opts->mode) {
	case StrictMode:	raise_strict(obj);		break;
	case NullMode:		dump_nil(out);			break;
	case CompatMode:	dump_data_comp(obj, out);	break;
	case ObjectMode:
	default:		dump_data_obj(obj, out);	break;
	}
	break;
    case T_STRUCT: // for Range
#if (defined T_COMPLEX && defined RCOMPLEX)
    case T_COMPLEX:
#endif
#if (defined T_RATIONAL && defined RRATIONAL)
    case T_RATIONAL:
#endif
    case T_REGEXP:
	// TBD
	rb_raise(rb_eNotImpError, "Failed to dump '%s' Object (%02x)\n",
		 rb_class2name(rb_obj_class(obj)), rb_type(obj));
	break;
    default:
	rb_raise(rb_eNotImpError, "Failed to dump '%s' Object (%02x)\n",
		 rb_class2name(rb_obj_class(obj)), rb_type(obj));
	break;
    }
}

static void
dump_obj_to_json(VALUE obj, Options copts, Out out) {
    out->buf = (char*)malloc(65336);
    out->end = out->buf + 65325; // 1 less than end plus extra for possible errors
    out->cur = out->buf;
//    out->circ_cache = 0;
//    out->circ_cnt = 0;
    out->opts = copts;
    out->hash_cnt = 0;
/*    if (Yes == copts->circular) {
        ox_cache8_new(&out->circ_cache);
	}*/
    out->indent = copts->indent;
    dump_val(obj, 0, out);
    
/*    if (Yes == copts->circular) {
        ox_cache8_delete(out->circ_cache);
	}*/
}

char*
oj_write_obj_to_str(VALUE obj, Options copts) {
    struct _Out out;

    dump_obj_to_json(obj, copts, &out);

    return out.buf;
}

void
oj_write_obj_to_file(VALUE obj, const char *path, Options copts) {
    struct _Out out;
    size_t      size;
    FILE        *f;    

    dump_obj_to_json(obj, copts, &out);
    size = out.cur - out.buf;
    if (0 == (f = fopen(path, "w"))) {
        rb_raise(rb_eIOError, "%s\n", strerror(errno));
    }
    if (size != fwrite(out.buf, 1, size, f)) {
        int err = ferror(f);
        rb_raise(rb_eIOError, "Write failed. [%d:%s]\n", err, strerror(err));
    }
    free(out.buf);
    fclose(f);
}
