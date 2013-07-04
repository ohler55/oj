/* strict.c
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "oj.h"
#include "err.h"
#include "parse.h"
#include "encode.h"

// Workaround in case INFINITY is not defined in math.h or if the OS is CentOS
#define OJ_INFINITY (1.0/0.0)

static void
noop_end(struct _ParseInfo *pi) {
}

static void
add_value(ParseInfo pi, VALUE val) {
    pi->stack.head->val = val;
}

static void
add_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    VALUE	rstr = rb_str_new(str, len);

    rstr = oj_encode(rstr);
    pi->stack.head->val = rstr;
}

static void
add_num(ParseInfo pi, NumInfo ni) {
    pi->stack.head->val = oj_num_as_value(ni);
}

static VALUE
start_hash(ParseInfo pi) {
    return rb_hash_new();
}

static VALUE
hash_key(ParseInfo pi, const char *key, size_t klen) {
    VALUE	rkey = rb_str_new(key, klen);

    rkey = oj_encode(rkey);
    if (Yes == pi->options.sym_key) {
	rkey = rb_str_intern(rkey);
    }
    return rkey;
}

static void
hash_set_cstr(ParseInfo pi, const char *key, size_t klen, const char *str, size_t len, const char *orig) {
    VALUE	rstr = rb_str_new(str, len);

    rstr = oj_encode(rstr);
    rb_hash_aset(stack_peek(&pi->stack)->val, hash_key(pi, key, klen), rstr);
}

static void
hash_set_num(struct _ParseInfo *pi, const char *key, size_t klen, NumInfo ni) {
    rb_hash_aset(stack_peek(&pi->stack)->val, hash_key(pi, key, klen), oj_num_as_value(ni));
}

static void
hash_set_value(ParseInfo pi, const char *key, size_t klen, VALUE value) {
    rb_hash_aset(stack_peek(&pi->stack)->val, hash_key(pi, key, klen), value);
}

static VALUE
start_array(ParseInfo pi) {
    return rb_ary_new();
}

static void
array_append_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    VALUE	rstr = rb_str_new(str, len);

    rstr = oj_encode(rstr);
    rb_ary_push(stack_peek(&pi->stack)->val, rstr);
}

static void
array_append_num(ParseInfo pi, NumInfo ni) {
    rb_ary_push(stack_peek(&pi->stack)->val, oj_num_as_value(ni));
}

static void
array_append_value(ParseInfo pi, VALUE value) {
    rb_ary_push(stack_peek(&pi->stack)->val, value);
}

void
oj_set_strict_callbacks(ParseInfo pi) {
    pi->start_hash = start_hash;
    pi->end_hash = noop_end;
    pi->hash_set_cstr = hash_set_cstr;
    pi->hash_set_num = hash_set_num;
    pi->hash_set_value = hash_set_value;
    pi->start_array = start_array;
    pi->end_array = noop_end;
    pi->array_append_cstr = array_append_cstr;
    pi->array_append_num = array_append_num;
    pi->array_append_value = array_append_value;
    pi->add_cstr = add_cstr;
    pi->add_num = add_num;
    pi->add_value = add_value;
    pi->expect_value = 1;
}

VALUE
oj_strict_parse(int argc, VALUE *argv, VALUE self) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    oj_set_strict_callbacks(&pi);

    return oj_pi_parse(argc, argv, &pi, 0);
}

VALUE
oj_strict_parse_cstr(int argc, VALUE *argv, char *json) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    oj_set_strict_callbacks(&pi);

    return oj_pi_parse(argc, argv, &pi, json);
}
