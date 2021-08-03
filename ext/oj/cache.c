// Copyright (c) 2011, 2021 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#include "cache.h"

#define REHASH_LIMIT 64
#define MIN_SHIFT 8

typedef struct _slot {
    struct _slot *next;
    uint32_t      hash;
    uint8_t       klen;
    char          key[CACHE_MAX_KEY];
    VALUE         val;
} * Slot;

typedef struct _cache {
    Slot * slots;
    size_t cnt;
    VALUE (*form)(const char *str, size_t len);
    uint32_t size;
    uint32_t mask;
    bool     reg;
} * Cache;

// almost the Murmur hash algorithm
#define M 0x5bd1e995
#define C1 0xCC9E2D51
#define C2 0x1B873593
#define N 0xE6546B64

#if 0
// For debugging only.
static void cache_print(Cache c) {
    for (uint32_t i = 0; i < c->size; i++) {
        printf("%4d:", i);
        for (Slot s = c->slots[i]; NULL != s; s = s->next) {
            char buf[40];
            strncpy(buf, s->key, s->klen);
            buf[s->klen] = '\0';
            printf(" %s", buf);
        }
        printf("\n");
    }
}
#endif

static uint32_t hash_calc(const uint8_t *key, size_t len) {
    const uint8_t *end     = key + len;
    const uint8_t *endless = key + (len & 0xFFFFFFFC);
    uint32_t       h       = (uint32_t)len;
    uint32_t       k;

    while (key < endless) {
        k = (uint32_t)*key++;
        k |= (uint32_t)*key++ << 8;
        k |= (uint32_t)*key++ << 16;
        k |= (uint32_t)*key++ << 24;

        k *= M;
        k ^= k >> 24;
        h *= M;
        h ^= k * M;
    }
    if (1 < end - key) {
        uint16_t k16 = (uint16_t)*key++;

        k16 |= (uint16_t)*key++ << 8;
        h ^= k16 << 8;
    }
    if (key < end) {
        h ^= *key;
    }
    h *= M;
    h ^= h >> 13;
    h *= M;
    h ^= h >> 15;

    return h;
}

Cache cache_create(size_t size, VALUE (*form)(const char *str, size_t len), bool reg) {
    Cache c     = ALLOC(struct _cache);
    int   shift = 0;

    for (; REHASH_LIMIT < size; size /= 2, shift++) {
    }
    if (shift < MIN_SHIFT) {
        shift = MIN_SHIFT;
    }
    c->size  = 1 << shift;
    c->mask  = c->size - 1;
    c->slots = ALLOC_N(Slot, c->size);
    memset(c->slots, 0, sizeof(Slot) * c->size);
    c->form = form;
    c->cnt  = 0;
    c->reg  = reg;

    return c;
}

static void rehash(Cache c) {
    uint32_t osize = c->size;

    c->size = osize * 4;
    c->mask = c->size - 1;
    REALLOC_N(c->slots, Slot, c->size);
    memset(c->slots + osize, 0, sizeof(Slot) * osize * 3);

    Slot *end = c->slots + osize;
    for (Slot *sp = c->slots; sp < end; sp++) {
        Slot s    = *sp;
        Slot next = NULL;
        *sp       = NULL;
        for (; NULL != s; s = next) {
            next = s->next;

            uint32_t h      = s->hash & c->mask;
            Slot *   bucket = c->slots + h;

            s->next = *bucket;
            *bucket = s;
        }
    }
}

void cache_free(Cache c) {
    for (uint32_t i = 0; i < c->size; i++) {
        Slot next;
        for (Slot s = c->slots[i]; NULL != s; s = next) {
            next = s->next;
            xfree(s);
        }
    }
    xfree(c);
}

VALUE
cache_intern(Cache c, const char *key, size_t len) {
    if (CACHE_MAX_KEY < len) {
        return c->form(key, len);
    }
    uint32_t h      = hash_calc((const uint8_t *)key, len);
    Slot *   bucket = c->slots + (h & c->mask);
    Slot     b;
    Slot     tail = NULL;

    for (b = *bucket; NULL != b; b = b->next) {
        if ((uint8_t)len == b->klen && 0 == strncmp(b->key, key, len)) {
            return b->val;
        }
        tail = b;
    }
    b       = ALLOC(struct _slot);
    b->hash = h;
    b->next = NULL;
    if (NULL == tail) {
        *bucket = b;
    } else {
        tail->next = b;
    }
    memcpy(b->key, key, len);
    b->klen = (uint8_t)len;
    b->val  = c->form(key, len);
    if (c->reg) {
        rb_gc_register_address(&b->val);
    }
    c->cnt++;
    if (REHASH_LIMIT < c->cnt / c->size) {
        rehash(c);
    }
    return b->val;
}
