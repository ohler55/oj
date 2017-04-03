/* rxclass.c
 * Copyright (c) 2017, Peter Ohler
 * All rights reserved.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <errno.h>
#include <regex.h>
#include <string.h>
#include <stdio.h>

#include "rxclass.h"

typedef struct _RxC {
    struct _RxC	*next;
    VALUE	rrx;
    regex_t	rx;
    VALUE	clas;
    char	src[256];
} *RxC;

void
oj_rxclass_init(RxClass rc) {
    *rc->err = '\0';
    rc->head = NULL;
    rc->tail = NULL;
}

void
oj_rxclass_cleanup(RxClass rc) {
    RxC	rxc;

    while (NULL != (rxc = rc->head)) {
	rc->head = rc->head->next;
	if (Qnil == rxc->rrx) {
	    regfree(&rxc->rx);
	}
	xfree(rxc);
    }
}

void
oj_rxclass_rappend(RxClass rc, VALUE rx, VALUE clas) {
    RxC	rxc = ALLOC_N(struct _RxC, 1);

    memset(rxc, 0, sizeof(struct _RxC));
    rxc->rrx = rx;
    rxc->clas = clas;
    if (NULL == rc->tail) {
	rc->head = rxc;
    } else {
	rc->tail->next = rxc;
    }
    rc->tail = rxc;
}

// Attempt to compile the expression. If it fails populate the error code..
int
oj_rxclass_append(RxClass rc, const char *expr, VALUE clas) {
    // Use mallow and not ALLOC_N to avoid pulliing ruby.h which conflicts
    // with regex_t.
    RxC	rxc;
    int	err;
    int	flags = 0;
    
    if (sizeof(rxc->src) <= strlen(expr)) {
	snprintf(rc->err, sizeof(rc->err), "expressions must be less than %lu chracter", sizeof(rxc->src));
	return EINVAL;
    }
    rxc = ALLOC_N(struct _RxC, 1);
    rxc->next = 0;
    rxc->rrx = Qnil;
    rxc->clas = clas;
    if (0 != (err = regcomp(&rxc->rx, expr, flags))) {
	regerror(err, &rxc->rx, rc->err, sizeof(rc->err));
	free(rxc);
	return err;
    }
    if (NULL == rc->tail) {
	rc->head = rxc;
    } else {
	rc->tail->next = rxc;
    }
    rc->tail = rxc;

    return 0;
}

VALUE
oj_rxclass_match(RxClass rc, const char *str, int len) {
    RxC		rxc;
    char	buf[4096];
    
    for (rxc = rc->head; NULL != rxc; rxc = rxc->next) {
	if (Qnil != rxc->rrx) {
	    // Must use a valiabel for this to work.
	    volatile VALUE	rstr = rb_str_new(str, len);
	    
	    //if (Qtrue == rb_funcall(rxc->rrx, rb_intern("match?"), 1, rstr)) {
	    if (Qnil != rb_funcall(rxc->rrx, rb_intern("match"), 1, rstr)) {
		return rxc->clas;
	    }
	} else if (len < (int)sizeof(buf)) {
	    // string is not \0 terminated so copy and atempt a match
	    memcpy(buf, str, len);
	    buf[len] = '\0';
	    if (0 == regexec(&rxc->rx, buf, 0, NULL, 0)) { // match
		return rxc->clas;
	    }
	} else {
	    // TBD allocate a larger buffer and attempt
	}
    }
    return Qnil;
}

void
oj_rxclass_copy(RxClass src, RxClass dest) {
    dest->head = NULL;
    dest->tail = NULL;
    if (NULL != src->head) {
	RxC	rxc;

	for (rxc = src->head; NULL != rxc; rxc = rxc->next) {
	    if (Qnil != rxc->rrx) {
		oj_rxclass_rappend(dest, rxc->rrx, rxc->clas);
	    } else {
		oj_rxclass_append(dest, rxc->src, rxc->clas);
	    }
	}
    }
}
