/* compat.c
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

#include <stdio.h>

#include "oj.h"
#include "err.h"
#include "parse.h"
#include "resolve.h"
#include "hash.h"
#include "encode.h"

static void
hash_set_cstr(ParseInfo pi, const char *key, size_t klen, const char *str, size_t len, const char *orig) {
    Val	parent = stack_peek(&pi->stack);

    if (0 != pi->options.create_id &&
	*pi->options.create_id == *key &&
	pi->options.create_id_len == klen &&
	0 == strncmp(pi->options.create_id, key, klen)) {
	if (str < pi->json || pi->cur < str) {
	    parent->classname = oj_strndup(str, len);
	} else {
	    parent->classname = str;
	}
	parent->clen = len;
    } else {
	VALUE	rstr = rb_str_new(str, len);
	VALUE	rkey = rb_str_new(key, klen);

	rstr = oj_encode(rstr);
	rkey = oj_encode(rkey);
	if (Yes == pi->options.sym_key) {
	    rkey = rb_str_intern(rkey);
	}
	rb_hash_aset(parent->val, rkey, rstr);
    }
}

static void
end_hash(struct _ParseInfo *pi) {
    Val	parent = stack_peek(&pi->stack);

    if (0 != parent->classname) {
	VALUE	clas;

	clas = oj_name2class(pi, parent->classname, parent->clen, 0);
	if (Qundef != clas) { // else an error
	    parent->val = rb_funcall(clas, oj_json_create_id, 1, parent->val);
	}
	if (parent->classname < pi->json || pi->cur < parent->classname) {
	    xfree((char*)parent->classname);
	    parent->classname = 0;
	}
    }
}

void
oj_set_compat_callbacks(ParseInfo pi) {
    oj_set_strict_callbacks(pi);
    pi->end_hash = end_hash;
    pi->hash_set_cstr = hash_set_cstr;
}

VALUE
oj_compat_parse(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    oj_set_compat_callbacks(&pi);

    return oj_pi_parse(argc, argv, &pi, 0);
}

VALUE
oj_compat_parse_cstr(int argc, VALUE *argv, char *json) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    oj_set_strict_callbacks(&pi);
    pi.end_hash = end_hash;
    pi.hash_set_cstr = hash_set_cstr;

    return oj_pi_parse(argc, argv, &pi, json);
}
