/* rxclass.h
 * Copyright (c) 2017, Peter Ohler
 * All rights reserved.
 */

#ifndef __OJ_RXCLASS_H__
#define __OJ_RXCLASS_H__

#include <stdbool.h>

struct _RxC;

typedef struct _RxClass {
    char	err[128];
    struct _RxC	*head;
    struct _RxC	*tail;
} *RxClass;

extern void	oj_rxclass_init(RxClass rc);
extern void	oj_rxclass_cleanup(RxClass rc);
extern int	oj_rxclass_append(RxClass rc, const char *expr, bool ext, bool icase, bool newline);
extern void*	oj_rxclass_match(RxClass rc, const char *str, int len);

#endif /* __OJ_RXCLASS_H__ */
