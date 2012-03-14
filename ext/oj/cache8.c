/* cache8.c
 * Copyright (c) 2011, Peter Ohler
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  - Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 *  - Neither the name of Peter Ohler nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "cache8.h"

#define BITS            4
#define MASK            0x000000000000000F
#define SLOT_CNT        16
#define DEPTH           16

struct _Cache8 {
    union {
	struct _Cache8	*slots[SLOT_CNT];
	slot_t		values[SLOT_CNT];
    };
};

static void	cache8_delete(Cache8 cache, int depth);
static void	slot_print(Cache8 cache, VALUE key, unsigned int depth);

void
oj_cache8_new(Cache8 *cache) {
    Cache8     *cp;
    int		i;
    
    if (0 == (*cache = ALLOC(struct _Cache8))) {
	rb_raise(rb_eNoMemError, "not enough memory\n");
    }
    for (i = SLOT_CNT, cp = (*cache)->slots; 0 < i; i--, cp++) {
	*cp = 0;
    }
}

void
oj_cache8_delete(Cache8 cache) {
    cache8_delete(cache, 0);
}

static void
cache8_delete(Cache8 cache, int depth) {
    Cache8		*cp;
    unsigned int	i;

    for (i = 0, cp = cache->slots; i < SLOT_CNT; i++, cp++) {
	if (0 != *cp) {
	    if (DEPTH - 1 != depth) {
		cache8_delete(*cp, depth + 1);
	    }
	}
    }
    xfree(cache);
}

slot_t
oj_cache8_get(Cache8 cache, VALUE key, slot_t **slot) {
    Cache8	*cp;
    int		i;
    VALUE	k;
    
    for (i = 64 - BITS; 0 < i; i -= BITS) {
	k = (key >> i) & MASK;
	cp = cache->slots + k;
	if (0 == *cp) {
	    oj_cache8_new(cp);
	}
	cache = *cp;
    }
    *slot = cache->values + (key & MASK);

    return **slot;
}

void
oj_cache8_print(Cache8 cache) {
    //printf("-------------------------------------------\n");
    slot_print(cache, 0, 0);
}

static void
slot_print(Cache8 c, VALUE key, unsigned int depth) {
    Cache8		*cp;
    unsigned int	i;
    unsigned long	k;

    for (i = 0, cp = c->slots; i < SLOT_CNT; i++, cp++) {
	if (0 != *cp) {
	    k = (key << BITS) | i;
	    //printf("*** key: 0x%016lx	 depth: %u  i: %u\n", k, depth, i);
	    if (DEPTH - 1 == depth) {
		printf("0x%016lx: %4lu\n", k, (unsigned long)*cp);
	    } else {
		slot_print(*cp, k, depth + 1);
	    }
	}
    }
}
