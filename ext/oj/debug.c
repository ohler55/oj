// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "parser.h"

static void
add_null(struct _ojParser *p, const char *key) {
    printf("*** add_null '%s'\n", key);
}

static void
add_true(struct _ojParser *p, const char *key) {
    printf("*** add_true '%s'\n", key);
}

static void
add_false(struct _ojParser *p, const char *key) {
    printf("*** add_false '%s'\n", key);
}

static void
add_int(struct _ojParser *p, const char *key, int64_t num) {
    printf("*** add_int '%s' %lld\n", key, (long long)num);
}

static void
add_float(struct _ojParser *p, const char *key, double num) {
    printf("*** add_float '%s' %f\n", key, num);
}

static void
add_big(struct _ojParser *p, const char *key, const char *str, size_t len) {
    char	buf[256];

    if (sizeof(buf) <= len) {
	len = sizeof(buf) - 1;
    }
    strncpy(buf, str, len);
    buf[len] = '\0';
    printf("*** add_big '%s' '%s'\n", key, buf);
}

static void
add_str(struct _ojParser *p, const char *key, const char *str, size_t len) {
    char	buf[256];

    if (sizeof(buf) <= len) {
	len = sizeof(buf) - 1;
    }
    strncpy(buf, str, len);
    buf[len] = '\0';
    printf("*** add_str '%s' '%s'\n", key, buf);
}

static void
open_array(struct _ojParser *p, const char *key) {
    printf("*** open_array '%s'\n", key);
}

static void
close_array(struct _ojParser *p) {
    printf("*** close_array\n");
}

static void
open_object(struct _ojParser *p, const char *key) {
    printf("*** open_object '%s'\n", key);
}

static void
close_object(struct _ojParser *p) {
    printf("*** close_object\n");
}

static VALUE
option(ojParser p, const char *key, VALUE value) {
    rb_raise(rb_eArgError, "%s is not an option for the debug delegate", key);
    return Qnil;
}

static VALUE
result(struct _ojParser *p) {
    return Qnil;
}

static void
dfree(struct _ojParser *p) {
}

void oj_set_parser_debug(ojParser p) {
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
    p->free = dfree;
}
