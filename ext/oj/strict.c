// Copyright (c) 2012 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "encode.h"
#include "err.h"
#include "intern.h"
#include "oj.h"
#include "parse.h"
#include "trace.h"

VALUE oj_cstr_to_value(const char *str, size_t len, size_t cache_str) {
    volatile VALUE rstr = Qnil;

    if (len < cache_str) {
	rstr = oj_str_intern(str, len);
    } else {
        rstr = rb_str_new(str, len);
        rstr = oj_encode(rstr);
    }
    return rstr;
}

VALUE oj_calc_hash_key(ParseInfo pi, Val parent) {
    volatile VALUE rkey = parent->key_val;

    if (Qundef != rkey) {
        return rkey;
    }
    if (Yes != pi->options.cache_keys) {
        if (Yes == pi->options.sym_key) {
            rkey = ID2SYM(rb_intern3(parent->key, parent->klen, oj_utf8_encoding));
        } else {
            rkey = rb_str_new(parent->key, parent->klen);
            rkey = oj_encode(rkey);
	    OBJ_FREEZE(rkey); // frozen when used as a Hash key anyway
        }
        return rkey;
    }
    if (Yes == pi->options.sym_key) {
	rkey = oj_sym_intern(parent->key, parent->klen);
    } else {
	rkey = oj_str_intern(parent->key, parent->klen);
    }
    return rkey;
}

static void hash_end(ParseInfo pi) {
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_hash_end(pi, __FILE__, __LINE__);
    }
}

static void array_end(ParseInfo pi) {
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_array_end(pi, __FILE__, __LINE__);
    }
}

static VALUE noop_hash_key(ParseInfo pi, const char *key, size_t klen) {
    return Qundef;
}

static void add_value(ParseInfo pi, VALUE val) {
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("add_value", pi, __FILE__, __LINE__, val);
    }
    pi->stack.head->val = val;
}

static void add_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    volatile VALUE rstr = oj_cstr_to_value(str, len, (size_t)pi->options.cache_str);

    pi->stack.head->val = rstr;
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("add_string", pi, __FILE__, __LINE__, rstr);
    }
}

static void add_num(ParseInfo pi, NumInfo ni) {
    if (ni->infinity || ni->nan) {
        oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "not a number or other value");
    }
    pi->stack.head->val = oj_num_as_value(ni);
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("add_number", pi, __FILE__, __LINE__, pi->stack.head->val);
    }
}

static VALUE start_hash(ParseInfo pi) {
    if (Qnil != pi->options.hash_class) {
        return rb_class_new_instance(0, NULL, pi->options.hash_class);
    }
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_in("start_hash", pi, __FILE__, __LINE__);
    }
    return rb_hash_new();
}

static void hash_set_cstr(ParseInfo pi, Val parent, const char *str, size_t len, const char *orig) {
    volatile VALUE rstr = oj_cstr_to_value(str, len, (size_t)pi->options.cache_str);

    rb_hash_aset(stack_peek(&pi->stack)->val,
                 oj_calc_hash_key(pi, parent),
                 rstr);
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("set_string", pi, __FILE__, __LINE__, rstr);
    }
}

static void hash_set_num(ParseInfo pi, Val parent, NumInfo ni) {
    volatile VALUE v;

    if (ni->infinity || ni->nan) {
        oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "not a number or other value");
    }
    v = oj_num_as_value(ni);
    rb_hash_aset(stack_peek(&pi->stack)->val,
                 oj_calc_hash_key(pi, parent),
                 v);
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("set_number", pi, __FILE__, __LINE__, v);
    }
}

static void hash_set_value(ParseInfo pi, Val parent, VALUE value) {
    rb_hash_aset(stack_peek(&pi->stack)->val,
                 oj_calc_hash_key(pi, parent),
                 value);
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("set_value", pi, __FILE__, __LINE__, value);
    }
}

static VALUE start_array(ParseInfo pi) {
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_in("start_array", pi, __FILE__, __LINE__);
    }
    return rb_ary_new();
}

static void array_append_cstr(ParseInfo pi, const char *str, size_t len, const char *orig) {
    volatile VALUE rstr = oj_cstr_to_value(str, len, (size_t)pi->options.cache_str);

    rb_ary_push(stack_peek(&pi->stack)->val, rstr);
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("append_string", pi, __FILE__, __LINE__, rstr);
    }
}

static void array_append_num(ParseInfo pi, NumInfo ni) {
    volatile VALUE v;

    if (ni->infinity || ni->nan) {
        oj_set_error_at(pi, oj_parse_error_class, __FILE__, __LINE__, "not a number or other value");
    }
    v = oj_num_as_value(ni);
    rb_ary_push(stack_peek(&pi->stack)->val, v);
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("append_number", pi, __FILE__, __LINE__, v);
    }
}

static void array_append_value(ParseInfo pi, VALUE value) {
    rb_ary_push(stack_peek(&pi->stack)->val, value);
    if (RB_UNLIKELY(Yes == pi->options.trace)) {
        oj_trace_parse_call("append_value", pi, __FILE__, __LINE__, value);
    }
}

void oj_set_strict_callbacks(ParseInfo pi) {
    pi->start_hash         = start_hash;
    pi->end_hash           = hash_end;
    pi->hash_key           = noop_hash_key;
    pi->hash_set_cstr      = hash_set_cstr;
    pi->hash_set_num       = hash_set_num;
    pi->hash_set_value     = hash_set_value;
    pi->start_array        = start_array;
    pi->end_array          = array_end;
    pi->array_append_cstr  = array_append_cstr;
    pi->array_append_num   = array_append_num;
    pi->array_append_value = array_append_value;
    pi->add_cstr           = add_cstr;
    pi->add_num            = add_num;
    pi->add_value          = add_value;
    pi->expect_value       = 1;
}

VALUE
oj_strict_parse(int argc, VALUE *argv, VALUE self) {
    struct _parseInfo pi;

    parse_info_init(&pi);
    pi.options   = oj_default_options;
    pi.handler   = Qnil;
    pi.err_class = Qnil;
    oj_set_strict_callbacks(&pi);

    if (T_STRING == rb_type(*argv)) {
        return oj_pi_parse(argc, argv, &pi, 0, 0, true);
    } else {
        return oj_pi_sparse(argc, argv, &pi, 0);
    }
}

VALUE
oj_strict_parse_cstr(int argc, VALUE *argv, char *json, size_t len) {
    struct _parseInfo pi;

    parse_info_init(&pi);
    pi.options   = oj_default_options;
    pi.handler   = Qnil;
    pi.err_class = Qnil;
    oj_set_strict_callbacks(&pi);

    return oj_pi_parse(argc, argv, &pi, json, len, true);
}
