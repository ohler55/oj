// Copyright (c) 2011, 2021 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#if HAVE_PTHREAD_MUTEX_INIT
#include <pthread.h>
#endif

#include "cache.h"

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

// almost the Murmur hash algorithm
#define M 0x5bd1e995

typedef struct _slot {
    struct _slot *next;
    VALUE         val;
    uint64_t      hash;
    uint32_t      use_cnt;
    uint8_t       klen;
    char          key[CACHE_MAX_KEY];
} * Slot;

typedef struct _cache {
    Slot * slots;
    size_t cnt;
    VALUE (*form)(const char *str, size_t len);
    uint64_t size;
    uint64_t mask;
    VALUE (*intern)(struct _cache *c, const char *key, size_t len);
    Slot   reuse;
    size_t rcnt;
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_t mutex;
#else
    VALUE mutex;
#endif
    uint8_t xrate;
    bool    mark;
} * Cache;

void cache_set_form(Cache c, VALUE (*form)(const char *str, size_t len)) {
    c->form = form;
}

#if 0
// For debugging only.
static void cache_print(Cache c) {
    for (uint64_t i = 0; i < c->size; i++) {
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

static uint64_t hash_calc(const uint8_t *key, size_t len) {
    const uint8_t *end     = key + len;
    const uint8_t *endless = key + (len & 0xFFFFFFFC);
    uint64_t       h       = (uint64_t)len;
    uint64_t       k;

    while (key < endless) {
        k = (uint64_t)*key++;
        k |= (uint64_t)*key++ << 8;
        k |= (uint64_t)*key++ << 16;
        k |= (uint64_t)*key++ << 24;

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

static void rehash(Cache c) {
    uint64_t osize = c->size;
    Slot *   end;
    Slot *   sp;

    c->size = osize * 4;
    c->mask = c->size - 1;
    REALLOC_N(c->slots, Slot, c->size);
    memset(c->slots + osize, 0, sizeof(Slot) * osize * 3);
    end = c->slots + osize;
    for (sp = c->slots; sp < end; sp++) {
        Slot s    = *sp;
        Slot next = NULL;

        *sp = NULL;
        for (; NULL != s; s = next) {
            uint64_t h      = s->hash & c->mask;
            Slot *   bucket = c->slots + h;

            next    = s->next;
            s->next = *bucket;
            *bucket = s;
        }
    }
}

static VALUE lockless_intern(Cache c, const char *key, size_t len) {
    uint64_t h      = hash_calc((const uint8_t *)key, len);
    Slot *   bucket = c->slots + (h & c->mask);
    Slot     b;

    while (REUSE_MAX < c->rcnt) {
        if (NULL != (b = c->reuse)) {
            c->reuse = b->next;
            xfree(b);
            c->rcnt--;
        } else {
            // An accounting error occured somewhere so correct it.
            c->rcnt = 0;
        }
    }
    for (b = *bucket; NULL != b; b = b->next) {
        if ((uint8_t)len == b->klen && 0 == strncmp(b->key, key, len)) {
            b->use_cnt += 4;
            return b->val;
        }
    }
    {
        volatile VALUE rkey = c->form(key, len);

        if (NULL == (b = c->reuse)) {
            b = ALLOC(struct _slot);
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
    }
    return b->val;
}

static VALUE locking_intern(Cache c, const char *key, size_t len) {
    uint64_t h;
    Slot *   bucket;
    Slot     b;
    uint64_t old_size;

    CACHE_LOCK(c);
    while (REUSE_MAX < c->rcnt) {
        if (NULL != (b = c->reuse)) {
            c->reuse = b->next;
            xfree(b);
            c->rcnt--;
        } else {
            // An accounting error occured somewhere so correct it.
            c->rcnt = 0;
        }
    }
    h      = hash_calc((const uint8_t *)key, len);
    bucket = c->slots + (h & c->mask);
    for (b = *bucket; NULL != b; b = b->next) {
        if ((uint8_t)len == b->klen && 0 == strncmp(b->key, key, len)) {
            b->use_cnt += 4;
            CACHE_UNLOCK(c);
            return b->val;
        }
    }
    old_size = c->size;
    // The creation of a new value may trigger a GC which be a problem if the
    // cache is locked so make sure it is unlocked for the key value creation.
    if (NULL == (b = c->reuse)) {
        b = ALLOC(struct _slot);
    } else {
        c->reuse = b->next;
        c->rcnt--;
    }
    CACHE_UNLOCK(c);
    {
        volatile VALUE rkey = c->form(key, len);

        b->hash = h;
        memcpy(b->key, key, len);
        b->klen     = (uint8_t)len;
        b->key[len] = '\0';
        b->val      = rkey;
        b->use_cnt  = 4;

        // Lock again to add the new entry.
        CACHE_LOCK(c);
        if (old_size != c->size) {
            h      = hash_calc((const uint8_t *)key, len);
            bucket = c->slots + (h & c->mask);
        }
        b->next = *bucket;
        *bucket = b;
        c->cnt++;  // Don't worry about wrapping. Worse case is the entry is removed and recreated.
        if (REHASH_LIMIT < c->cnt / c->size) {
            rehash(c);
        }
        CACHE_UNLOCK(c);
    }
    return b->val;
}

Cache cache_create(size_t size, VALUE (*form)(const char *str, size_t len), bool mark, bool locking) {
    Cache c     = ALLOC(struct _cache);
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
    c->slots = ALLOC_N(Slot, c->size);
    memset(c->slots, 0, sizeof(Slot) * c->size);
    c->form  = form;
    c->cnt   = 0;
    c->xrate = 1;  // low
    c->mark  = mark;
    c->reuse = NULL;
    c->rcnt  = 0;
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

void cache_free(Cache c) {
    uint64_t i;

    for (i = 0; i < c->size; i++) {
        Slot next;
        Slot s;

        for (s = c->slots[i]; NULL != s; s = next) {
            next = s->next;
            xfree(s);
        }
    }
    xfree(c->slots);
    xfree(c);
}

void cache_mark(Cache c) {
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
    if (CACHE_MAX_KEY < len) {
        return c->form(key, len);
    }
    return c->intern(c, key, len);
}
