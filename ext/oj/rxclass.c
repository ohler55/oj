// Copyright (c) 2017 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#if !IS_WINDOWS
#include <regex.h>
#endif

#include "mem.h"
#include "rxclass.h"

typedef struct _rxC {
    struct _rxC *next;
    VALUE        rrx;
#if !IS_WINDOWS
    regex_t rx;
#endif
    VALUE clas;
    char  src[256];
} *RxC;

// The GC can not update the VALUEs in the malloced chain, so the ones it holds
// are also kept in an array and marked here, which pins them.
static void keep_mark(void *ptr) {
    VALUE ary = (VALUE)ptr;
    long  i;

    rb_gc_mark(ary);
    for (i = 0; i < RARRAY_LEN(ary); i++) {
        rb_gc_mark(RARRAY_AREF(ary, i));
    }
}

static const rb_data_type_t keep_type = {
    "Oj/rxclass_keep",
    {
        keep_mark,
        NULL,
        NULL,
    },
    0,
    0,
};

static void keep_value(RxClass rc, VALUE v) {
    rb_ary_push((VALUE)RTYPEDDATA_DATA(rc->keep), v);
}

// Returns the object that keeps the classes and regexps of the chain.
VALUE oj_rxclass_init(RxClass rc) {
    VALUE ary = rb_obj_hide(rb_ary_new());

    *rc->err = '\0';
    rc->head = NULL;
    rc->tail = NULL;
    rc->keep = TypedData_Wrap_Struct(0, &keep_type, (void *)ary);
    RB_GC_GUARD(ary);

    return rc->keep;
}

void oj_rxclass_cleanup(RxClass rc) {
    RxC rxc;

    while (NULL != (rxc = rc->head)) {
        rc->head = rc->head->next;
#if !IS_WINDOWS
        if (Qnil == rxc->rrx) {
            regfree(&rxc->rx);
        }
#endif
        OJ_R_FREE(rxc);
    }
    rc->keep = Qnil;
}

void oj_rxclass_rappend(RxClass rc, VALUE rx, VALUE clas) {
    RxC rxc;

    keep_value(rc, rx);
    keep_value(rc, clas);
    rxc = OJ_R_ALLOC_N(struct _rxC, 1);
    memset(rxc, 0, sizeof(struct _rxC));
    rxc->rrx  = rx;
    rxc->clas = clas;
    if (NULL == rc->tail) {
        rc->head = rxc;
    } else {
        rc->tail->next = rxc;
    }
    rc->tail = rxc;
}

// Attempt to compile the expression. If it fails populate the error code..
int oj_rxclass_append(RxClass rc, const char *expr, VALUE clas) {
    RxC   rxc;
    VALUE rrx = Qnil;
#if !IS_WINDOWS
    int err;
    int flags = 0;
#endif
    if (sizeof(rxc->src) <= strlen(expr)) {
        snprintf(rc->err,
                 sizeof(rc->err),
                 "expressions must be less than %lu characters",
                 (unsigned long)sizeof(rxc->src));
        return EINVAL;
    }
#if IS_WINDOWS
    rrx = rb_funcall(rb_cRegexp, rb_intern("new"), 1, rb_str_new2(expr));
    keep_value(rc, rrx);
#endif
    keep_value(rc, clas);
    rxc       = OJ_R_ALLOC_N(struct _rxC, 1);
    rxc->next = 0;
    rxc->clas = clas;
    rxc->rrx  = rrx;

#if !IS_WINDOWS
    if (0 != (err = regcomp(&rxc->rx, expr, flags))) {
        regerror(err, &rxc->rx, rc->err, sizeof(rc->err));
        OJ_FREE(rxc);
        // No node holds the class pushed above, so drop it.
        rb_ary_pop((VALUE)RTYPEDDATA_DATA(rc->keep));
        return err;
    }
#endif
    if (NULL == rc->tail) {
        rc->head = rxc;
    } else {
        rc->tail->next = rxc;
    }
    rc->tail = rxc;

    return 0;
}

VALUE
oj_rxclass_match(RxClass rc, const char *str, size_t len) {
    RxC  rxc;
    char buf[4096];

    for (rxc = rc->head; NULL != rxc; rxc = rxc->next) {
        if (Qnil != rxc->rrx) {
            // Must use a valiabel for this to work.
            volatile VALUE rstr = rb_str_new(str, len);

            // if (Qtrue == rb_funcall(rxc->rrx, rb_intern("match?"), 1, rstr)) {
            if (Qnil != rb_funcall(rxc->rrx, rb_intern("match"), 1, rstr)) {
                return rxc->clas;
            }
        } else if (len < (int)sizeof(buf)) {
#if !IS_WINDOWS
            // string is not \0 terminated so copy and attempt a match
            memcpy(buf, str, len);
            buf[len] = '\0';
            if (0 == regexec(&rxc->rx, buf, 0, NULL, 0)) {  // match
                return rxc->clas;
            }
#endif
        } else {
            // TBD allocate a larger buffer and attempt
        }
    }
    return Qnil;
}

// Add the Ruby objects in the chain to an array. The regexps and the classes
// can be referenced from nowhere else, as with a default match_string option,
// and are collected unless something holds onto them.
void oj_rxclass_keep(RxClass rc, VALUE keep) {
    RxC rxc;

    for (rxc = rc->head; NULL != rxc; rxc = rxc->next) {
        if (Qnil != rxc->rrx) {
            rb_ary_push(keep, rxc->rrx);
        }
        if (Qnil != rxc->clas) {
            rb_ary_push(keep, rxc->clas);
        }
    }
}

void oj_rxclass_mark(RxClass rc) {
    RxC rxc;

    rb_gc_mark(rc->keep);
    for (rxc = rc->head; NULL != rxc; rxc = rxc->next) {
        rb_gc_mark(rxc->rrx);
        rb_gc_mark(rxc->clas);
    }
}
