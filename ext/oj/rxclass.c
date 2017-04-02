/* rxclass.c
 * Copyright (c) 2017, Peter Ohler
 * All rights reserved.
 */

#include <sys/types.h>
#include <stdlib.h>
#include <regex.h>

#include "rxclass.h"

typedef struct _RxC {
    struct _RxC	*next;
    regex_t	rx;
    void	*clas;
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
	regfree(&rxc->rx);
	free(rxc);
    }
}

int
oj_rxclass_append(RxClass rc, const char *expr, bool ext, bool icase, bool newline) {
    // Attempt to compile the expression. If it fails populate the err_msg.
    
    // TBD
    return 0;
}

void*
oj_rxclass_match(RxClass rc, const char *str, int len) {
    RxC	rxc;
    
    for (rxc = rc->head; NULL != rxc; rxc = rxc->next) {
	// TBD regexec
    }
    return NULL; // Really a VALUE, Qfalse but certainly not a class
}

