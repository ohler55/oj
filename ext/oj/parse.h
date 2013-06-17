/* parse.h
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

#ifndef __OJ_PARSE_H__
#define __OJ_PARSE_H__

#include <stdarg.h>

#include "ruby.h"
#include "oj.h"
#include "val_stack.h"
#include "circarray.h"

typedef struct _NumInfo {
    int64_t	i;
    int64_t	num;
    int64_t	div;
    const char	*str;
    size_t	len;
    long	exp;
    int		dec_cnt;
    int		big;
    int		infinity;
    int		neg;
} *NumInfo;

typedef struct _ParseInfo {
    const char		*json;
    const char		*cur;
    struct _Err		err;
    struct _Options	options;
    void		*cbc;
    struct _ValStack	stack;
    CircArray		circ_array;
    int			expect_value;
    VALUE		(*start_hash)(struct _ParseInfo *pi);
    void		(*end_hash)(struct _ParseInfo *pi);
    void		(*hash_set_cstr)(struct _ParseInfo *pi, const char *key, size_t klen, const char *str, size_t len, const char *orig);
    void		(*hash_set_num)(struct _ParseInfo *pi, const char *key, size_t klen, NumInfo ni);
    void		(*hash_set_value)(struct _ParseInfo *pi, const char *key, size_t klen, VALUE value);

    VALUE		(*start_array)(struct _ParseInfo *pi);
    void		(*end_array)(struct _ParseInfo *pi);
    void		(*array_append_cstr)(struct _ParseInfo *pi, const char *str, size_t len, const char *orig);
    void		(*array_append_num)(struct _ParseInfo *pi, NumInfo ni);
    void		(*array_append_value)(struct _ParseInfo *pi, VALUE value);

    void		(*add_cstr)(struct _ParseInfo *pi, const char *str, size_t len, const char *orig);
    void		(*add_num)(struct _ParseInfo *pi, NumInfo ni);
    void		(*add_value)(struct _ParseInfo *pi, VALUE val);
} *ParseInfo;

extern void	oj_parse2(ParseInfo pi);
extern void	oj_set_error_at(ParseInfo pi, VALUE err_clas, const char* file, int line, const char *format, ...);
extern VALUE	oj_pi_parse(int argc, VALUE *argv, ParseInfo pi, char *json);
extern VALUE	oj_num_as_value(NumInfo ni);

extern void	oj_set_strict_callbacks(ParseInfo pi);
extern void	oj_set_compat_callbacks(ParseInfo pi);

#endif /* __OJ_PARSE_H__ */
