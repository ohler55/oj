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
    struct _num num;
    struct _buf key;
    struct _buf buf;

    void (*add_null)(struct _ojParser *p, const char *key);
    void (*add_true)(struct _ojParser *p, const char *key);
    void (*add_false)(struct _ojParser *p, const char *key);
    void (*add_int)(struct _ojParser *p, const char *key, int64_t num);
    void (*add_float)(struct _ojParser *p, const char *key, double num);
    void (*add_big)(struct _ojParser *p, const char *key, const char *str, size_t len);
    void (*add_str)(struct _ojParser *p, const char *key, const char *str, size_t len);
    void (*open_array)(struct _ojParser *p, const char *key);
    void (*close_array)(struct _ojParser *p);
    void (*open_object)(struct _ojParser *p, const char *key);
    void (*close_object)(struct _ojParser *p);
    VALUE (*option)(struct _ojParser *p, const char *key, VALUE value);
    VALUE (*result)(struct _ojParser *p);
    void (*free)(struct _ojParser *p);
    void (*mark)(struct _ojParser *p);

    void *ctx;

    char     token[8];
    long     line;
    long     col;
    int      ri;
    uint32_t ucode;
    uint32_t	cache_str;
    ojType      type;  // valType
    bool	cache_keys;
} * ojParser;

#endif /* OJ_PARSER_H */
