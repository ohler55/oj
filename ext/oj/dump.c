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

#include "ruby.h"
#ifdef HAVE_RUBY_ENCODING_H
#include "ruby/st.h"
#else
#include "st.h"
#endif
#include "oj.h"

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
    VALUE		obj;
} *Out;

static void     dump_obj_to_json(VALUE obj, Options copts, Out out);
static void	dump_val(VALUE obj, int depth, Out out);
static void	dump_nil(Out out);
static void	dump_true(Out out);
static void	dump_false(Out out);
static void	dump_fixnum(VALUE obj, Out out);
static void	dump_float(VALUE obj, Out out);
static void	dump_cstr(const char *str, int cnt, Out out);
static void	dump_hex(u_char c, Out out);
static void	dump_str(VALUE obj, Out out);
static void	dump_sym(VALUE obj, Out out);
static void	dump_array(VALUE obj, int depth, Out out);
static void	dump_hash(VALUE obj, int depth, Out out);

static void     grow(Out out, size_t len);
static int      is_json_friendly(const u_char *str, int len);
static int	json_friendly_size(const u_char *str, int len);


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
is_json_friendly(const u_char *str, int len) {
    for (; 0 < len; str++, len--) {
        if ('o' != json_friendly_chars[*str]) {
            return 0;
        }
    }
    return 1;
}

inline static int
json_friendly_size(const u_char *str, int len) {
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
    cnt *= out->indent;
    if (0 <= cnt) {
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
    u_char	d = c & 0xF0;

    if (9 < d) {
	*out->cur++ = (d - 10) + 'a';
    } else {
	*out->cur++ = d + '0';
    }
    d = c & 0x0F;
    if (9 < d) {
	*out->cur++ = (d - 10) + 'a';
    } else {
	*out->cur++ = d + '0';
    }
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
dump_cstr(const char *str, int cnt, Out out) {
    int	size = json_friendly_size((u_char*)str, cnt);
    
    if (cnt == size) {
	cnt += 2;
	if (out->end - out->cur <= (long)cnt) {
	    grow(out, cnt);
	}
	*out->cur++ = '"';
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
    dump_cstr(StringValuePtr(obj), (int)RSTRING_LEN(obj), out);
}

static void
dump_sym(VALUE obj, Out out) {
    const char	*sym = rb_id2name(SYM2ID(obj));
    
    dump_cstr(sym, (int)strlen(sym), out);
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
hash_cb(VALUE key, VALUE value, Out out) {
    int		depth = out->depth;
    size_t	size = depth * out->indent + 1;

    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
    fill_indent(out, depth);
    dump_str(key, out);
    *out->cur++ = ':';
    dump_val(value, depth, out);
    out->depth = depth;
    *out->cur++ = ',';
    
    return ST_CONTINUE;
}

static void
dump_hash(VALUE obj, int depth, Out out) {
    int	cnt = (int)RHASH_SIZE(obj);

    *out->cur++ = '{';
    if (0 == cnt) {
	*out->cur++ = '}';
    } else {
	size_t	size = depth * out->indent + 2;

	out->depth = depth + 1;
	rb_hash_foreach(obj, hash_cb, (VALUE)out);
	out->cur--; // backup to overwrite last comma
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
	*out->cur++ = '}';
    }
    *out->cur = '\0';
}

static void
dump_val(VALUE obj, int depth, Out out) {
    switch (rb_type(obj)) {
    case T_NIL:		dump_nil(out);			break;
    case T_TRUE:	dump_true(out);			break;
    case T_FALSE:	dump_false(out);		break;
    case T_FIXNUM:	dump_fixnum(obj, out);		break;
    case T_FLOAT:	dump_float(obj, out);		break;
    case T_BIGNUM:	break; // TBD
    case T_STRING:	dump_str(obj, out);		break;
    case T_SYMBOL:	dump_sym(obj, out);		break;
    case T_ARRAY:	dump_array(obj, depth, out);	break;
    case T_HASH:	dump_hash(obj, depth, out);	break;
    case T_OBJECT:
    case T_REGEXP:
    case T_CLASS:
    case T_DATA: // for Time
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
    out->obj = obj;
/*    if (Yes == copts->circular) {
        ox_cache8_new(&out->circ_cache);
	}*/
    out->indent = copts->indent;
    out->indent = 2; // TBD
    dump_val(obj, 0, out);
    
/*    if (Yes == copts->circular) {
        ox_cache8_delete(out->circ_cache);
	}*/
}

char*
write_obj_to_str(VALUE obj, Options copts) {
    struct _Out out;

    dump_obj_to_json(obj, copts, &out);

    return out.buf;
}

void
write_obj_to_file(VALUE obj, const char *path, Options copts) {
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
