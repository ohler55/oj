// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "parser.h"

static void
add_null(void *ctx, const char *key) {
}

static void
add_true(void *ctx, const char *key) {
}

static void
add_false(void *ctx, const char *key) {
}

static void
add_int(void *ctx, const char *key, int64_t num) {
}

static void
add_float(void *ctx, const char *key, double num) {
}

static void
add_big(void *ctx, const char *key, const char *str, size_t len) {
}

static void
add_str(void *ctx, const char *key, const char *str, size_t len) {
}

static void
open_array(void *ctx, const char *key) {
}

static void
close_array(void *ctx) {
}

static void
open_object(void *ctx, const char *key) {
}

static void
close_object(void *ctx) {
}


static VALUE
option(void *ctx, const char *key, VALUE value) {
    rb_raise(rb_eArgError, "%s is not an option for the validate delegate", key);
    return Qnil;
}

static VALUE
result(void *ctx) {
    return Qnil;
}

void oj_set_parser_validator(ojParser p) {
    p->ctx = NULL;
    p->add_null = add_null;
    p->add_true = add_true;
    p->add_false = add_false;
    p->add_int = add_int;
    p->add_float = add_float;
    p->add_big = add_big;
    p->add_str = add_str;
    p->open_array = open_array;
    p->close_array = close_array;
    p->open_object = open_object;
    p->close_object = close_object;
    p->option = option;
    p->result = result;
}
