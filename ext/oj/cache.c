/* cache.c
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

#include "cache.h"

struct _Cache {
    char                *key; // only set if the node has a value, and it is not an exact match
    VALUE               value;
    struct _Cache       *slots[16];
};

static void     slot_print(Cache cache, unsigned int depth);

void
oj_cache_new(Cache *cache) {
    if (0 == (*cache = ALLOC(struct _Cache))) {
        rb_raise(rb_eNoMemError, "not enough memory\n");
    }
    (*cache)->key = 0;
    (*cache)->value = Qundef;
    memset((*cache)->slots, 0, sizeof((*cache)->slots));
}

VALUE
oj_cache_get(Cache cache, const char *key, VALUE **slot) {
    unsigned char       *k = (unsigned char*)key;
    Cache               *cp;

    for (; '\0' != *k; k++) {
        cp = cache->slots + (unsigned int)(*k >> 4); // upper 4 bits
        if (0 == *cp) {
            oj_cache_new(cp);
        }
        cache = *cp;
        cp = cache->slots + (unsigned int)(*k & 0x0F); // lower 4 bits
        if (0 == *cp) {
            oj_cache_new(cp);
            cache = *cp;
            cache->key = ('\0' == *(k + 1)) ? 0 : strdup(key);
            break;
        } else {
            cache = *cp;
            if (Qundef != cache->value && 0 != cache->key) {
                unsigned char   *ck = (unsigned char*)(cache->key + (unsigned int)(k - (unsigned char*)key + 1));

                if (0 == strcmp((char*)ck, (char*)(k + 1))) {
                    break;
                } else {
                    Cache     *cp2 = cp;

                    // if value was set along with the key then there are no slots filled yet
                    cp2 = (*cp2)->slots + (*ck >> 4);
                    oj_cache_new(cp2);
                    cp2 = (*cp2)->slots + (*ck & 0x0F);
                    oj_cache_new(cp2);
                    if ('\0' == *(ck + 1)) {
                        xfree(cache->key);
                    } else {
                        (*cp2)->key = cache->key;
                    }
                    (*cp2)->value = cache->value;
                    cache->key = 0;
                    cache->value = Qundef;
                }
            }
        }
    }
    *slot = &cache->value;

    return cache->value;
}

void
oj_cache_print(Cache cache) {
    //printf("-------------------------------------------\n");
    slot_print(cache, 0);
}

static void
slot_print(Cache c, unsigned int depth) {
    char                indent[256];
    Cache               *cp;
    unsigned int        i;

    if (sizeof(indent) - 1 < depth) {
        depth = ((int)sizeof(indent) - 1);
    }
    memset(indent, ' ', depth);
    indent[depth] = '\0';
    for (i = 0, cp = c->slots; i < 16; i++, cp++) {
        if (0 == *cp) {
            //printf("%s%02u:\n", indent, i);
        } else {
            if (0 == (*cp)->key && Qundef == (*cp)->value) {
                printf("%s%02u:\n", indent, i);
            } else {
                const char      *key = (0 == (*cp)->key) ? "*" : (*cp)->key;
                const char      *vs;
                const char      *clas;

                if (Qundef == (*cp)->value) {
                    vs = "undefined";
                    clas = "";
                } else {
                    VALUE       rs = rb_funcall2((*cp)->value, rb_intern("to_s"), 0, 0);

                    vs = StringValuePtr(rs);
                    clas = rb_class2name(rb_obj_class((*cp)->value));
                }
                printf("%s%02u: %s = %s (%s)\n", indent, i, key, vs, clas);
            }
            slot_print(*cp, depth + 2);
        }
    }
}
