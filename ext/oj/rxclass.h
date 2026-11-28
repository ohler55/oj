// Copyright (c) 2017 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef OJ_RXCLASS_H
#define OJ_RXCLASS_H

#include <stdbool.h>

#include "ruby.h"

struct _rxC;

typedef struct _rxClass {
    struct _rxC *head;
    struct _rxC *tail;
    char         err[128];
    // The chain is malloced, so this object keeps its classes and regexps from
    // being moved or collected, and frees the chain when it is collected. The GC
    // sees it through options on the stack or by marking the defaults. Writers and
    // encoders never match, so they let go of theirs.
    VALUE keep;
} *RxClass;

extern VALUE oj_rxclass_init(RxClass rc);
extern void  oj_rxclass_cleanup(RxClass rc);
extern int   oj_rxclass_append(RxClass rc, const char *expr, VALUE clas);
extern VALUE oj_rxclass_match(RxClass rc, const char *str, size_t len);
extern void  oj_rxclass_rappend(RxClass rc, VALUE rx, VALUE clas);
extern void  oj_rxclass_keep(RxClass rc, VALUE keep);
extern void  oj_rxclass_mark(RxClass rc);

#endif /* OJ_RXCLASS_H */
