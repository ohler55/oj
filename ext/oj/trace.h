/* trace.h
 * Copyright (c) 2018, Peter Ohler
 * All rights reserved.
 */

#ifndef __OJ_TRACE_H__
#define __OJ_TRACE_H__

#include <stdbool.h>
#include <ruby.h>

extern void	oj_trace(const char *func, VALUE obj, const char *file, int line, int depth, bool in);

#endif /* __OJ_TRACE_H__ */
