/* trace.h
 * Copyright (c) 2018, Peter Ohler
 * All rights reserved.
 */

#include "trace.h"

void
oj_trace(const char *func, VALUE obj, const char *file, int line, int depth, bool in) {
    char	fmt[64];
    int		thread = 0;
    char	indent[256];

    depth *= 2;
    if ((int)sizeof(indent) <= depth) {
	depth = sizeof(indent) - 1;
    }
    memset(indent, ' ', depth);
    indent[depth] = '\0';
    
    sprintf(fmt, "%%d:%%s:%%d:Oj:%c:%%%ds %%s %%s\n", in ? '}' : '{', depth);

    printf(fmt, thread, file, line, indent, func, rb_obj_classname(obj));
}
