/* dump.c
 * Copyright (c) 2012, 2017, Peter Ohler
 * All rights reserved.
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
#include "dump.h"
#include "odd.h"

#if !HAS_ENCODING_SUPPORT || defined(RUBINIUS_RUBY)
#define rb_eEncodingError	rb_eException
#endif

// Workaround in case INFINITY is not defined in math.h or if the OS is CentOS
#define OJ_INFINITY (1.0/0.0)

#define MAX_DEPTH 1000

static const char	inf_val[] = INF_VAL;
static const char	ninf_val[] = NINF_VAL;
static const char	nan_val[] = NAN_VAL;

typedef unsigned long	ulong;

static void	raise_strict(VALUE obj);
static void	dump_hex(uint8_t c, Out out);
static void	dump_str_comp(VALUE obj, Out out);
static void	dump_sym_comp(VALUE obj, Out out);
static void	dump_class_comp(VALUE obj, Out out);
static int	hash_cb_compat(VALUE key, VALUE value, Out out);
static void	dump_hash(VALUE obj, VALUE clas, int depth, int mode, Out out);
static void	dump_data_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok);
static void	dump_obj_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok);
static void	dump_obj_to_s(VALUE obj, int depth, Out out);
static void	dump_struct_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok);
#if HAS_IVAR_HELPERS
static int	dump_attr_cb(ID key, VALUE value, Out out);
#endif
static void	dump_obj_attrs(VALUE obj, VALUE clas, slot_t id, int depth, Out out);

static void	grow(Out out, size_t len);
static size_t	hibit_friendly_size(const uint8_t *str, size_t len);
static size_t	xss_friendly_size(const uint8_t *str, size_t len);
static size_t	ascii_friendly_size(const uint8_t *str, size_t len);

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

// JSON XSS combo
static char	hixss_friendly_chars[256] = "\
66666666222622666666666666666666\
11211161111111111111111111116161\
11111111111111111111111111112111\
11111111111111111111111111111116\
11111111111111111111111111111111\
11111111111111111111111111111111\
11111111111111111111111111111111\
11311111111111111111111111111111";

inline static size_t
newline_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;
    size_t	i = len;

    for (; 0 < i; str++, i--) {
	size += newline_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static size_t
hibit_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;
    size_t	i = len;

    for (; 0 < i; str++, i--) {
	size += hibit_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static size_t
ascii_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;
    size_t	i = len;

    for (; 0 < i; str++, i--) {
	size += ascii_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static size_t
xss_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;
    size_t	i = len;

    for (; 0 < i; str++, i--) {
	size += xss_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

inline static size_t
hixss_friendly_size(const uint8_t *str, size_t len) {
    size_t	size = 0;
    size_t	i = len;

    for (; 0 < i; str++, i--) {
	size += hixss_friendly_chars[*str];
    }
    return size - len * (size_t)'0';
}

const char*
oj_nan_str(VALUE obj, int opt, int mode, bool plus, int *lenp) {
    const char	*str = NULL;
    
    if (AutoNan == opt) {
	switch (mode) {
	case CompatMode:	opt = WordNan;	break;
	case StrictMode:	opt = RaiseNan;	break;
	default:				break;
	}
    }
    switch (opt) {
    case RaiseNan:
	raise_strict(obj);
	break;
    case WordNan:
	if (plus) {
	    str = "Infinity";
	    *lenp = 8;
	} else {
	    str = "-Infinity";
	    *lenp = 9;
	}
	break;
    case NullNan:
	str = "null";
	*lenp = 4;
	break;
    case HugeNan:
    default:
	if (plus) {
	    str = inf_val;
	    *lenp = sizeof(inf_val) - 1;
	} else {
	    str = ninf_val;
	    *lenp = sizeof(ninf_val) - 1;
	}
	break;
    }
    return str;
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

static const char*
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
#if 1
long
oj_check_circular(VALUE obj, Out out) {
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
#else
long
oj_check_circular(VALUE obj, Out out) {
    slot_t	id = 0;
    slot_t	*slot;

    if (Yes == out->opts->circular) {
	if (0 == (id = oj_cache8_get(out->circ_cache, obj, &slot))) {
	    out->circ_cnt++;
	    id = out->circ_cnt;
	    *slot = id;
	} else {
	    if (ObjectMode == out->opts->mode) {
		if (out->end - out->cur <= 18) {
		    grow(out, 18);
		}
		*out->cur++ = '"';
		*out->cur++ = '^';
		*out->cur++ = 'r';
		dump_ulong(id, out);
		*out->cur++ = '"';
	    }
	    return -1;
	}
    }
    return (long)id;
}
#endif

static void
dump_str_comp(VALUE obj, Out out) {
    oj_dump_cstr(rb_string_value_ptr((VALUE*)&obj), RSTRING_LEN(obj), 0, 0, out);
}

static void
dump_sym_comp(VALUE obj, Out out) {
    const char	*sym = rb_id2name(SYM2ID(obj));
    
    oj_dump_cstr(sym, strlen(sym), 0, 0, out);
}

static void
dump_class_comp(VALUE obj, Out out) {
    const char	*s = rb_class2name(obj);

    oj_dump_cstr(s, strlen(s), 0, 0, out);
}

static void
dump_array(VALUE a, VALUE clas, int depth, Out out) {
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
	    oj_dump_comp_val(rb_ary_entry(a, i), d2, out, 0, 0, true);
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
hash_cb_compat(VALUE key, VALUE value, Out out) {
    int		depth = out->depth;
    long	size;

    if (out->omit_nil && Qnil == value) {
	return ST_CONTINUE;
    }
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
    oj_dump_comp_val(value, depth, out, 0, 0, true);
    out->depth = depth;
    *out->cur++ = ',';

    return ST_CONTINUE;
}

static void
dump_hash(VALUE obj, VALUE clas, int depth, int mode, Out out) {
    int		cnt;
    size_t	size;

    cnt = (int)RHASH_SIZE(obj);
    size = depth * out->indent + 2;
    if (out->end - out->cur <= 2) {
	grow(out, 2);
    }
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
	rb_hash_foreach(obj, hash_cb_compat, (VALUE)out);
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

void
oj_dump_time(VALUE obj, Out out, int withZone) {
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

	if (0 == tzsecs && rb_funcall2(obj, oj_utcq_id, 0, 0)) {
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

void
oj_dump_ruby_time(VALUE obj, Out out) {
    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
}

void
oj_dump_xml_time(VALUE obj, Out out) {
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

	// This is pretty lame but to be compatible with rails and active
	// support rounding is not done but instead a floor is done when
	// second precision is 3 just to be like rails. sigh.
	if (3 == out->opts->sec_prec) {
	    nsec /= 1000000;
	    one = 1000;
	} else {
	    for (i = 9 - out->opts->sec_prec; 0 < i; i--) {
		nsec = (nsec + 5) / 10;
		one /= 10;
	    }
	    if (one <= nsec) {
		nsec -= one;
		sec++;
	    }
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
	if (0 == tzsecs && rb_funcall2(obj, oj_utcq_id, 0, 0)) {
	    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02dZ",
		    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		    tm->tm_hour, tm->tm_min, tm->tm_sec);
	    oj_dump_cstr(buf, 20, 0, 0, out);
	} else {
	    sprintf(buf, "%04d-%02d-%02dT%02d:%02d:%02d%c%02d:%02d",
		    tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		    tm->tm_hour, tm->tm_min, tm->tm_sec,
		    tzsign, tzhour, tzmin);
	    oj_dump_cstr(buf, 25, 0, 0, out);
	}
    } else if (0 == tzsecs && rb_funcall2(obj, oj_utcq_id, 0, 0)) {
	char	format[64] = "%04d-%02d-%02dT%02d:%02d:%02d.%09ldZ";
	int	len = 30;

	if (9 > out->opts->sec_prec) {
	    format[32] = '0' + out->opts->sec_prec;
	    len -= 9 - out->opts->sec_prec;
	}
	sprintf(buf, format,
		tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
		tm->tm_hour, tm->tm_min, tm->tm_sec, nsec);
	oj_dump_cstr(buf, len, 0, 0, out);
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
	oj_dump_cstr(buf, len, 0, 0, out);
    }
}

void
dump_time(VALUE obj, Out out) {
    switch (out->opts->time_format) {
    case RubyTime:	oj_dump_ruby_time(obj, out);	break;
    case XmlTime:	oj_dump_xml_time(obj, out);	break;
    case UnixZTime:	oj_dump_time(obj, out, 1);	break;
    case UnixTime:
    default:		oj_dump_time(obj, out, 0);	break;
    }
}

static void
dump_data_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok) {
    VALUE	clas = rb_obj_class(obj);

    if (as_ok && Yes == out->opts->to_json && rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);

	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    oj_dump_comp_val(h, depth, out, 0, 0, false);
	}
	dump_hash(h, Qundef, depth, out->opts->mode, out);
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
	    if (1 <= argc) {
		aj = rb_funcall2(obj, oj_as_json_id, 1, argv);
	    } else {
		VALUE	nothing [1];

		nothing[0] = Qnil;
		aj = rb_funcall2(obj, oj_as_json_id, 1, nothing);
	    }
	    break;
	default:
	    aj = rb_funcall2(obj, oj_as_json_id, argc, argv);
	    break;
	}
#else
	aj = rb_funcall2(obj, oj_as_json_id, argc, argv);
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

	if (out->end - out->cur <= len + 1) {
	    grow(out, len);
	}
	memcpy(out->cur, s, len);
	out->cur += len;
	*out->cur = '\0';
    } else {
	if (rb_cTime == clas) {
	    dump_time(obj, out);
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
dump_obj_to_s(VALUE obj, int depth, Out out) {
    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
}

static void
dump_obj_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok) {
    if (as_ok && Yes == out->opts->to_json && rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);

	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    oj_dump_comp_val(h, depth, out, 0, 0, false);
	} else {
	    dump_hash(h, Qundef, depth, out->opts->mode, out);
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
	    if (1 <= argc) {
		aj = rb_funcall2(obj, oj_as_json_id, 1, argv);
	    } else {
		VALUE	nothing [1];

		nothing[0] = Qnil;
		aj = rb_funcall2(obj, oj_as_json_id, 1, nothing);
	    }
	    break;
	default:
	    aj = rb_funcall2(obj, oj_as_json_id, argc, argv);
	    break;
	}
#else
	aj = rb_funcall2(obj, oj_as_json_id, argc, argv);
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
    if (out->end - out->cur <= (long)size) {
	grow(out, size);
    }
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
	oj_dump_cstr(class_name, clen, 0, 0, out);
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
	oj_dump_cstr(rb_string_value_ptr((VALUE*)&obj), RSTRING_LEN(obj), 0, 0, out);
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
	if (0 == cnt && Qundef == clas) {
	    // Might be something special like an Enumerable.
	    if (Qtrue == rb_obj_is_kind_of(obj, oj_enumerable_class)) {
		out->cur--;
		oj_dump_comp_val(rb_funcall(obj, rb_intern("entries"), 0), depth, out, 0, 0, false);
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
#ifdef RUBINIUS_RUBY
	    if (T_HASH == type && isRbxHashAttr(attr)) {
		continue;
	    }
#endif
	    value = rb_ivar_get(obj, vid);
	    if (out->omit_nil && Qnil == value) {
		continue;
	    }
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
		oj_dump_cstr(attr, strlen(attr), 0, 0, out);
	    } else {
		char	buf[32];

		*buf = '~';
		strncpy(buf + 1, attr, sizeof(buf) - 2);
		buf[sizeof(buf) - 1] = '\0';
		oj_dump_cstr(buf, strlen(attr) + 1, 0, 0, out);
	    }
	    *out->cur++ = ':';
	    oj_dump_comp_val(value, d2, out, 0, 0, true);
	    if (out->end - out->cur <= 2) {
		grow(out, 2);
	    }
	}
#endif
#if HAS_EXCEPTION_MAGIC
	if (rb_obj_is_kind_of(obj, rb_eException)) {
	    volatile VALUE	rv;

	    if (',' != *(out->cur - 1)) {
		*out->cur++ = ',';
	    }
	    // message
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    oj_dump_cstr("~mesg", 5, 0, 0, out);
	    *out->cur++ = ':';
	    rv = rb_funcall2(obj, rb_intern("message"), 0, 0);
	    oj_dump_comp_val(rv, d2, out, 0, 0, true);
	    if (out->end - out->cur <= 2) {
		grow(out, 2);
	    }
	    *out->cur++ = ',';
	    // backtrace
	    if (out->end - out->cur <= (long)size) {
		grow(out, size);
	    }
	    fill_indent(out, d2);
	    oj_dump_cstr("~bt", 3, 0, 0, out);
	    *out->cur++ = ':';
	    rv = rb_funcall2(obj, rb_intern("backtrace"), 0, 0);
	    oj_dump_comp_val(rv, d2, out, 0, 0, true);
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
dump_struct_comp(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok) {
    if (as_ok && Yes == out->opts->to_json && rb_respond_to(obj, oj_to_hash_id)) {
	volatile VALUE	h = rb_funcall(obj, oj_to_hash_id, 0);
 
	if (T_HASH != rb_type(h)) {
	    // It seems that ActiveRecord implemented to_hash so that it returns
	    // an Array and not a Hash. To get around that any value returned
	    // will be dumped.

	    //rb_raise(rb_eTypeError, "%s.to_hash() did not return a Hash.\n", rb_class2name(rb_obj_class(obj)));
	    oj_dump_comp_val(h, depth, out, 0, 0, false);
	}
	dump_hash(h, Qundef, depth, out->opts->mode, out);
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
	    if (1 <= argc) {
		aj = rb_funcall2(obj, oj_as_json_id, 1, argv);
	    } else {
		VALUE	nothing [1];

		nothing[0] = Qnil;
		aj = rb_funcall2(obj, oj_as_json_id, 1, nothing);
	    }
	    break;
	default:
	    aj = rb_funcall2(obj, oj_as_json_id, argc, argv);
	    break;
	}
#else
	aj = rb_funcall2(obj, oj_as_json_id, argc, argv);
#endif
	// Catch the obvious brain damaged recursive dumping.
	if (aj == obj) {
	    volatile VALUE	rstr = rb_funcall(obj, oj_to_s_id, 0);

	    oj_dump_cstr(rb_string_value_ptr((VALUE*)&rstr), RSTRING_LEN(rstr), 0, 0, out);
	} else {
	    oj_dump_comp_val(aj, depth, out, 0, 0, false);
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

	if (out->end - out->cur <= (long)size) {
	    grow(out, size);
	}
	if (clas == rb_cRange) {
	    *out->cur++ = '"';
	    oj_dump_comp_val(rb_funcall(obj, oj_begin_id, 0), d3, out, 0, 0, false);
	    if (out->end - out->cur <= (long)3) {
		grow(out, 3);
	    }
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
	    if (out->end - out->cur <= (long)(size + len + 3)) {
		grow(out, size + len + 3);
	    }
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
raise_strict(VALUE obj) {
    rb_raise(rb_eTypeError, "Failed to dump %s Object to JSON in strict mode.\n", rb_class2name(rb_obj_class(obj)));
}

void
oj_dump_comp_val(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok) {
    int	type = rb_type(obj);

    if (MAX_DEPTH < depth) {
	rb_raise(rb_eNoMemError, "Too deeply nested.\n");
    }
#ifdef OJ_DEBUG
    printf("Oj-debug: oj_dump_comp_val %s\n", rb_class2name(rb_obj_class(obj)));
#endif
    //printf("*** Oj-debug: oj_dump_comp_val %s type: %02x\n", rb_class2name(rb_obj_class(obj)), type);
    switch (type) {
    case T_NIL:		oj_dump_nil(Qnil, 0, out);		break;
    case T_TRUE:	oj_dump_true(Qtrue, 0, out);		break;
    case T_FALSE:	oj_dump_false(Qfalse, 0, out);		break;
    case T_FIXNUM:	oj_dump_fixnum(obj, 0, out);		break;
    case T_FLOAT:	oj_dump_float(obj, 0, out);		break;
    case T_MODULE:
    case T_CLASS:
	dump_class_comp(obj, out);
	break;
    case T_SYMBOL:
	dump_sym_comp(obj, out);
	break;
    case T_STRUCT: // for Range
	dump_struct_comp(obj, depth, out, argc, argv, as_ok);
	break;
    default:
	// Most developers have enough sense not to subclass primitive types but
	// since these classes could potentially be subclassed a check for odd
	// classes is performed.
	{
	    VALUE	clas = rb_obj_class(obj);

	    switch (type) {
	    case T_BIGNUM:		oj_dump_bignum(obj, 0, out);		break;
	    case T_STRING:
		dump_str_comp(obj, out);
		break;
	    case T_ARRAY:		dump_array(obj, clas, depth, out);	break;
	    case T_HASH:		dump_hash(obj, clas, depth, out->opts->mode, out);	break;
#if (defined T_RATIONAL && defined RRATIONAL)
	    case T_RATIONAL:
#endif
	    case T_OBJECT:
		dump_obj_comp(obj, depth, out, argc, argv, as_ok);
		break;
	    case T_DATA:
		dump_data_comp(obj, depth, out, argc, argv, as_ok);
		break;
#if (defined T_COMPLEX && defined RCOMPLEX)
	    case T_COMPLEX:
#endif
	    case T_REGEXP:
		dump_obj_to_s(obj, depth, out);
		break;
	    default:
		dump_obj_to_s(obj, depth, out);
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
    out->indent = copts->indent;
    if (Yes == copts->circular) {
	oj_cache8_new(&out->circ_cache);
    }
    switch (copts->mode) {
    case StrictMode:	oj_dump_strict_val(obj, 0, out);		break;
    case NullMode:	oj_dump_null_val(obj, 0, out);			break;
    case ObjectMode:	oj_dump_obj_val(obj, 0, out);			break;
    case CompatMode:	oj_dump_comp_val(obj, 0, out, argc, argv, true);	break;
    default:		oj_dump_comp_val(obj, 0, out, argc, argv, true);	break;
    }
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
    out.omit_nil = copts->dump_opts.omit_nil;
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
    out.omit_nil = copts->dump_opts.omit_nil;
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

//////////////////////////////////

void
oj_dump_cstr(const char *str, size_t cnt, int is_sym, int escape1, Out out) {
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
    case JXEsc:
	cmap = hixss_friendly_chars;
	size = hixss_friendly_size((uint8_t*)str, cnt);
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

void
oj_dump_raw(const char *str, size_t cnt, Out out) {
    if (out->end - out->cur <= (long)cnt + 10) {
	grow(out, cnt + 10);
    }
    memcpy(out->cur, str, cnt);
    out->cur += cnt;
    *out->cur = '\0';
}

void
oj_grow_out(Out out, size_t len) {
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
