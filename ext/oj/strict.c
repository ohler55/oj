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

static void
noop_end(struct _ParseInfo *pi) {
}

static VALUE
noop_hash_key(struct _ParseInfo *pi, const char *key, size_t klen) {
    return Qundef;
}

static void
add_value(ParseInfo pi, VALUE val) {
    pi->stack.head->val = val;
}

static void
add_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    volatile VALUE	rstr = rb_str_new(str, len);

    rstr = oj_encode(rstr);
    pi->stack.head->val = rstr;
}

static void
add_num(ParseInfo pi, NumInfo ni) {
    if (ni->infinity || ni->nan) {
	oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "not a number or other value");
    }
    pi->stack.head->val = oj_num_as_value(ni);
}

static VALUE
start_hash(ParseInfo pi) {
    return rb_hash_new();
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
hash_set_cstr(ParseInfo pi, Val parent, const char *str, size_t len, const char *orig) {
    volatile VALUE	rstr = rb_str_new(str, len);

    rstr = oj_encode(rstr);
    rb_hash_aset(stack_peek(&pi->stack)->val, calc_hash_key(pi, parent), rstr);
}

static void
hash_set_num(struct _ParseInfo *pi, Val parent, NumInfo ni) {
    if (ni->infinity || ni->nan) {
	oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "not a number or other value");
    }
    rb_hash_aset(stack_peek(&pi->stack)->val, calc_hash_key(pi, parent), oj_num_as_value(ni));
}

static void
hash_set_value(ParseInfo pi, Val parent, VALUE value) {
    rb_hash_aset(stack_peek(&pi->stack)->val, calc_hash_key(pi, parent), value);
}

static VALUE
start_array(ParseInfo pi) {
    return rb_ary_new();
}

static void
array_append_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    volatile VALUE	rstr = rb_str_new(str, len);

    rstr = oj_encode(rstr);
    rb_ary_push(stack_peek(&pi->stack)->val, rstr);
}

static void
array_append_num(ParseInfo pi, NumInfo ni) {
    if (ni->infinity || ni->nan) {
	oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "not a number or other value");
    }
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
    pi->hash_key = noop_hash_key;
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
    pi.handler = Qnil;
    pi.err_class = Qnil;
    oj_set_strict_callbacks(&pi);

    if (T_STRING == rb_type(*argv)) {
	return oj_pi_parse(argc, argv, &pi, 0, 0, 1);
    } else {
	return oj_pi_sparse(argc, argv, &pi, 0);
    }
}

VALUE
oj_strict_parse_cstr(int argc, VALUE *argv, char *json, size_t len) {
    struct _ParseInfo	pi;

    pi.options = oj_default_options;
    pi.handler = Qnil;
    pi.err_class = Qnil;
    oj_set_strict_callbacks(&pi);

    return oj_pi_parse(argc, argv, &pi, json, len, 1);
}
