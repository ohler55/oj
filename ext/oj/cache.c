// Copyright (c) 2011, 2021 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#if HAVE_PTHREAD_MUTEX_INIT
#include <pthread.h>
#endif
#include <stdlib.h>

#include "cache.h"
#include "mem.h"

// The stdlib calloc, realloc, and free are used instead of the Ruby ALLOC,
// ALLOC_N, REALLOC, and xfree since the later could trigger a GC which will
// either corrupt memory or if the mark function locks will deadlock.

#define REHASH_LIMIT 4
#define MIN_SHIFT 8
#define REUSE_MAX 8192

#if HAVE_PTHREAD_MUTEX_INIT
#define CACHE_LOCK(c) pthread_mutex_lock(&((c)->mutex))
#define CACHE_UNLOCK(c) pthread_mutex_unlock(&((c)->mutex))
#else
#define CACHE_LOCK(c) rb_mutex_lock((c)->mutex)
#define CACHE_UNLOCK(c) rb_mutex_unlock((c)->mutex)
#endif

#define ROTL(x, b) (uint64_t)(((x) << (b)) | ((x) >> (64 - (b))))
#define SIPROUND           \
    do {                   \
        v0 += v1;          \
        v1 = ROTL(v1, 13); \
        v1 ^= v0;          \
        v0 = ROTL(v0, 32); \
        v2 += v3;          \
        v3 = ROTL(v3, 16); \
        v3 ^= v2;          \
        v0 += v3;          \
        v3 = ROTL(v3, 21); \
        v3 ^= v0;          \
        v2 += v1;          \
        v1 = ROTL(v1, 17); \
        v1 ^= v2;          \
        v2 = ROTL(v2, 32); \
    } while (0)

static uint64_t hash_key[2];

void oj_hash_seed_init(void) {
    static bool seeded = false;
    VALUE       seed;
    VALUE       mask;

    if (seeded) {
        return;
    }
    seed        = rb_funcall(rb_cRandom, rb_intern("new_seed"), 0);
    mask        = ULL2NUM(UINT64_MAX);
    hash_key[0] = NUM2ULL(rb_funcall(seed, rb_intern("&"), 1, mask)) ^ (uint64_t)rb_memhash("oj0", 3);
    hash_key[1] = NUM2ULL(rb_funcall(rb_funcall(seed, rb_intern(">>"), 1, INT2FIX(64)), rb_intern("&"), 1, mask)) ^
                  (uint64_t)rb_memhash("oj1", 3);
    seeded      = true;
}

typedef struct _slot {
    struct _slot     *next;
    VALUE             val;
    uint64_t          hash;
    volatile uint32_t use_cnt;
    uint8_t           klen;
    char              key[CACHE_MAX_KEY];
} *Slot;

struct _cache {
    volatile Slot  *slots;
    volatile size_t cnt;
    VALUE (*form)(const char *str, size_t len);
    uint64_t size;
    uint64_t mask;
    VALUE (*intern)(struct _cache *c, const char *key, size_t len);
    volatile Slot reuse;
    size_t        rcnt;
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_t mutex;
#else
    VALUE mutex;
#endif
    uint8_t xrate;
    bool    mark;
};

void cache_set_form(Cache c, VALUE (*form)(const char *str, size_t len)) {
    c->form = form;
}

uint64_t oj_hash_calc(const uint8_t *key, size_t len) {
    const uint8_t *end = key + (len & ~(size_t)7);
    uint64_t       v0  = 0x736f6d6570736575ULL ^ hash_key[0];
    uint64_t       v1  = 0x646f72616e646f6dULL ^ hash_key[1];
    uint64_t       v2  = 0x6c7967656e657261ULL ^ hash_key[0];
    uint64_t       v3  = 0x7465646279746573ULL ^ hash_key[1];
    uint64_t       b   = (uint64_t)len << 56;
    uint64_t       m;
    size_t         i;

    for (; key < end; key += 8) {
        m = 0;
        for (i = 0; i < 8; i++) {
            m |= (uint64_t)key[i] << (8 * i);
        }
        v3 ^= m;
        SIPROUND;
        v0 ^= m;
    }
    for (i = 0; i < (len & 7); i++) {
        b |= (uint64_t)key[i] << (8 * i);
    }
    v3 ^= b;
    SIPROUND;
    v0 ^= b;
    v2 ^= 0xff;
    SIPROUND;
    SIPROUND;
    SIPROUND;

    return v0 ^ v1 ^ v2 ^ v3;
}

static void rehash(Cache c) {
    uint64_t osize;
    Slot    *end;
    Slot    *sp;

    osize    = c->size;
    c->size  = osize * 4;
    c->mask  = c->size - 1;
    c->slots = OJ_REALLOC((void *)c->slots, sizeof(Slot) * c->size);
    memset((Slot *)c->slots + osize, 0, sizeof(Slot) * osize * 3);
    end = (Slot *)c->slots + osize;
    for (sp = (Slot *)c->slots; sp < end; sp++) {
        Slot s    = *sp;
        Slot next = NULL;

        *sp = NULL;
        for (; NULL != s; s = next) {
            uint64_t h      = s->hash & c->mask;
            Slot    *bucket = (Slot *)c->slots + h;

            next    = s->next;
            s->next = *bucket;
            *bucket = s;
        }
    }
}

static VALUE lockless_intern(Cache c, const char *key, size_t len) {
    uint64_t       h      = oj_hash_calc((const uint8_t *)key, len);
    Slot          *bucket = (Slot *)c->slots + (h & c->mask);
    Slot           b;
    volatile VALUE rkey;

    while (REUSE_MAX < c->rcnt) {
        if (NULL != (b = c->reuse)) {
            c->reuse = b->next;
            OJ_FREE(b);
            c->rcnt--;
        } else {
            // An accounting error occured somewhere so correct it.
            c->rcnt = 0;
        }
    }
    for (b = *bucket; NULL != b; b = b->next) {
        if ((uint8_t)len == b->klen && 0 == memcmp(b->key, key, len)) {
            b->use_cnt += 16;
            return b->val;
        }
    }
    rkey = c->form(key, len);
    if (NULL == (b = c->reuse)) {
        b = OJ_CALLOC(1, sizeof(struct _slot));
    } else {
        c->reuse = b->next;
        c->rcnt--;
    }
    b->hash = h;
    memcpy(b->key, key, len);
    b->klen     = (uint8_t)len;
    b->key[len] = '\0';
    b->val      = rkey;
    b->use_cnt  = 4;
    b->next     = *bucket;
    *bucket     = b;
    c->cnt++;  // Don't worry about wrapping. Worse case is the entry is removed and recreated.
    if (REHASH_LIMIT < c->cnt / c->size) {
        rehash(c);
    }
    return rkey;
}

static VALUE locking_intern(Cache c, const char *key, size_t len) {
    uint64_t       h;
    Slot          *bucket;
    Slot           b;
    uint64_t       old_size;
    volatile VALUE rkey;

    CACHE_LOCK(c);
    while (REUSE_MAX < c->rcnt) {
        if (NULL != (b = c->reuse)) {
            c->reuse = b->next;
            OJ_FREE(b);
            c->rcnt--;
        } else {
            // An accounting error occured somewhere so correct it.
            c->rcnt = 0;
        }
    }
    h      = oj_hash_calc((const uint8_t *)key, len);
    bucket = (Slot *)c->slots + (h & c->mask);
    for (b = *bucket; NULL != b; b = b->next) {
        if ((uint8_t)len == b->klen && 0 == memcmp(b->key, key, len)) {
            b->use_cnt += 4;
            CACHE_UNLOCK(c);

            return b->val;
        }
    }
    old_size = c->size;
    // The creation of a new value may trigger a GC which be a problem if the
    // cache is locked so make sure it is unlocked for the key value creation.
    if (NULL != (b = c->reuse)) {
        c->reuse = b->next;
        c->rcnt--;
    }
    CACHE_UNLOCK(c);
    if (NULL == b) {
        b = OJ_CALLOC(1, sizeof(struct _slot));
    }
    rkey    = c->form(key, len);
    b->hash = h;
    memcpy(b->key, key, len);
    b->klen     = (uint8_t)len;
    b->key[len] = '\0';
    b->val      = rkey;
    b->use_cnt  = 16;

    // Lock again to add the new entry.
    CACHE_LOCK(c);
    if (old_size != c->size) {
        h      = oj_hash_calc((const uint8_t *)key, len);
        bucket = (Slot *)c->slots + (h & c->mask);
    }
    b->next = *bucket;
    *bucket = b;
    c->cnt++;  // Don't worry about wrapping. Worse case is the entry is removed and recreated.
    if (REHASH_LIMIT < c->cnt / c->size) {
        rehash(c);
    }
    CACHE_UNLOCK(c);

    return rkey;
}

Cache cache_create(size_t size, VALUE (*form)(const char *str, size_t len), bool mark, bool locking) {
    Cache c     = OJ_CALLOC(1, sizeof(struct _cache));
    int   shift = 0;

    for (; REHASH_LIMIT < size; size /= 2, shift++) {
    }
    if (shift < MIN_SHIFT) {
        shift = MIN_SHIFT;
    }
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_init(&c->mutex, NULL);
#else
    c->mutex = rb_mutex_new();
#endif
    c->size  = 1 << shift;
    c->mask  = c->size - 1;
    c->slots = OJ_CALLOC(c->size, sizeof(Slot));
    c->form  = form;
    c->xrate = 1;  // low
    c->mark  = mark;
    if (locking) {
        c->intern = locking_intern;
    } else {
        c->intern = lockless_intern;
    }
    return c;
}

void cache_set_expunge_rate(Cache c, int rate) {
    c->xrate = (uint8_t)rate;
}

void cache_free(void *data) {
    Cache    c = (Cache)data;
    uint64_t i;

    for (i = 0; i < c->size; i++) {
        Slot next;
        Slot s;

        for (s = c->slots[i]; NULL != s; s = next) {
            next = s->next;
            OJ_FREE(s);
        }
    }
    OJ_FREE((void *)c->slots);
    OJ_FREE(c);
}

void cache_mark(void *data) {
    Cache    c = (Cache)data;
    uint64_t i;

#if !HAVE_PTHREAD_MUTEX_INIT
    rb_gc_mark(c->mutex);
#endif
    if (0 == c->cnt) {
        return;
    }
    for (i = 0; i < c->size; i++) {
        Slot s;
        Slot prev = NULL;
        Slot next;

        for (s = c->slots[i]; NULL != s; s = next) {
            next = s->next;
            if (0 == s->use_cnt) {
                if (NULL == prev) {
                    c->slots[i] = next;
                } else {
                    prev->next = next;
                }
                c->cnt--;
                s->next  = c->reuse;
                c->reuse = s;
                c->rcnt++;
                continue;
            }
            switch (c->xrate) {
            case 0: break;
            case 2: s->use_cnt -= 2; break;
            case 3: s->use_cnt /= 2; break;
            default: s->use_cnt--; break;
            }
            if (c->mark) {
                rb_gc_mark(s->val);
            }
            prev = s;
        }
    }
}

VALUE
cache_intern(Cache c, const char *key, size_t len) {
    if (CACHE_MAX_KEY <= len) {
        return c->form(key, len);
    }
    return c->intern(c, key, len);
}
