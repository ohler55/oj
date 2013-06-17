/* err.c
 * Copyright (c) 2011, Peter Ohler
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

#include <stdarg.h>

#include "err.h"

void
oj_err_set(Err e, VALUE clas, const char *format, ...) {
    va_list	ap;

    va_start(ap, format);
    e->clas = clas;
    vsnprintf(e->msg, sizeof(e->msg) - 1, format, ap);
    va_end(ap);
}

void
oj_err_raise(Err e) {
    rb_raise(e->clas, "%s", e->msg);
}

void
_oj_err_set_with_location(Err err, VALUE eclas, const char *msg, const char *json, const char *current, const char* file, int line) {
    int	n = 1;
    int	col = 1;

    for (; json < current && '\n' != *current; current--) {
	col++;
    }
    for (; json < current; current--) {
	if ('\n' == *current) {
	    n++;
	}
    }
    oj_err_set(err, eclas, "%s at line %d, column %d [%s:%d]", msg, n, col, file, line);
}

void
_oj_raise_error(const char *msg, const char *json, const char *current, const char* file, int line) {
    struct _Err	err;
    int		n = 1;
    int		col = 1;

    for (; json < current && '\n' != *current; current--) {
	col++;
    }
    for (; json < current; current--) {
	if ('\n' == *current) {
	    n++;
	}
    }
    oj_err_set(&err, oj_parse_error_class, "%s at line %d, column %d [%s:%d]", msg, n, col, file, line);
    rb_raise(err.clas, "%s", err.msg);
}
