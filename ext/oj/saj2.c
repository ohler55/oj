// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "encode.h"
#include "hash.h"
#include "oj.h"
#include "parser.h"

static void add_null(struct _ojParser *p, const char *key) {
}

static void add_true(struct _ojParser *p, const char *key) {
}

static void add_false(struct _ojParser *p, const char *key) {
}

static void add_int(struct _ojParser *p, const char *key, int64_t num) {
}

static void add_float(struct _ojParser *p, const char *key, double num) {
}

static void add_big(struct _ojParser *p, const char *key, const char *str, size_t len) {
}

static void add_str(struct _ojParser *p, const char *key, const char *str, size_t len) {
}

static void open_array(struct _ojParser *p, const char *key) {
}

static void close_array(struct _ojParser *p) {
}

static void open_object(struct _ojParser *p, const char *key) {
}

static void close_object(struct _ojParser *p) {
}

static VALUE get_key(ojParser p, const char *key) {
    if (NULL == key) {
        return Qnil;
    }
    volatile VALUE rkey;

    if (p->cache_keys) {
        VALUE *slot;
        size_t len = strlen(key);

        if (Qnil == (rkey = oj_str_hash_get(key, len, &slot))) {
            rkey  = oj_encode(rb_str_new(key, len));
            *slot = rkey;
            rb_gc_register_address(slot);
        }
    } else {
        rkey = oj_encode(rb_str_new2(key));
    }
    return rkey;
}

static void handle_add_null(struct _ojParser *p, const char *key) {
    rb_funcall((VALUE)p->ctx, oj_add_value_id, 2, Qnil, get_key(p, key));
}

static void handle_add_true(struct _ojParser *p, const char *key) {
    rb_funcall((VALUE)p->ctx, oj_add_value_id, 2, Qtrue, get_key(p, key));
}

static void handle_add_false(struct _ojParser *p, const char *key) {
    rb_funcall((VALUE)p->ctx, oj_add_value_id, 2, Qfalse, get_key(p, key));
}

static void handle_add_int(struct _ojParser *p, const char *key, int64_t num) {
    rb_funcall((VALUE)p->ctx, oj_add_value_id, 2, LONG2NUM(num), get_key(p, key));
}

static void handle_add_float(struct _ojParser *p, const char *key, double num) {
    rb_funcall((VALUE)p->ctx, oj_add_value_id, 2, rb_float_new(num), get_key(p, key));
}

static void handle_add_big(struct _ojParser *p, const char *key, const char *str, size_t len) {
    rb_funcall((VALUE)p->ctx,
               oj_add_value_id,
               2,
               rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(str, len)),
               get_key(p, key));
}

static void handle_add_str(struct _ojParser *p, const char *key, const char *str, size_t len) {
    volatile VALUE rstr;

    if (p->cache_str <= len) {
        VALUE *slot;

        if (Qnil == (rstr = oj_str_hash_get(str, len, &slot))) {
            rstr  = oj_encode(rb_str_new(str, len));
            *slot = rstr;
            rb_gc_register_address(slot);
        }
    } else {
        rstr = oj_encode(rb_str_new(str, len));
    }
    rb_funcall((VALUE)p->ctx, oj_add_value_id, 2, rstr, get_key(p, key));
}

static void handle_open_array(struct _ojParser *p, const char *key) {
    rb_funcall((VALUE)p->ctx, oj_array_start_id, 1, get_key(p, key));
}

static void handle_close_array(struct _ojParser *p) {
    rb_funcall((VALUE)p->ctx, oj_array_end_id, 1, Qnil);
}

static void handle_open_object(struct _ojParser *p, const char *key) {
    rb_funcall((VALUE)p->ctx, oj_hash_start_id, 1, get_key(p, key));
}

static void handle_close_object(struct _ojParser *p) {
    rb_funcall((VALUE)p->ctx, oj_hash_end_id, 1, Qnil);
}

static VALUE option(ojParser p, const char *key, VALUE value) {
    if (0 == strcmp(key, "handler")) {
        return (VALUE)p->ctx;
    }
    if (0 == strcmp(key, "handler=")) {
        p->ctx         = (void *)value;
        p->open_object = (rb_respond_to(value, oj_hash_start_id)) ? handle_open_object
                                                                  : open_object;
        p->close_object = (rb_respond_to(value, oj_hash_end_id)) ? handle_close_object
                                                                 : close_object;
        p->open_array  = (rb_respond_to(value, oj_array_start_id)) ? handle_open_array : open_array;
        p->close_array = (rb_respond_to(value, oj_array_end_id)) ? handle_close_array : close_array;
        if (rb_respond_to(value, oj_add_value_id)) {
            p->add_null  = handle_add_null;
            p->add_true  = handle_add_true;
            p->add_false = handle_add_false;
            p->add_int   = handle_add_int;
            p->add_float = handle_add_float;
            p->add_big   = handle_add_big;
            p->add_str   = handle_add_str;
        } else {
            p->add_null  = add_null;
            p->add_true  = add_true;
            p->add_false = add_false;
            p->add_int   = add_int;
            p->add_float = add_float;
            p->add_big   = add_big;
            p->add_str   = add_str;
        }
        return Qnil;
    }
    rb_raise(rb_eArgError, "%s is not an option for the validate delegate", key);
    return Qnil;
}

static VALUE result(struct _ojParser *p) {
    return Qnil;
}

static void dfree(struct _ojParser *p) {
}

void oj_set_parser_saj(ojParser p) {
    p->ctx          = (void *)Qnil;
    p->add_null     = add_null;
    p->add_true     = add_true;
    p->add_false    = add_false;
    p->add_int      = add_int;
    p->add_float    = add_float;
    p->add_big      = add_big;
    p->add_str      = add_str;
    p->open_array   = open_array;
    p->close_array  = close_array;
    p->open_object  = open_object;
    p->close_object = close_object;
    p->option       = option;
    p->result       = result;
    p->free         = dfree;
}
