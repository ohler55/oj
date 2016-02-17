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
hash_set_cstr(ParseInfo pi, Val kval, const char *str, size_t len, const char *orig) {
    const char		*key = kval->key;
    int			klen = kval->klen;
    Val			parent = stack_peek(&pi->stack);
    volatile VALUE	rkey = kval->key_val;

    if (Qundef == rkey &&
	0 != pi->options.create_id &&
	*pi->options.create_id == *key &&
	(int)pi->options.create_id_len == klen &&
	0 == strncmp(pi->options.create_id, key, klen)) {
	parent->classname = oj_strndup(str, len);
	parent->clen = len;
    } else {
	volatile VALUE	rstr = rb_str_new(str, len);

	if (Qundef == rkey) {
	    rkey = rb_str_new(key, klen);
	    rstr = oj_encode(rstr);
	    rkey = oj_encode(rkey);
	    if (Yes == pi->options.sym_key) {
		rkey = rb_str_intern(rkey);
	    }
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
	if (0 != parent->classname) {
	    xfree((char*)parent->classname);
	    parent->classname = 0;
	}
    }
}

static VALUE
calc_hash_key(ParseInfo pi, Val parent) {
    volatile VALUE	rkey = parent->key_val;

    if (Qundef == rkey) {
	rkey = rb_str_new(parent->key, parent->klen);
    }
    rkey = oj_encode(rkey);
    if (Yes == pi->options.sym_key) {
	rkey = rb_str_intern(rkey);
    }
    return rkey;
}

static void
add_num(ParseInfo pi, NumInfo ni) {
    pi->stack.head->val = oj_num_as_value(ni);
}

static void
hash_set_num(struct _ParseInfo *pi, Val parent, NumInfo ni) {
    rb_hash_aset(stack_peek(&pi->stack)->val, calc_hash_key(pi, parent), oj_num_as_value(ni));
}

static void
array_append_num(ParseInfo pi, NumInfo ni) {
    rb_ary_push(stack_peek(&pi->stack)->val, oj_num_as_value(ni));
}


void
oj_set_compat_callbacks(ParseInfo pi) {
    oj_set_strict_callbacks(pi);
    pi->end_hash = end_hash;
    pi->hash_set_cstr = hash_set_cstr;
    pi->add_num = add_num;
    pi->hash_set_num = hash_set_num;
    pi->array_append_num = array_append_num;
}

VALUE
oj_compat_parse(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    pi.handler = Qnil;
    pi.err_class = Qnil;
    oj_set_compat_callbacks(&pi);

    if (T_STRING == rb_type(*argv)) {
	return oj_pi_parse(argc, argv, &pi, 0, 0, 1);
    } else {
	return oj_pi_sparse(argc, argv, &pi, 0);
    }
}

VALUE
oj_compat_parse_cstr(int argc, VALUE *argv, char *json, size_t len) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    pi.handler = Qnil;
    pi.err_class = Qnil;
    oj_set_strict_callbacks(&pi);
    pi.end_hash = end_hash;
    pi.hash_set_cstr = hash_set_cstr;

    return oj_pi_parse(argc, argv, &pi, json, len, 1);
}
