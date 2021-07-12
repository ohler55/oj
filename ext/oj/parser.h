// Copyright (c) 2021 Peter Ohler, All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef OJ_PARSER_H
#define OJ_PARSER_H

#include "buf.h"
#include "ruby.h"

typedef enum {
    OJ_NONE    = '\0',
    OJ_NULL    = 'n',
    OJ_TRUE    = 't',
    OJ_FALSE   = 'f',
    OJ_INT     = 'i',
    OJ_DECIMAL = 'd',
    OJ_BIG     = 'b',  // indicates parser buf is used
    OJ_STRING  = 's',
    OJ_OBJECT  = 'o',
    OJ_ARRAY   = 'a',
} ojType;

typedef struct _num {
    long double dub;
    int64_t     fixnum;  // holds all digits
    uint32_t    len;
    int16_t     div;  // 10^div
    int16_t     exp;
    uint8_t     shift;  // shift of fixnum to get decimal
    bool        neg;
    bool        exp_neg;
    // for numbers as strings, reuse buf
} * Num;

typedef struct _ojParser {
    const char *map;
    const char *next_map;
    int         depth;
    char        stack[1024];

    const char *end;  // TBD ???

    // value data
    ojType      type;  // valType
    struct _num num;
    struct _buf key;
    struct _buf buf;

    void (*add_null)(void *ctx, const char *key);
    void (*add_true)(void *ctx, const char *key);
    void (*add_false)(void *ctx, const char *key);
    void (*add_int)(void *ctx, const char *key, int64_t num);
    void (*add_float)(void *ctx, const char *key, double num);
    void (*add_big)(void *ctx, const char *key, const char *str, size_t len);
    void (*add_str)(void *ctx, const char *key, const char *str, size_t len);
    void (*open_array)(void *ctx, const char *key);
    void (*close_array)(void *ctx);
    void (*open_object)(void *ctx, const char *key);
    void (*close_object)(void *ctx);
    VALUE (*option)(void *ctx, const char *key, VALUE value);
    VALUE (*result)(void *ctx);

    void *ctx;

    char     token[8];
    long     line;
    long     col;
    int      ri;
    uint32_t ucode;
} * ojParser;

#endif /* OJ_PARSER_H */
