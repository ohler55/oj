// Copyright (c) 2018 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef OJ_TRACE_H
#define OJ_TRACE_H

#include <ruby.h>
#include <stdbool.h>

typedef enum {
    TraceIn      = '}',
    TraceOut     = '{',
    TraceCall    = '-',
    TraceRubyIn  = '>',
    TraceRubyOut = '<',
} TraceWhere;

struct _parseInfo;

extern void
            oj_trace(const char *func, VALUE obj, const char *file, int line, int depth, TraceWhere where);
extern void oj_trace_parse_in(const char *func, struct _parseInfo *pi, const char *file, int line);
extern void
            oj_trace_parse_call(const char *func, struct _parseInfo *pi, const char *file, int line, VALUE obj);
extern void oj_trace_parse_hash_end(struct _parseInfo *pi, const char *file, int line);
extern void oj_trace_parse_array_end(struct _parseInfo *pi, const char *file, int line);

#endif /* OJ_TRACE_H */
