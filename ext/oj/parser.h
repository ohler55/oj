// Copyright (c) 2021 Peter Ohler, All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef OJ_PARSER_H
#define OJ_PARSER_H

#include <stdbool.h>
#include <ruby.h>

#include "buf.h"

#define TOP_FUN 0
#define ARRAY_FUN 1
#define OBJECT_FUN 2

typedef uint8_t byte;

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

struct _ojParser;

typedef struct _funcs {
    void (*add_null)(struct _ojParser *p);
    void (*add_true)(struct _ojParser *p);
    void (*add_false)(struct _ojParser *p);
    void (*add_int)(struct _ojParser *p);
    void (*add_float)(struct _ojParser *p);
    void (*add_big)(struct _ojParser *p);
    void (*add_str)(struct _ojParser *p);
    void (*open_array)(struct _ojParser *p);
    void (*close_array)(struct _ojParser *p);
    void (*open_object)(struct _ojParser *p);
    void (*close_object)(struct _ojParser *p);
} * Funcs;

typedef struct _ojParser {
    const char *  map;
    const char *  next_map;
    int           depth;
    unsigned char stack[1024];

    // value data
    struct _num num;
    struct _buf key;
    struct _buf buf;

    struct _funcs funcs[3]; // indexed by XXX_FUN defines

    void (*start)(struct _ojParser *p);
    VALUE (*option)(struct _ojParser *p, const char *key, VALUE value);
    VALUE (*result)(struct _ojParser *p);
    void (*free)(struct _ojParser *p);
    void (*mark)(struct _ojParser *p);

    void *ctx;
    VALUE reader;

    char     token[8];
    long     line;
    long     col;
    int      ri;
    uint32_t ucode;
    ojType   type;  // valType
    bool     just_one;
} * ojParser;

#endif /* OJ_PARSER_H */
