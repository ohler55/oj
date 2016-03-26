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
#if !IS_WINDOWS
#include <sys/time.h>
#endif
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>

#include "oj.h"
#include "cache8.h"
#include "odd.h"

#if !HAS_ENCODING_SUPPORT || defined(RUBINIUS_RUBY)
#define rb_eEncodingError	rb_eException
#endif

// Workaround in case INFINITY is not defined in math.h or if the OS is CentOS
#define OJ_INFINITY (1.0/0.0)

// Extra padding at end of buffer.
#define BUFFER_EXTRA 10

#define MAX_DEPTH 1000

typedef unsigned long	ulong;

static void	raise_strict(VALUE obj);
static void	dump_val(VALUE obj, int depth, Out out, int argc, VALUE *argv);
static void	dump_nil(Out out);
static void	dump_true(Out out);
static void	dump_false(Out out);
static void	dump_fixnum(VALUE obj, Out out);
static void	dump_bignum(VALUE obj, Out out);
static void	dump_float(VALUE obj, Out out);
static void	dump_raw(const char *str, size_t cnt, Out out);
static void	dump_cstr(const char *str, size_t cnt, int is_sym, int escape1, Out out);
static void	dump_hex(uint8_t c, Out out);
static void	dump_str_comp(VALUE obj, Out out);
static void	dump_str_obj(VALUE obj, VALUE clas, int depth, Out out);
static void	dump_sym_comp(VALUE obj, Out out);
static void	dump_sym_obj(VALUE obj, Out out);
static void	dump_class_comp(VALUE obj, Out out);
static void	dump_class_obj(VALUE obj, Out out);
static void	dump_array(VALUE obj, VALUE clas, int depth, Out out);
static int	hash_cb_strict(VALUE key, VALUE value, Out out);
static int	hash_cb_compat(VALUE key, VALUE value, Out out);
static int	hash_cb_object(VALUE key, VALUE value, Out out);
static void	dump_hash(VALUE obj, VALUE clas, int depth, int mode, Out out);
static void	dump_time(VALUE obj, Out out, int withZone);
static void	dump_ruby_time(VALUE obj, Out out);
static void	dump_xml_time(VALUE obj, Out out);
static void	dump_data_strict(VALUE obj, Out out);
static void	dump_data_null(VALUE obj, Out out);
static void	dump_data_comp(VALUE obj, int depth, Out out);
static void	dump_data_obj(VALUE obj, int depth, Out out);
static void	dump_obj_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv);
static void	dump_obj_obj(VALUE obj, int depth, Out out);
static void	dump_struct_comp(VALUE obj, int depth, Out out);
static void	dump_struct_obj(VALUE obj, int depth, Out out);
#if HAS_IVAR_HELPERS
static int	dump_attr_cb(ID key, VALUE value, Out out);
#endif
static void	dump_obj_attrs(VALUE obj, VALUE clas, slot_t id, int depth, Out out);
static void	dump_odd(VALUE obj, Odd odd, VALUE clas, int depth, Out out);

static void	grow(Out out, size_t len);
static size_t	hibit_friendly_size(const uint8_t *str, size_t len);
static size_t	xss_friendly_size(const uint8_t *str, size_t len);
static size_t	ascii_friendly_size(const uint8_t *str, size_t len);

static void	dump_leaf(Leaf leaf, int depth, Out out);
static void	dump_leaf_str(Leaf leaf, Out out);
static void	dump_leaf_fixnum(Leaf leaf, Out out);
static void	dump_leaf_float(Leaf leaf, Out out);
static void	dump_leaf_array(Leaf leaf, int depth, Out out);
static void	dump_leaf_hash(Leaf leaf, int depth, Out out);

static const char	hex_chars[17] = "0123456789abcdef";

// JSON standard except newlines are no escaped
static char	newline_friendly_chars[256] = "\
66666666221622666666666666666666\
11211111111111111111111111111111\
11111111111111111111111111112111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111";

// JSON standard
static char	hibit_friendly_chars[256] = "\
66666666222622666666666666666666\
11211111111111111111111111111111\
11111111111111111111111111112111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111";

// High bit set characters are always encoded as unicode. Worse case is 3
// bytes per character in the output. That makes this conservative.
static char	ascii_friendly_chars[256] = "\
66666666222622666666666666666666\
11211111111111111111111111111111\
11111111111111111111111111112111\
11111111111111111111111111111116\
33333333333333333333333333333333\
33333333333333333333333333333333\
33333333333333333333333333333333\
33333333333333333333333333333333";

// XSS safe mode
static char	xss_friendly_chars[256] = "\
66666666222622666666666666666666\
11211161111111121111111111116161\
11111111111111111111111111112111\
11111111111111111111111111111116\
33333333333333333333333333333333\
33333333333333333333333333333333\
33333333333333333333333333333333\
33333333333333333333333333333333";

inline static size_t
newline_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;

    for (; 0 < len; str++, len--) {
	size += newline_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static size_t
hibit_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;

    for (; 0 < len; str++, len--) {
	size += hibit_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static size_t
ascii_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;

    for (; 0 < len; str++, len--) {
	size += ascii_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static size_t
xss_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;

    for (; 0 < len; str++, len--) {
	size += xss_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static void
fill_indent(Out out, int cnt) {
    if (0 < out->indent) {
	cnt *= out->indent;
	*out->cur++ = '\n';
	for (; 0 < cnt; cnt--) {
	    *out->cur++ = ' ';
	}
    }
}

inline static void
dump_ulong(unsigned long num, Out out) {
    char	buf[32];
    char	*b = buf + sizeof(buf) - 1;

    *b-- = '\0';
    if (0 < num) {
	for (; 0 < num; num /= 10, b--) {
	    *b = (num % 10) + '0';
	}
	b++;
    } else {
	*b = '0';
    }
    for (; '\0' != *b; b++) {
	*out->cur++ = *b;
    }
    *out->cur = '\0';
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
    if (out->allocated) {
	buf = REALLOC_N(out->buf, char, (size + BUFFER_EXTRA));
    } else {
	buf = ALLOC_N(char, (size + BUFFER_EXTRA));
	out->allocated = 1;
	memcpy(buf, out->buf, out->end - out->buf + BUFFER_EXTRA);
    }
    if (0 == buf) {
	rb_raise(rb_eNoMemError, "Failed to create string. [%d:%s]\n", ENOSPC, strerror(ENOSPC));
    }
    out->buf = buf;
    out->end = buf + size;
    out->cur = out->buf + pos;
}

inline static void
dump_hex(uint8_t c, Out out) {
    uint8_t	d = (c >> 4) & 0x0F;

    *out->cur++ = hex_chars[d];
    d = c & 0x0F;
    *out->cur++ = hex_chars[d];
}

static void
dump_raw(const char *str, size_t cnt, Out out) {
    if (out->end - out->cur <= (long)cnt + 10) {
	grow(out, cnt + 10);
    }
    memcpy(out->cur, str, cnt);
    out->cur += cnt;
    *out->cur = '\0';
}

const char*
dump_unicode(const char *str, const char *end, Out out) {
    uint32_t	code = 0;
    uint8_t	b = *(uint8_t*)str;
    int		i, cnt;
    
    if (0xC0 == (0xE0 & b)) {
	cnt = 1;
	code = b & 0x0000001F;
    } else if (0xE0 == (0xF0 & b)) {
	cnt = 2;
	code = b & 0x0000000F;
    } else if (0xF0 == (0xF8 & b)) {
	cnt = 3;
	code = b & 0x00000007;
    } else if (0xF8 == (0xFC & b)) {
	cnt = 4;
	code = b & 0x00000003;
    } else if (0xFC == (0xFE & b)) {
	cnt = 5;
	code = b & 0x00000001;
    } else {
	cnt = 0;
	rb_raise(rb_eEncodingError, "Invalid Unicode\n");
    }
    str++;
    for (; 0 < cnt; cnt--, str++) {
	b = *(uint8_t*)str;
	if (end <= str || 0x80 != (0xC0 & b)) {
	    rb_raise(rb_eEncodingError, "Invalid Unicode\n");
	}
	code = (code << 6) | (b & 0x0000003F);
    }
    if (0x0000FFFF < code) {
	uint32_t	c1;

	code -= 0x00010000;
	c1 = ((code >> 10) & 0x000003FF) + 0x0000D800;
	code = (code & 0x000003FF) + 0x0000DC00;
	*out->cur++ = '\\';
	*out->cur++ = 'u';
	for (i = 3; 0 <= i; i--) {
	    *out->cur++ = hex_chars[(uint8_t)(c1 >> (i * 4)) & 0x0F];
	}
    }
    *out->cur++ = '\\';
    *out->cur++ = 'u';
    for (i = 3; 0 <= i; i--) {
	*out->cur++ = hex_chars[(uint8_t)(code >> (i * 4)) & 0x0F];
    }	
    return str - 1;
}

// returns 0 if not using circular references, -1 if not further writing is
// needed (duplicate), and a positive value if the object was added to the cache.
static long
check_circular(VALUE obj, Out out) {
    slot_t	id = 0;
    slot_t	*slot;

    if (ObjectMode == out->opts->mode && Yes == out->opts->circular) {
	if (0 == (id = oj_cache8_get(out->circ_cache, obj, &slot))) {
	    out->circ_cnt++;
	    id = out->circ_cnt;
	    *slot = id;
	} else {
	    if (out->end - out->cur <= 18) {
		grow(out, 18);
	    }
	    *out->cur++ = '"';
	    *out->cur++ = '^';
	    *out->cur++ = 'r';
	    dump_ulong(id, out);
	    *out->cur++ = '"';

	    return -1;
	}
    }
    return (long)id;
}

static void
dump_nil(Out out) {
    size_t	size = 4;

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
    size_t	size = 4;

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
    size_t	size = 5;

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
    char	buf[32];
    char	*b = buf + sizeof(buf) - 1;
    long long	num = rb_num2ll(obj);
    int		neg = 0;

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
    volatile VALUE	rs = rb_big2str(obj, 10);
    int			cnt = (int)RSTRING_LEN(rs);

    if (out->end - out->cur <= (long)cnt) {
	grow(out, cnt);
    }
    memcpy(out->cur, rb_string_value_ptr((VALUE*)&rs), cnt);
    out->cur += cnt;
    *out->cur = '\0';
}

static const char	inf_val[] = INF_VAL;
static const char	ninf_val[] = NINF_VAL;
static const char	nan_val[] = NAN_VAL;

// Removed dependencies on math due to problems with CentOS 5.4.
static void
dump_float(VALUE obj, Out out) {
    char	buf[64];
    char	*b;
    double	d = rb_num2dbl(obj);
    int		cnt = 0;

    if (0.0 == d) {
	b = buf;
	*b++ = '0';
	*b++ = '.';
	*b++ = '0';
	*b++ = '\0';
	cnt = 3;
    } else if (OJ_INFINITY == d) {
	if (ObjectMode == out->opts->mode) {
	    strcpy(buf, inf_val);
	    cnt = sizeof(inf_val) - 1;
	} else {
	    NanDump	nd = out->opts->dump_opts.nan_dump;
	    
	    if (AutoNan == nd) {
		switch (out->opts->mode) {
		case CompatMode:	nd = WordNan;	break;
		case StrictMode:	nd = RaiseNan;	break;
		case NullMode:		nd = NullNan;	break;
		default:				break;
		}
	    }
	    switch (nd) {
	    case RaiseNan:
		raise_strict(obj);
		break;
	    case WordNan:
		strcpy(buf, "Infinity");
		cnt = 8;
		break;
	    case NullNan:
		strcpy(buf, "null");
		cnt = 4;
		break;
	    case HugeNan:
	    default:
		strcpy(buf, inf_val);
		cnt = sizeof(inf_val) - 1;
		break;
	    }
	}
    } else if (-OJ_INFINITY == d) {
	if (ObjectMode == out->opts->mode) {
	    strcpy(buf, ninf_val);
	    cnt = sizeof(ninf_val) - 1;
	} else {
	    NanDump	nd = out->opts->dump_opts.nan_dump;
	    
	    if (AutoNan == nd) {
		switch (out->opts->mode) {
		case CompatMode:	nd = WordNan;	break;
		case StrictMode:	nd = RaiseNan;	break;
		case NullMode:		nd = NullNan;	break;
		default:				break;
		}
	    }
	    switch (nd) {
	    case RaiseNan:
		raise_strict(obj);
		break;
	    case WordNan:
		strcpy(buf, "-Infinity");
		cnt = 9;
		break;
	    case NullNan:
		strcpy(buf, "null");
		cnt = 4;
		break;
	    case HugeNan:
	    default:
		strcpy(buf, ninf_val);
		cnt = sizeof(ninf_val) - 1;
		break;
	    }
	}
    } else if (isnan(d)) {
	if (ObjectMode == out->opts->mode) {
	    strcpy(buf, nan_val);
	    cnt = sizeof(nan_val) - 1;
	} else {
	    NanDump	nd = out->opts->dump_opts.nan_dump;
	    
	    if (AutoNan == nd) {
		switch (out->opts->mode) {
		case CompatMode:	nd = WordNan;	break;
		case StrictMode:	nd = RaiseNan;	break;
		case NullMode:		nd = NullNan;	break;
		default:				break;
		}
	    }
	    switch (nd) {
	    case RaiseNan:
		raise_strict(obj);
		break;
	    case WordNan:
		strcpy(buf, "NaN");
		cnt = 3;
		break;
	    case NullNan:
		strcpy(buf, "null");
		cnt = 4;
		break;
	    case HugeNan:
	    default:
		strcpy(buf, nan_val);
		cnt = sizeof(nan_val) - 1;
		break;
	    }
	}
    } else if (d == (double)(long long int)d) {
	cnt = snprintf(buf, sizeof(buf), "%.1f", d);
    } else if (0 == out->opts->float_prec) {
	volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	cnt = RSTRING_LEN(rstr);
	if ((int)sizeof(buf) <= cnt) {
	    cnt = sizeof(buf) - 1;
	}
	strncpy(buf, rb_string_value_ptr((VALUE*)&rstr), cnt);
	buf[cnt] = '\0';
    } else {
	cnt = snprintf(buf, sizeof(buf), out->opts->float_fmt, d);
    }
    if (out->end - out->cur <= (long)cnt) {
	grow(out, cnt);
    }
    for (b = buf; '\0' != *b; b++) {
	*out->cur++ = *b;
    }
    *out->cur = '\0';
}

static void
dump_cstr(const char *str, size_t cnt, int is_sym, int escape1, Out out) {
    size_t	size;
    char	*cmap;

    switch (out->opts->escape_mode) {
    case NLEsc:
	cmap = newline_friendly_chars;
	size = newline_friendly_size((uint8_t*)str, cnt);
	break;
    case ASCIIEsc:
	cmap = ascii_friendly_chars;
	size = ascii_friendly_size((uint8_t*)str, cnt);
	break;
    case XSSEsc:
	cmap = xss_friendly_chars;
	size = xss_friendly_size((uint8_t*)str, cnt);
	break;
    case JSONEsc:
    default:
	cmap = hibit_friendly_chars;
	size = hibit_friendly_size((uint8_t*)str, cnt);
    }
    if (out->end - out->cur <= (long)size + BUFFER_EXTRA) { // extra 10 for escaped first char, quotes, and sym
	grow(out, size + BUFFER_EXTRA);
    }
    *out->cur++ = '"';
    if (escape1) {
	*out->cur++ = '\\';
	*out->cur++ = 'u';
	*out->cur++ = '0';
	*out->cur++ = '0';
	dump_hex((uint8_t)*str, out);
	cnt--;
	size--;
	str++;
	is_sym = 0; // just to make sure
    }
    if (cnt == size) {
	if (is_sym) {
	    *out->cur++ = ':';
	}
	for (; '\0' != *str; str++) {
	    *out->cur++ = *str;
	}
	*out->cur++ = '"';
    } else {
	const char	*end = str + cnt;

	if (is_sym) {
	    *out->cur++ = ':';
	}
	for (; str < end; str++) {
	    switch (cmap[(uint8_t)*str]) {
	    case '1':
		*out->cur++ = *str;
		break;
	    case '2':
		*out->cur++ = '\\';
		switch (*str) {
		case '\\':	*out->cur++ = '\\';	break;
		case '\b':	*out->cur++ = 'b';	break;
		case '\t':	*out->cur++ = 't';	break;
		case '\n':	*out->cur++ = 'n';	break;
		case '\f':	*out->cur++ = 'f';	break;
		case '\r':	*out->cur++ = 'r';	break;
		default:	*out->cur++ = *str;	break;
		}
		break;
	    case '3': // Unicode
		str = dump_unicode(str, end, out);
		break;
	    case '6': // control characters
		*out->cur++ = '\\';
		*out->cur++ = 'u';
		*out->cur++ = '0';
		*out->cur++ = '0';
		dump_hex((uint8_t)*str, out);
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
dump_str_comp(VALUE obj, Out out) {
    dump_cstr(rb_string_value_ptr((VALUE*)&obj), RSTRING_LEN(obj), 0, 0, out);
}

static void
dump_str_obj(VALUE obj, VALUE clas, int depth, Out out) {
    if (Qundef != clas && rb_cString != clas) {
	dump_obj_attrs(obj, clas, 0, depth, out);
    } else {
	const char	*s = rb_string_value_ptr((VALUE*)&obj);
	size_t		len = RSTRING_LEN(obj);
	char		s1 = s[1];

	dump_cstr(s, len, 0, (':' == *s || ('^' == *s && ('r' == s1 || 'i' == s1))), out);
    }
}

static void
dump_sym_comp(VALUE obj, Out out) {
    const char	*sym = rb_id2name(SYM2ID(obj));
    
    dump_cstr(sym, strlen(sym), 0, 0, out);
}

static void
dump_sym_obj(VALUE obj, Out out) {
    const char	*sym = rb_id2name(SYM2ID(obj));
    size_t	len = strlen(sym);
    
    dump_cstr(sym, len, 1, 0, out);
}

static void
dump_class_comp(VALUE obj, Out out) {
    const char	*s = rb_class2name(obj);

    dump_cstr(s, strlen(s), 0, 0, out);
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
    dump_cstr(s, len, 0, 0, out);
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
dump_array(VALUE a, VALUE clas, int depth, Out out) {
    size_t	size;
    int		i, cnt;
    int		d2 = depth + 1;
    long	id = check_circular(a, out);

    if (id < 0) {
	return;
    }
    if (Qundef != clas && rb_cArray != clas && ObjectMode == out->opts->mode) {
	dump_obj_attrs(a, clas, 0, depth, out);
	return;
    }
    cnt = (int)RARRAY_LEN(a);
    *out->cur++ = '[';
    if (0 < id) {
	size = d2 * out->indent + 16;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, d2);
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = 'i';
	dump_ulong(id, out);
	*out->cur++ = '"';
    }
    size = 2;
    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
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
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
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
	    dump_val(rb_ary_entry(a, i), d2, out, 0, 0);
	    if (i < cnt) {
		*out->cur++ = ',';
	    }
	}
	size = depth * out->indent + 1;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
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

static int
hash_cb_strict(VALUE key, VALUE value, Out out) {
    int		depth = out->depth;
    long	size;

    if (rb_type(key) != T_STRING) {
	rb_raise(rb_eTypeError, "In :strict mode all Hash keys must be Strings, not %s.\n", rb_class2name(rb_obj_class(key)));
    }
    if (!out->opts->dump_opts.use) {
	size = depth * out->indent + 1;
	if (out->end - out->cur <= size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
	dump_str_comp(key, out);
	*out->cur++ = ':';
    } else {
	size = depth * out->opts->dump_opts.indent_size + out->opts->dump_opts.hash_size + 1;
	if (out->end - out->cur <= size) {
	    grow(out, size);
	}
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
	dump_str_comp(key, out);
	size = out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2;
	if (out->end - out->cur <= size) {
	    grow(out, size);
	}
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
    dump_val(value, depth, out, 0, 0);
    out->depth = depth;
    *out->cur++ = ',';

    return ST_CONTINUE;
}

static int
hash_cb_compat(VALUE key, VALUE value, Out out) {
    int		depth = out->depth;
    long	size;

    if (!out->opts->dump_opts.use) {
	size = depth * out->indent + 1;
	if (out->end - out->cur <= size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
    } else {
	size = depth * out->opts->dump_opts.indent_size + out->opts->dump_opts.hash_size + 1;
	if (out->end - out->cur <= size) {
	    grow(out, size);
	}
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
	dump_str_comp(key, out);
	break;
    case T_SYMBOL:
	dump_sym_comp(key, out);
	break;
    default:
	/*rb_raise(rb_eTypeError, "In :compat mode all Hash keys must be Strings or Symbols, not %s.\n", rb_class2name(rb_obj_class(key)));*/
	dump_str_comp(rb_funcall(key, oj_to_s_id, 0), out);
	break;
    }
    if (!out->opts->dump_opts.use) {
	*out->cur++ = ':';
    } else {
	size = out->opts->dump_opts.before_size + out->opts->dump_opts.after_size + 2;
	if (out->end - out->cur <= size) {
	    grow(out, size);
	}
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
    dump_val(value, depth, out, 0, 0);
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
    if (rb_type(key) == T_STRING) {
	dump_str_obj(key, Qundef, depth, out);
	*out->cur++ = ':';
	dump_val(value, depth, out, 0, 0);
    } else if (rb_type(key) == T_SYMBOL) {
	dump_sym_obj(key, out);
	*out->cur++ = ':';
	dump_val(value, depth, out, 0, 0);
    } else {
	int	d2 = depth + 1;
	long	s2 = size + out->indent + 1;
	int	i;
	int	started = 0;
	uint8_t	b;

	if (out->end - out->cur <= s2 + 15) {
	    grow(out, s2 + 15);
	}
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = '#';
	out->hash_cnt++;
	for (i = 28; 0 <= i; i -= 4) {
	    b = (uint8_t)((out->hash_cnt >> i) & 0x0000000F);
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
	dump_val(key, d2, out, 0, 0);
	if (out->end - out->cur <= (long)s2) {
	    grow(out, s2);
	}
	*out->cur++ = ',';
	fill_indent(out, d2);
	dump_val(value, d2, out, 0, 0);
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
dump_hash(VALUE obj, VALUE clas, int depth, int mode, Out out) {
    int		cnt;
    size_t	size;

    if (Qundef != clas && rb_cHash != clas && ObjectMode == mode) {
	dump_obj_attrs(obj, clas, 0, depth, out);
	return;
    }
    cnt = (int)RHASH_SIZE(obj);
    size = depth * out->indent + 2;
    if (out->end - out->cur <= 2) {
	grow(out, 2);
    }
    if (0 == cnt) {
	*out->cur++ = '{';
	*out->cur++ = '}';
    } else {
	long	id = check_circular(obj, out);

	if (0 > id) {
	    return;
	}
	*out->cur++ = '{';
	if (0 < id) {
	    if (out->end - out->cur <= (long)size + 16) {
		grow(out, size + 16);
	    }
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
	if (ObjectMode == mode) {
	    rb_hash_foreach(obj, hash_cb_object, (VALUE)out);
	} else if (CompatMode == mode) {
	    rb_hash_foreach(obj, hash_cb_compat, (VALUE)out);
	} else {
	    rb_hash_foreach(obj, hash_cb_strict, (VALUE)out);
	}
	if (',' == *(out->cur - 1)) {
	    out->cur--; // backup to overwrite last comma
	}
	if (!out->opts->dump_opts.use) {
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, depth);
	} else {
	    size = depth * out->opts->dump_opts.indent_size + out->opts->dump_opts.hash_size + 1;
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
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
dump_time(VALUE obj, Out out, int withZone) {
    char		buf[64];
    char		*b = buf + sizeof(buf) - 1;
    long		size;
    char		*dot;
    int			neg = 0;
    long		one = 1000000000;
#if HAS_RB_TIME_TIMESPEC
    struct timespec	ts = rb_time_timespec(obj);
    time_t		sec = ts.tv_sec;
    long		nsec = ts.tv_nsec;
#else
    time_t		sec = NUM2LONG(rb_funcall2(obj, oj_tv_sec_id, 0, 0));
#if HAS_NANO_TIME
    long long		nsec = rb_num2ll(rb_funcall2(obj, oj_tv_nsec_id, 0, 0));
#else
    long long		nsec = rb_num2ll(rb_funcall2(obj, oj_tv_usec_id, 0, 0)) * 1000;
#endif
#endif
    
    *b-- = '\0';
    if (withZone) {
	long	tzsecs = NUM2LONG(rb_funcall2(obj, oj_utc_offset_id, 0, 0));
	int	zneg = (0 > tzsecs);

	if (0 == tzsecs && Qtrue == rb_funcall2(obj, oj_utcq_id, 0, 0)) {
	    tzsecs = 86400;
	}
	if (zneg) {
	    tzsecs = -tzsecs;
	}
	if (0 == tzsecs) {
	    *b-- = '0';
	} else {
	    for (; 0 < tzsecs; b--, tzsecs /= 10) {
		*b = '0' + (tzsecs % 10);
	    }
	    if (zneg) {
		*b-- = '-';
	    }
	}
	*b-- = 'e';
    }
    if (0 > sec) {
	neg = 1;
	sec = -sec;
	if (0 < nsec) {
	    nsec = 1000000000 - nsec;
	    sec--;
	}
    }
    dot = b - 9;
    if (0 < out->opts->sec_prec) {
	if (9 > out->opts->sec_prec) {
	    int	i;

	    for (i = 9 - out->opts->sec_prec; 0 < i; i--) {
		dot++;
		nsec = (nsec + 5) / 10;
		one /= 10;
	    }
	}
	if (one <= nsec) {
	    nsec -= one;
	    sec++;
	}
	for (; dot < b; b--, nsec /= 10) {
	    *b = '0' + (nsec % 10);
	}
	*b-- = '.';
    }
    if (0 == sec) {
	*b-- = '0';
    } else {
	for (; 0 < sec; b--, sec /= 10) {
	    *b = '0' + (sec % 10);
	}
    }
    if (neg) {
	*b-- = '-';
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
dump_ruby_time(VALUE obj, Out out) {
    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
}

static void
dump_xml_time(VALUE obj, Out out) {
    char		buf[64];
    struct tm		*tm;
    long		one = 1000000000;
#if HAS_RB_TIME_TIMESPEC
    struct timespec	ts = rb_time_timespec(obj);
    time_t		sec = ts.tv_sec;
    long		nsec = ts.tv_nsec;
#else
    time_t		sec = NUM2LONG(rb_funcall2(obj, oj_tv_sec_id, 0, 0));
#if HAS_NANO_TIME
    long long		nsec = rb_num2ll(rb_funcall2(obj, oj_tv_nsec_id, 0, 0));
#else
    long long		nsec = rb_num2ll(rb_funcall2(obj, oj_tv_usec_id, 0, 0)) * 1000;
#endif
#endif
    long		tzsecs = NUM2LONG(rb_funcall2(obj, oj_utc_offset_id, 0, 0));
    int			tzhour, tzmin;
    char		tzsign = '+';

    if (out->end - out->cur <= 36) {
	grow(out, 36);
    }
    if (9 > out->opts->sec_prec) {
	int	i;

	for (i = 9 - out->opts->sec_prec; 0 < i; i--) {
	    nsec = (nsec + 5) / 10;
	    one /= 10;
	}
	if (one <= nsec) {
	    nsec -= one;
	    sec++;
	}
    }
    // 2012-01-05T23:58:07.123456000+09:00
    //tm = localtime(&sec);
    sec += tzsecs;
    tm = gmtime(&sec);
#if 1
    if (0 > tzsecs) {
        tzsign = '-';
        tzhour = (int)(tzsecs / -3600);
        tzmin = (int)(tzsecs / -60) - (tzhour * 60);
    } else {
        tzhour = (int)(tzsecs / 3600);
        tzmin = (int)(tzsecs / 60) - (tzhour * 60);
    }
#else
    if (0 > tm->tm_gmtoff) {
        tzsign = '-';
        tzhour = (int)(tm->tm_gmtoff / -3600);
        tzmin = (int)(tm->tm_gmtoff / -60) - (tzhour * 60);
    } else {
        tzhour = (int)(tm->tm_gmtoff / 3600);
        tzmin = (int)(tm->tm_gmtoff / 60) - (tzhour * 60);
    }
#endif
    if (0 == nsec || 0 == out->opts->sec_prec) {
	if (0 == tzsecs && Qtrue == rb_funcall2(obj, oj_utcq_id, 0, 0)) {
	    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
		    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		    tm->tm_hour, tm->tm_min, tm->tm_sec);
	    dump_cstr(buf, 20, 0, 0, out);
	} else {
	    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",
		    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		    tm->tm_hour, tm->tm_min, tm->tm_sec,
		    tzsign, tzhour, tzmin);
	    dump_cstr(buf, 25, 0, 0, out);
	}
    } else if (0 == tzsecs && Qtrue == rb_funcall2(obj, oj_utcq_id, 0, 0)) {
	char	format[64] = "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ";
	int	len = 30;

	if (9 > out->opts->sec_prec) {
	    format[32] = '0' + out->opts->sec_prec;
	    len -= 9 - out->opts->sec_prec;
	}
	sprintf(buf, format,
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, nsec);
	dump_cstr(buf, len, 0, 0, out);
    } else {
	char	format[64] = "%04d-%02d-%02dT%02d:%02d:%02d.%09ld%c%02d:%02d";
	int	len = 35;

	if (9 > out->opts->sec_prec) {
	    format[32] = '0' + out->opts->sec_prec;
	    len -= 9 - out->opts->sec_prec;
	}
	sprintf(buf, format,
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, nsec,
		tzsign, tzhour, tzmin);
	dump_cstr(buf, len, 0, 0, out);
    }
}

static void
dump_data_strict(VALUE obj, Out out) {
    VALUE	clas = rb_obj_class(obj);

    if (oj_bigdecimal_class == clas) {
	volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	dump_raw(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), out);
    } else {
	raise_strict(obj);
    }
}

static void
dump_data_null(VALUE obj, Out out) {
    VALUE	clas = rb_obj_class(obj);

    if (oj_bigdecimal_class == clas) {
	volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	dump_raw(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), out);
    } else {
	dump_nil(out);
    }
}

static void
dump_data_comp(VALUE obj, int depth, Out out) {
    VALUE	clas = rb_obj_class(obj);

    if (rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);
 
	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    dump_val(h, depth, out, 0, 0);
	}
	dump_hash(h, Qundef, depth, out->opts->mode, out);

    } else if (Yes == out->opts->bigdec_as_num && oj_bigdecimal_class == clas) {
	volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	dump_raw(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), out);
    } else if (rb_respond_to(obj, oj_as_json_id)) {
	volatile VALUE	aj = rb_funcall(obj, oj_as_json_id, 0);

	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    dump_val(aj, depth, out, 0, 0);
	}
    } else if (Yes == out->opts->to_json && rb_respond_to(obj, oj_to_json_id)) {
	volatile VALUE	rs;
	const char	*s;
	int		len;

	rs = rb_funcall(obj, oj_to_json_id, 0);
	s = rb_string_value_ptr((VALUE*)&rs);
	len = (int)RSTRING_LEN(rs);

	if (out->end - out->cur <= len + 1) {
	    grow(out, len);
	}
	memcpy(out->cur, s, len);
	out->cur += len;
	*out->cur = '\0';
    } else {
	if (rb_cTime == clas) {
	    switch (out->opts->time_format) {
	    case RubyTime:	dump_ruby_time(obj, out);	break;
	    case XmlTime:	dump_xml_time(obj, out);	break;
	    case UnixZTime:	dump_time(obj, out, 1);		break;
	    case UnixTime:
	    default:		dump_time(obj, out, 0);		break;
	    }
	} else if (oj_bigdecimal_class == clas) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	}
    }
}

static void
dump_data_obj(VALUE obj, int depth, Out out) {
    VALUE	clas = rb_obj_class(obj);

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
	switch (out->opts->time_format) {
	case RubyTime: // Does not output fractional seconds
	case XmlTime:
	    dump_xml_time(obj, out);
	    break;
	case UnixZTime:
	    dump_time(obj, out, 1);
	    break;
	case UnixTime:
	default:
	    dump_time(obj, out, 0);
	    break;
	}
	*out->cur++ = '}';
	*out->cur = '\0';
    } else {
	if (oj_bigdecimal_class == clas) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);
		
	    if (Yes == out->opts->bigdec_as_num) {
		dump_raw(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), out);
	    } else {
		dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	    }
	} else {
	    dump_nil(out);
	}
    }
}

static void
dump_obj_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv) {
    if (rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);
 
	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    dump_val(h, depth, out, 0, 0);
	}
	dump_hash(h, Qundef, depth, out->opts->mode, out);
    } else if (rb_respond_to(obj, oj_as_json_id)) {
	volatile VALUE	aj = rb_funcall2(obj, oj_as_json_id, argc, argv);

	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    dump_val(aj, depth, out, 0, 0);
	}
    } else if (Yes == out->opts->to_json && rb_respond_to(obj, oj_to_json_id)) {
	volatile VALUE	rs;
	const char	*s;
	int		len;

	rs = rb_funcall(obj, oj_to_json_id, 0);
	s = rb_string_value_ptr((VALUE*)&rs);
	len = (int)RSTRING_LEN(rs);

	if (out->end - out->cur <= len + 1) {
	    grow(out, len);
	}
	memcpy(out->cur, s, len);
	out->cur += len;
	*out->cur = '\0';
    } else {
	VALUE	clas = rb_obj_class(obj);

	if (oj_bigdecimal_class == clas) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    if (Yes == out->opts->bigdec_as_num) {
		dump_raw(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), out);
	    } else {
		dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	    }
#if (defined T_RATIONAL && defined RRATIONAL)
	} else if (oj_datetime_class == clas || oj_date_class == clas || rb_cRational == clas) {
#else
	} else if (oj_datetime_class == clas || oj_date_class == clas) {
#endif
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    dump_obj_attrs(obj, Qundef, 0, depth, out);
	}
    }
    *out->cur = '\0';
}

inline static void
dump_obj_obj(VALUE obj, int depth, Out out) {
    long	id = check_circular(obj, out);

    if (0 <= id) {
	VALUE	clas = rb_obj_class(obj);

	if (oj_bigdecimal_class == clas) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    dump_raw(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), out);
	} else {
	    dump_obj_attrs(obj, clas, id, depth, out);
	}
    }
}

#ifdef RUBINIUS_RUBY
static int
isRbxHashAttr(const char *attr) {
    const char	*hashAttrs[] = {
	"@capacity",
	"@max_entries",
	"@state",
	"@mask",
	"@size",
	"@entries",
	"@default_proc",
	0 };
    const char	**ap;

    for (ap = hashAttrs; 0 != *ap; ap++) {
	if (0 == strcmp(attr, *ap)) {
	    return 1;
	}
    }
    return 0;
}
#endif

#if HAS_IVAR_HELPERS
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
    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
    fill_indent(out, depth);
    if ('@' == *attr) {
	attr++;
	dump_cstr(attr, strlen(attr), 0, 0, out);
    } else {
	char	buf[32];

	*buf = '~';
	strncpy(buf + 1, attr, sizeof(buf) - 2);
	buf[sizeof(buf) - 1] = '\0';
	dump_cstr(buf, strlen(buf), 0, 0, out);
    }
    *out->cur++ = ':';
    dump_val(value, depth, out, 0, 0);
    out->depth = depth;
    *out->cur++ = ',';
    
    return ST_CONTINUE;
}
#endif

static void
dump_obj_attrs(VALUE obj, VALUE clas, slot_t id, int depth, Out out) {
    size_t	size = 0;
    int		d2 = depth + 1;
    int		type = rb_type(obj);

    if (out->end - out->cur <= 2) {
	grow(out, 2);
    }
    *out->cur++ = '{';
    if (Qundef != clas) {
	const char	*class_name = rb_class2name(clas);
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
	dump_cstr(class_name, clen, 0, 0, out);
    }
    if (0 < id) {
	size = d2 * out->indent + 16;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	*out->cur++ = ',';
	fill_indent(out, d2);
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = 'i';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_ulong(id, out);
    }
    switch (type) {
    case T_STRING:
	size = d2 * out->indent + 14;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	*out->cur++ = ',';
	fill_indent(out, d2);
	*out->cur++ = '"';
	*out->cur++ = 's';
	*out->cur++ = 'e';
	*out->cur++ = 'l';
	*out->cur++ = 'f';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_cstr(rb_string_value_ptr((VALUE*)&obj), RSTRING_LEN(obj), 0, 0, out);
	break;
    case T_ARRAY:
	size = d2 * out->indent + 14;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	*out->cur++ = ',';
	fill_indent(out, d2);
	*out->cur++ = '"';
	*out->cur++ = 's';
	*out->cur++ = 'e';
	*out->cur++ = 'l';
	*out->cur++ = 'f';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_array(obj, Qundef, depth + 1, out);
	break;
    case T_HASH:
	size = d2 * out->indent + 14;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	*out->cur++ = ',';
	fill_indent(out, d2);
	*out->cur++ = '"';
	*out->cur++ = 's';
	*out->cur++ = 'e';
	*out->cur++ = 'l';
	*out->cur++ = 'f';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_hash(obj, Qundef, depth + 1, out->opts->mode, out);
	break;
    default:
	break;
    }
    {
	int	cnt;
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
	out->depth = depth + 1;
#if HAS_IVAR_HELPERS
	rb_ivar_foreach(obj, dump_attr_cb, (VALUE)out);
	if (',' == *(out->cur - 1)) {
	    out->cur--; // backup to overwrite last comma
	}
#else
	size = d2 * out->indent + 1;
	for (i = cnt; 0 < i; i--, np++) {
	    vid = rb_to_id(*np);
	    attr = rb_id2name(vid);
#ifdef RUBINIUS_RUBY
	    if (T_HASH == type && isRbxHashAttr(attr)) {
		continue;
	    }
#endif
	    if (first) {
		first = 0;
	    } else {
		*out->cur++ = ',';
	    }
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    if ('@' == *attr) {
		attr++;
		dump_cstr(attr, strlen(attr), 0, 0, out);
	    } else {
		char	buf[32];

		*buf = '~';
		strncpy(buf + 1, attr, sizeof(buf) - 2);
		buf[sizeof(buf) - 1] = '\0';
		dump_cstr(buf, strlen(attr) + 1, 0, 0, out);
	    }
	    *out->cur++ = ':';
	    dump_val(rb_ivar_get(obj, vid), d2, out, 0, 0);
	    if (out->end - out->cur <= 2) {
		grow(out, 2);
	    }
	}
#endif
#if HAS_EXCEPTION_MAGIC
	if (Qtrue == rb_obj_is_kind_of(obj, rb_eException)) {
	    volatile VALUE	rv;

	    if (',' != *(out->cur - 1)) {
		*out->cur++ = ',';
	    }
	    // message
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    dump_cstr("~mesg", 5, 0, 0, out);
	    *out->cur++ = ':';
	    rv = rb_funcall2(obj, rb_intern("message"), 0, 0);
	    dump_val(rv, d2, out, 0, 0);
	    if (out->end - out->cur <= 2) {
		grow(out, 2);
	    }
	    *out->cur++ = ',';
	    // backtrace
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    dump_cstr("~bt", 3, 0, 0, out);
	    *out->cur++ = ':';
	    rv = rb_funcall2(obj, rb_intern("backtrace"), 0, 0);
	    dump_val(rv, d2, out, 0, 0);
	    if (out->end - out->cur <= 2) {
		grow(out, 2);
	    }
	}
#endif
	out->depth = depth;
    }
    fill_indent(out, depth);
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
dump_struct_comp(VALUE obj, int depth, Out out) {
    if (rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);
 
	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    dump_val(h, depth, out, 0, 0);
	}
	dump_hash(h, Qundef, depth, out->opts->mode, out);
    } else if (rb_respond_to(obj, oj_as_json_id)) {
	volatile VALUE	aj = rb_funcall(obj, oj_as_json_id, 0);

	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    dump_val(aj, depth, out, 0, 0);
	}
    } else if (Yes == out->opts->to_json && rb_respond_to(obj, oj_to_json_id)) {
	volatile VALUE	rs = rb_funcall(obj, oj_to_json_id, 0);
	const char	*s;
	int		len;

	s = rb_string_value_ptr((VALUE*)&rs);
	len = (int)RSTRING_LEN(rs);
	if (out->end - out->cur <= len) {
	    grow(out, len);
	}
	memcpy(out->cur, s, len);
	out->cur += len;
    } else {
	volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
    }
}

static void
dump_struct_obj(VALUE obj, int depth, Out out) {
    VALUE	clas = rb_obj_class(obj);
    const char	*class_name = rb_class2name(clas);
    int		i;
    int		d2 = depth + 1;
    int		d3 = d2 + 1;
    size_t	len = strlen(class_name);
    size_t	size = d2 * out->indent + d3 * out->indent + 10 + len;

    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
    *out->cur++ = '{';
    fill_indent(out, d2);
    *out->cur++ = '"';
    *out->cur++ = '^';
    *out->cur++ = 'u';
    *out->cur++ = '"';
    *out->cur++ = ':';
    *out->cur++ = '[';
    if ('#' == *class_name) {
	VALUE		ma = rb_struct_s_members(clas);
	const char	*name;
	int		cnt = (int)RARRAY_LEN(ma);

	*out->cur++ = '[';
	for (i = 0; i < cnt; i++) {
	    name = rb_id2name(SYM2ID(rb_ary_entry(ma, i)));
	    len = strlen(name);
	    size = len + 3;
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    if (0 < i) {
		*out->cur++ = ',';
	    }
	    *out->cur++ = '"';
	    memcpy(out->cur, name, len);
	    out->cur += len;
	    *out->cur++ = '"';
	}
	*out->cur++ = ']';
    } else {
	fill_indent(out, d3);
	*out->cur++ = '"';
	memcpy(out->cur, class_name, len);
	out->cur += len;
	*out->cur++ = '"';
    }
    *out->cur++ = ',';
    size = d3 * out->indent + 2;
#ifdef RSTRUCT_LEN
    {
	const VALUE	*vp;

#ifndef RSTRUCT_CONST_PTR
# define RSTRUCT_CONST_PTR(st) (const VALUE *)RSTRUCT_PTR(st)
#endif
	for (i = (int)RSTRUCT_LEN(obj), vp = RSTRUCT_CONST_PTR(obj); 0 < i; i--, vp++) {
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d3);
	    dump_val(*vp, d3, out, 0, 0);
	    *out->cur++ = ',';
	}
    }
#else
    {
	// This is a bit risky as a struct in C ruby is not the same as a Struct
	// class in interpreted Ruby so length() may not be defined.
	int	slen = FIX2INT(rb_funcall2(obj, oj_length_id, 0, 0));

	for (i = 0; i < slen; i++) {
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d3);
	    dump_val(rb_struct_aref(obj, INT2FIX(i)), d3, out, 0, 0);
	    *out->cur++ = ',';
	}
    }
#endif
    out->cur--;
    *out->cur++ = ']';
    *out->cur++ = '}';
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

    if (out->end - out->cur <= 2) {
	grow(out, 2);
    }
    *out->cur++ = '{';
    if (Qundef != clas) {
	const char	*class_name = rb_class2name(clas);
	int		clen = (int)strlen(class_name);

	size = d2 * out->indent + clen + 10;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, d2);
	*out->cur++ = '"';
	*out->cur++ = '^';
	*out->cur++ = 'O';
	*out->cur++ = '"';
	*out->cur++ = ':';
	dump_cstr(class_name, clen, 0, 0, out);
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
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
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

	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
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
		ID		i;
	    
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
	    dump_cstr(name, nlen, 0, 0, out);
	    *out->cur++ = ':';
	    dump_val(v, d2, out, 0, 0);
	    if (out->end - out->cur <= 2) {
		grow(out, 2);
	    }
	    *out->cur++ = ',';
	}
	out->cur--;
    }
    *out->cur++ = '}';
    *out->cur = '\0';
}

static void
raise_strict(VALUE obj) {
    rb_raise(rb_eTypeError, "Failed to dump %s Object to JSON in strict mode.\n", rb_class2name(rb_obj_class(obj)));
}

static void
dump_val(VALUE obj, int depth, Out out, int argc, VALUE *argv) {
    int	type = rb_type(obj);

    if (MAX_DEPTH < depth) {
	rb_raise(rb_eNoMemError, "Too deeply nested.\n");
    }
    switch (type) {
    case T_NIL:		dump_nil(out);				break;
    case T_TRUE:	dump_true(out);				break;
    case T_FALSE:	dump_false(out);			break;
    case T_FIXNUM:	dump_fixnum(obj, out);			break;
    case T_FLOAT:	dump_float(obj, out);			break;
    case T_MODULE:
    case T_CLASS:
	switch (out->opts->mode) {
	case StrictMode:	raise_strict(obj);		break;
	case NullMode:		dump_nil(out);			break;
	case CompatMode:	dump_class_comp(obj, out);	break;
	case ObjectMode:
	default:		dump_class_obj(obj, out);	break;
	}
	break;
    case T_SYMBOL:
	switch (out->opts->mode) {
	case StrictMode:	raise_strict(obj);		break;
	case NullMode:		dump_nil(out);			break;
	case CompatMode:	dump_sym_comp(obj, out);	break;
	case ObjectMode:
	default:		dump_sym_obj(obj, out);		break;
	}
	break;
    case T_STRUCT: // for Range
	switch (out->opts->mode) {
	case StrictMode:	raise_strict(obj);		break;
	case NullMode:		dump_nil(out);			break;
	case CompatMode:	dump_struct_comp(obj, depth, out);	break;
	case ObjectMode:
	default:		dump_struct_obj(obj, depth, out);	break;
	}
	break;
    default:
	// Most developers have enough sense not to subclass primitive types but
	// since these classes could potentially be subclassed a check for odd
	// classes is performed.
	{
	    VALUE	clas = rb_obj_class(obj);
	    Odd		odd;

	    if (ObjectMode == out->opts->mode && 0 != (odd = oj_get_odd(clas))) {
		dump_odd(obj, odd, clas, depth + 1, out);
		return;
	    }
	    switch (type) {
	    case T_BIGNUM:		dump_bignum(obj, out);		break;
	    case T_STRING:
		switch (out->opts->mode) {
		case StrictMode:
		case NullMode:
		case CompatMode:	dump_str_comp(obj, out);	break;
		case ObjectMode:
		default:		dump_str_obj(obj, clas, depth, out);	break;
		}
		break;
	    case T_ARRAY:		dump_array(obj, clas, depth, out);	break;
	    case T_HASH:		dump_hash(obj, clas, depth, out->opts->mode, out);	break;
#if (defined T_RATIONAL && defined RRATIONAL)
	    case T_RATIONAL:
#endif
	    case T_OBJECT:
		switch (out->opts->mode) {
		case StrictMode:	dump_data_strict(obj, out);	break;
		case NullMode:		dump_data_null(obj, out);	break;
		case CompatMode:	dump_obj_comp(obj, depth, out, argc, argv);	break;
		case ObjectMode:
		default:		dump_obj_obj(obj, depth, out);	break;
		}
		break;
	    case T_DATA:
		switch (out->opts->mode) {
		case StrictMode:	dump_data_strict(obj, out);	break;
		case NullMode:		dump_data_null(obj, out);	break;
		case CompatMode:	dump_data_comp(obj, depth, out);break;
		case ObjectMode:
		default:		dump_data_obj(obj, depth, out);	break;
		}
		break;
#if (defined T_COMPLEX && defined RCOMPLEX)
	    case T_COMPLEX:
#endif
	    case T_REGEXP:
		switch (out->opts->mode) {
		case StrictMode:	raise_strict(obj);		break;
		case NullMode:		dump_nil(out);			break;
		case CompatMode:
		case ObjectMode:
		default:		dump_obj_comp(obj, depth, out, argc, argv);	break;
		}
		break;
	    default:
		switch (out->opts->mode) {
		case StrictMode:	raise_strict(obj);		break;
		case NullMode:		dump_nil(out);			break;
		case CompatMode: {
		    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);
		    dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
		    break;
		}
		case ObjectMode:
		default:
		    rb_raise(rb_eNotImpError, "Failed to dump '%s' Object (%02x)\n",
			     rb_class2name(rb_obj_class(obj)), rb_type(obj));
		    break;
		}
		break;
	    }
	}
    }
}

void
oj_dump_obj_to_json(VALUE obj, Options copts, Out out) {
    oj_dump_obj_to_json_using_params(obj, copts, out, 0, 0);
}

void
oj_dump_obj_to_json_using_params(VALUE obj, Options copts, Out out, int argc, VALUE *argv) {
    if (0 == out->buf) {
	out->buf = ALLOC_N(char, 4096);
	out->end = out->buf + 4095 - BUFFER_EXTRA; // 1 less than end plus extra for possible errors
	out->allocated = 1;
    }
    out->cur = out->buf;
    out->circ_cnt = 0;
    out->opts = copts;
    out->hash_cnt = 0;
    if (Yes == copts->circular) {
	oj_cache8_new(&out->circ_cache);
    }
    out->indent = copts->indent;
    dump_val(obj, 0, out, argc, argv);
    if (0 < out->indent) {
	switch (*(out->cur - 1)) {
	case ']':
	case '}':
	    grow(out, 1);
	    *out->cur++ = '\n';
	default:
	    break;
	}
    }
    *out->cur = '\0';
    if (Yes == copts->circular) {
	oj_cache8_delete(out->circ_cache);
    }
}

void
oj_write_obj_to_file(VALUE obj, const char *path, Options copts) {
    char	buf[4096];
    struct _Out out;
    size_t	size;
    FILE	*f;
    int		ok;

    out.buf = buf;
    out.end = buf + sizeof(buf) - BUFFER_EXTRA;
    out.allocated = 0;
    oj_dump_obj_to_json(obj, copts, &out);
    size = out.cur - out.buf;
    if (0 == (f = fopen(path, "w"))) {
	if (out.allocated) {
	    xfree(out.buf);
	}
	rb_raise(rb_eIOError, "%s\n", strerror(errno));
    }
    ok = (size == fwrite(out.buf, 1, size, f));
    if (out.allocated) {
	xfree(out.buf);
    }
    fclose(f);
    if (!ok) {
	int	err = ferror(f);

	rb_raise(rb_eIOError, "Write failed. [%d:%s]\n", err, strerror(err));
    }
}

void
oj_write_obj_to_stream(VALUE obj, VALUE stream, Options copts) {
    char	buf[4096];
    struct _Out out;
    ssize_t	size;
    VALUE	clas = rb_obj_class(stream);
#if !IS_WINDOWS
    int		fd;
    VALUE	s;
#endif

    out.buf = buf;
    out.end = buf + sizeof(buf) - BUFFER_EXTRA;
    out.allocated = 0;
    oj_dump_obj_to_json(obj, copts, &out);
    size = out.cur - out.buf;
    if (oj_stringio_class == clas) {
	rb_funcall(stream, oj_write_id, 1, rb_str_new(out.buf, size));
#if !IS_WINDOWS
    } else if (rb_respond_to(stream, oj_fileno_id) &&
	       Qnil != (s = rb_funcall(stream, oj_fileno_id, 0)) &&
	       0 != (fd = FIX2INT(s))) {
	if (size != write(fd, out.buf, size)) {
	    if (out.allocated) {
		xfree(out.buf);
	    }
	    rb_raise(rb_eIOError, "Write failed. [%d:%s]\n", errno, strerror(errno));
	}
#endif
    } else if (rb_respond_to(stream, oj_write_id)) {
	rb_funcall(stream, oj_write_id, 1, rb_str_new(out.buf, size));
    } else {
	if (out.allocated) {
	    xfree(out.buf);
	}
	rb_raise(rb_eArgError, "to_stream() expected an IO Object.");
    }
    if (out.allocated) {
	xfree(out.buf);
    }
}

// dump leaf functions

inline static void
dump_chars(const char *s, size_t size, Out out) {
    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
    memcpy(out->cur, s, size);
    out->cur += size;
    *out->cur = '\0';
}

static void
dump_leaf_str(Leaf leaf, Out out) {
    switch (leaf->value_type) {
    case STR_VAL:
	dump_cstr(leaf->str, strlen(leaf->str), 0, 0, out);
	break;
    case RUBY_VAL:
	dump_cstr(rb_string_value_cstr(&leaf->value), RSTRING_LEN(leaf->value), 0, 0, out);
	break;
    case COL_VAL:
    default:
	rb_raise(rb_eTypeError, "Unexpected value type %02x.\n", leaf->value_type);
	break;
    }
}

static void
dump_leaf_fixnum(Leaf leaf, Out out) {
    switch (leaf->value_type) {
    case STR_VAL:
	dump_chars(leaf->str, strlen(leaf->str), out);
	break;
    case RUBY_VAL:
	if (T_BIGNUM == rb_type(leaf->value)) {
	    dump_bignum(leaf->value, out);
	} else {
	    dump_fixnum(leaf->value, out);
	}
	break;
    case COL_VAL:
    default:
	rb_raise(rb_eTypeError, "Unexpected value type %02x.\n", leaf->value_type);
	break;
    }
}

static void
dump_leaf_float(Leaf leaf, Out out) {
    switch (leaf->value_type) {
    case STR_VAL:
	dump_chars(leaf->str, strlen(leaf->str), out);
	break;
    case RUBY_VAL:
	dump_float(leaf->value, out);
	break;
    case COL_VAL:
    default:
	rb_raise(rb_eTypeError, "Unexpected value type %02x.\n", leaf->value_type);
	break;
    }
}

static void
dump_leaf_array(Leaf leaf, int depth, Out out) {
    size_t	size;
    int		d2 = depth + 1;

    size = 2;
    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
    *out->cur++ = '[';
    if (0 == leaf->elements) {
	*out->cur++ = ']';
    } else {
	Leaf	first = leaf->elements->next;
	Leaf	e = first;

	size = d2 * out->indent + 2;
	do {
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    dump_leaf(e, d2, out);
	    if (e->next != first) {
		*out->cur++ = ',';
	    }
	    e = e->next;
	} while (e != first);
	size = depth * out->indent + 1;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
	*out->cur++ = ']';
    }
    *out->cur = '\0';
}

static void
dump_leaf_hash(Leaf leaf, int depth, Out out) {
    size_t	size;
    int		d2 = depth + 1;

    size = 2;
    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
    *out->cur++ = '{';
    if (0 == leaf->elements) {
	*out->cur++ = '}';
    } else {
	Leaf	first = leaf->elements->next;
	Leaf	e = first;

	size = d2 * out->indent + 2;
	do {
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    dump_cstr(e->key, strlen(e->key), 0, 0, out);
	    *out->cur++ = ':';
	    dump_leaf(e, d2, out);
	    if (e->next != first) {
		*out->cur++ = ',';
	    }
	    e = e->next;
	} while (e != first);
	size = depth * out->indent + 1;
	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	fill_indent(out, depth);
	*out->cur++ = '}';
    }
    *out->cur = '\0';
}

static void
dump_leaf(Leaf leaf, int depth, Out out) {
    switch (leaf->rtype) {
    case T_NIL:
	dump_nil(out);
	break;
    case T_TRUE:
	dump_true(out);
	break;
    case T_FALSE:
	dump_false(out);
	break;
    case T_STRING:
	dump_leaf_str(leaf, out);
	break;
    case T_FIXNUM:
	dump_leaf_fixnum(leaf, out);
	break;
    case T_FLOAT:
	dump_leaf_float(leaf, out);
	break;
    case T_ARRAY:
	dump_leaf_array(leaf, depth, out);
	break;
    case T_HASH:
	dump_leaf_hash(leaf, depth, out);
	break;
    default:
	rb_raise(rb_eTypeError, "Unexpected type %02x.\n", leaf->rtype);
	break;
    }
}

void
oj_dump_leaf_to_json(Leaf leaf, Options copts, Out out) {
    if (0 == out->buf) {
	out->buf = ALLOC_N(char, 4096);
	out->end = out->buf + 4095 - BUFFER_EXTRA; // 1 less than end plus extra for possible errors
	out->allocated = 1;
    }
    out->cur = out->buf;
    out->circ_cnt = 0;
    out->opts = copts;
    out->hash_cnt = 0;
    out->indent = copts->indent;
    dump_leaf(leaf, 0, out);
}

void
oj_write_leaf_to_file(Leaf leaf, const char *path, Options copts) {
    char	buf[4096];
    struct _Out out;
    size_t	size;
    FILE	*f;

    out.buf = buf;
    out.end = buf + sizeof(buf) - BUFFER_EXTRA;
    out.allocated = 0;
    oj_dump_leaf_to_json(leaf, copts, &out);
    size = out.cur - out.buf;
    if (0 == (f = fopen(path, "w"))) {
	rb_raise(rb_eIOError, "%s\n", strerror(errno));
    }
    if (size != fwrite(out.buf, 1, size, f)) {
	int	err = ferror(f);

	rb_raise(rb_eIOError, "Write failed. [%d:%s]\n", err, strerror(err));
    }
    if (out.allocated) {
	xfree(out.buf);
    }
    fclose(f);
}

// string writer functions

static void
key_check(StrWriter sw, const char *key) {
    DumpType	type = sw->types[sw->depth];

    if (0 == key && (ObjectNew == type || ObjectType == type)) {
	rb_raise(rb_eStandardError, "Can not push onto an Object without a key.");
    }
}

static void
push_type(StrWriter sw, DumpType type) {
    if (sw->types_end <= sw->types + sw->depth + 1) {
	size_t	size = (sw->types_end - sw->types) * 2;

	REALLOC_N(sw->types, char, size);
	sw->types_end = sw->types + size;
    }
    sw->depth++;
    sw->types[sw->depth] = type;
}

static void
maybe_comma(StrWriter sw) {
    switch (sw->types[sw->depth]) {
    case ObjectNew:
	sw->types[sw->depth] = ObjectType;
	break;
    case ArrayNew:
	sw->types[sw->depth] = ArrayType;
	break;
    case ObjectType:
    case ArrayType:
	// Always have a few characters available in the out.buf.
	*sw->out.cur++ = ',';
	break;
    }
}

void
oj_str_writer_push_key(StrWriter sw, const char *key) {
    DumpType	type = sw->types[sw->depth];
    long	size;

    if (sw->keyWritten) {
	rb_raise(rb_eStandardError, "Can not push more than one key before pushing a non-key.");
    }
    if (ObjectNew != type && ObjectType != type) {
	rb_raise(rb_eStandardError, "Can only push a key onto an Object.");
    }
    size = sw->depth * sw->out.indent + 3;
    if (sw->out.end - sw->out.cur <= (long)size) {
	grow(&sw->out, size);
    }
    maybe_comma(sw);
    if (0 < sw->depth) {
	fill_indent(&sw->out, sw->depth);
    }
    dump_cstr(key, strlen(key), 0, 0, &sw->out);
    *sw->out.cur++ = ':';
    sw->keyWritten = 1;
}

void
oj_str_writer_push_object(StrWriter sw, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
	if (sw->out.end - sw->out.cur <= 1) {
	    grow(&sw->out, 1);
	}
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	if (sw->out.end - sw->out.cur <= (long)size) {
	    grow(&sw->out, size);
	}
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    *sw->out.cur++ = '{';
    push_type(sw, ObjectNew);
}

void
oj_str_writer_push_array(StrWriter sw, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
	if (sw->out.end - sw->out.cur <= 1) {
	    grow(&sw->out, 1);
	}
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	if (sw->out.end - sw->out.cur <= size) {
	    grow(&sw->out, size);
	}
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    *sw->out.cur++ = '[';
    push_type(sw, ArrayNew);
}

void
oj_str_writer_push_value(StrWriter sw, VALUE val, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	if (sw->out.end - sw->out.cur <= size) {
	    grow(&sw->out, size);
	}
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    dump_val(val, sw->depth, &sw->out, 0, 0);
}

void
oj_str_writer_push_json(StrWriter sw, const char *json, const char *key) {
    if (sw->keyWritten) {
	sw->keyWritten = 0;
    } else {
	long	size;

	key_check(sw, key);
	size = sw->depth * sw->out.indent + 3;
	if (sw->out.end - sw->out.cur <= size) {
	    grow(&sw->out, size);
	}
	maybe_comma(sw);
	if (0 < sw->depth) {
	    fill_indent(&sw->out, sw->depth);
	}
	if (0 != key) {
	    dump_cstr(key, strlen(key), 0, 0, &sw->out);
	    *sw->out.cur++ = ':';
	}
    }
    dump_raw(json, strlen(json), &sw->out);
}

void
oj_str_writer_pop(StrWriter sw) {
    long	size;
    DumpType	type = sw->types[sw->depth];

    if (sw->keyWritten) {
	sw->keyWritten = 0;
	rb_raise(rb_eStandardError, "Can not pop after writing a key but no value.");
    }
    sw->depth--;
    if (0 > sw->depth) {
	rb_raise(rb_eStandardError, "Can not pop with no open array or object.");
    }
    size = sw->depth * sw->out.indent + 2;
    if (sw->out.end - sw->out.cur <= size) {
	grow(&sw->out, size);
    }
    fill_indent(&sw->out, sw->depth);
    switch (type) {
    case ObjectNew:
    case ObjectType:
	*sw->out.cur++ = '}';
	break;
    case ArrayNew:
    case ArrayType:
	*sw->out.cur++ = ']';
	break;
    }
    if (0 == sw->depth && 0 <= sw->out.indent) {
	*sw->out.cur++ = '\n';
    }
}

void
oj_str_writer_pop_all(StrWriter sw) {
    while (0 < sw->depth) {
	oj_str_writer_pop(sw);
    }
}
