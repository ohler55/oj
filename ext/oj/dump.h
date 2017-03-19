/* dump.h
 * Copyright (c) 2011, Peter Ohler
 * All rights reserved.
 */

#ifndef __OJ_DUMP_H__
#define __OJ_DUMP_H__

#include <ruby.h>

#include "oj.h"

// Extra padding at end of buffer.
#define BUFFER_EXTRA 10

typedef void	(*DumpFunc)(VALUE obj, int depth, Out out);

extern void	oj_dump_nil(VALUE obj, int depth, Out out);
extern void	oj_dump_true(VALUE obj, int depth, Out out);
extern void	oj_dump_false(VALUE obj, int depth, Out out);
extern void	oj_dump_fixnum(VALUE obj, int depth, Out out);
extern void	oj_dump_bignum(VALUE obj, int depth, Out out);
extern void	oj_dump_float(VALUE obj, int depth, Out out);
extern void	oj_dump_str(VALUE obj, int depth, Out out);
extern void	oj_dump_sym(VALUE obj, int depth, Out out);

extern void	oj_dump_raw(const char *str, size_t cnt, Out out);
extern void	oj_dump_cstr(const char *str, size_t cnt, int is_sym, int escape1, Out out);
extern void	oj_grow_out(Out out, size_t len);

extern void	oj_dump_strict_val(VALUE obj, int depth, Out out);
extern void	oj_dump_null_val(VALUE obj, int depth, Out out);
extern void	oj_dump_val(VALUE obj, int depth, Out out, int argc, VALUE *argv, bool as_ok);



inline static void
assure_size(Out out, size_t len) {
    if (out->end - out->cur <= (long)len) {
	oj_grow_out(out, len);
    }
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

#endif /* __OJ_DUMP_H__ */
