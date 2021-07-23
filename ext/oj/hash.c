// Copyright (c) 2011 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#include "hash.h"

#include <stdint.h>

#define HASH_SLOT_CNT ((uint32_t)8192)
#define HASH_MASK (HASH_SLOT_CNT - 1)

typedef struct _keyVal {
    struct _keyVal *next;
    const char *    key;
    size_t          len;
    VALUE           val;
} * KeyVal;

struct _hash {
    struct _keyVal slots[HASH_SLOT_CNT];
};

struct _hash class_hash;
struct _hash str_hash;
struct _hash sym_hash;
struct _hash intern_hash;

// almost the Murmur hash algorithm
#define M 0x5bd1e995
#define C1 0xCC9E2D51
#define C2 0x1B873593
#define N 0xE6546B64

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

void oj_hash_init() {
    memset(class_hash.slots, 0, sizeof(class_hash.slots));
    memset(str_hash.slots, 0, sizeof(str_hash.slots));
    memset(sym_hash.slots, 0, sizeof(sym_hash.slots));
    memset(intern_hash.slots, 0, sizeof(intern_hash.slots));
}

// if slotp is 0 then just lookup
static VALUE hash_get(Hash hash, const char *key, size_t len, VALUE **slotp, VALUE def_value) {
    uint32_t h      = hash_calc((const uint8_t *)key, len) & HASH_MASK;
    KeyVal   bucket = hash->slots + h;

    if (0 != bucket->key) {
        KeyVal b;

        for (b = bucket; 0 != b; b = b->next) {
            if (len == b->len && 0 == strncmp(b->key, key, len)) {
                *slotp = &b->val;
                return b->val;
            }
            bucket = b;
        }
    }
    if (0 != slotp) {
        if (0 != bucket->key) {
            KeyVal b = ALLOC(struct _keyVal);

            b->next      = 0;
            bucket->next = b;
            bucket       = b;
        }
        bucket->key = oj_strndup(key, len);
        bucket->len = len;
        bucket->val = def_value;
        *slotp      = &bucket->val;
    }
    return def_value;
}

void oj_hash_print() {
    uint32_t i;
    KeyVal   b;

    for (i = 0; i < HASH_SLOT_CNT; i++) {
        printf("%4d:", i);
        for (b = class_hash.slots + i; 0 != b && 0 != b->key; b = b->next) {
            printf(" %s", b->key);
        }
        printf("\n");
    }
}

void oj_hash_sizes() {
    uint32_t i;
    KeyVal   b;
    int      max = 0;
    int      min = 1000000;

    for (i = 0; i < HASH_SLOT_CNT; i++) {
        int cnt = 0;

        for (b = str_hash.slots + i; 0 != b && 0 != b->key; b = b->next) {
            cnt++;
        }
        // printf(" %4d\n", cnt);
        if (max < cnt) {
            max = cnt;
        }
        if (cnt < min) {
            min = cnt;
        }
    }
    printf("min: %d  max: %d\n", min, max);
}

VALUE
oj_class_hash_get(const char *key, size_t len, VALUE **slotp) {
    return hash_get(&class_hash, key, len, slotp, Qnil);
}

VALUE
oj_str_hash_get(const char *key, size_t len, VALUE **slotp) {
    return hash_get(&str_hash, key, len, slotp, Qnil);
}

VALUE
oj_sym_hash_get(const char *key, size_t len, VALUE **slotp) {
    return hash_get(&sym_hash, key, len, slotp, Qnil);
}

ID oj_attr_hash_get(const char *key, size_t len, ID **slotp) {
    return (ID)hash_get(&intern_hash, key, len, (VALUE **)slotp, 0);
}

char *oj_strndup(const char *s, size_t len) {
    char *d = ALLOC_N(char, len + 1);

    memcpy(d, s, len);
    d[len] = '\0';

    return d;
}
