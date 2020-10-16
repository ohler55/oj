// Copyright (c) 2011 Peter Ohler. All rights reserved.

#ifndef OJ_ERR_H
#define OJ_ERR_H

#include "ruby.h"
// Needed to silence 2.4.0 warnings.
#ifndef NORETURN
# define NORETURN(x) x
#endif

#define set_error(err, eclas, msg, json, current) _oj_err_set_with_location(err, eclas, msg, json, current, FILE, LINE)

typedef struct _err {
    VALUE	clas;
    char	msg[128];
} *Err;

extern VALUE	oj_parse_error_class;

extern void	oj_err_set(Err e, VALUE clas, const char *format, ...);
extern void	_oj_err_set_with_location(Err err, VALUE eclas, const char *msg, const char *json, const char *current, const char* file, int line);

NORETURN(extern void	oj_err_raise(Err e));

#define raise_error(msg, json, current) _oj_raise_error(msg, json, current, __FILE__, __LINE__)

NORETURN(extern void	_oj_raise_error(const char *msg, const char *json, const char *current, const char* file, int line));


inline static void
err_init(Err e) {
    e->clas = Qnil;
    *e->msg = '\0';
}

inline static int
err_has(Err e) {
    return (Qnil != e->clas);
}

#endif /* OJ_ERR_H */
