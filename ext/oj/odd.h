/* odd.h
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

#ifndef __OJ_ODD_H__
#define __OJ_ODD_H__

#include <stdbool.h>

#include "ruby.h"

#define MAX_ODD_ARGS	10

typedef VALUE	(*AttrGetFunc)(VALUE obj);

typedef struct _Odd {
    const char	*classname;
    size_t	clen;
    VALUE	clas;			// Ruby class or module
    VALUE	create_obj;
    ID		create_op;
    int		attr_cnt;
    bool	is_module;
    bool	raw;
    const char	*attr_names[MAX_ODD_ARGS];	// 0 terminated attr IDs
    ID		attrs[MAX_ODD_ARGS];	// 0 terminated attr IDs
    AttrGetFunc	attrFuncs[MAX_ODD_ARGS];
} *Odd;

typedef struct _OddArgs {
    Odd		odd;
    VALUE	args[MAX_ODD_ARGS];
} *OddArgs;

extern void	oj_odd_init(void);
extern Odd	oj_get_odd(VALUE clas);
extern Odd	oj_get_oddc(const char *classname, size_t len);
extern OddArgs	oj_odd_alloc_args(Odd odd);
extern void	oj_odd_free(OddArgs args);
extern int	oj_odd_set_arg(OddArgs args, const char *key, size_t klen, VALUE value);
extern void	oj_reg_odd(VALUE clas, VALUE create_object, VALUE create_method, int mcnt, VALUE *members, bool raw);

#endif /* __OJ_ODD_H__ */
