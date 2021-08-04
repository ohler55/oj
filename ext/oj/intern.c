// Copyright (c) 2011, 2021 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#include "intern.h"

#include <stdint.h>

#if HAVE_PTHREAD_MUTEX_INIT
#include <pthread.h>
#endif
#include "parse.h"

#define HASH_SLOT_CNT ((uint32_t)8192)
#define HASH_MASK (HASH_SLOT_CNT - 1)

typedef struct _keyVal {
    struct _keyVal *next;
    const char *    key;
    size_t          len;
    VALUE           val;
} * KeyVal;

typedef struct _hash {
    struct _keyVal slots[HASH_SLOT_CNT];
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_t mutex;
#else
    VALUE mutex;
#endif
} * Hash;

struct _hash class_hash;
struct _hash str_hash;
struct _hash sym_hash;
struct _hash attr_hash;

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
    memset(attr_hash.slots, 0, sizeof(attr_hash.slots));
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_init(&class_hash.mutex, NULL);
    pthread_mutex_init(&str_hash.mutex, NULL);
    pthread_mutex_init(&sym_hash.mutex, NULL);
    pthread_mutex_init(&attr_hash.mutex, NULL);
#else
    class_hash.mutex = rb_mutex_new();
    rb_gc_register_address(&class_hash.mutex);
    str_hash.mutex = rb_mutex_new();
    rb_gc_register_address(&str_hash.mutex);
    sym_hash.mutex = rb_mutex_new();
    rb_gc_register_address(&sym_hash.mutex);
    attr_hash.mutex = rb_mutex_new();
    rb_gc_register_address(&attr_hash.mutex);
#endif
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
oj_str_intern(const char *key, size_t len) {
    uint32_t h      = hash_calc((const uint8_t *)key, len) & HASH_MASK;
    KeyVal   bucket = str_hash.slots + h;
    KeyVal   b;

#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_lock(&str_hash.mutex);
#else
    rb_mutex_lock(str_hash.mutex);
#endif
    if (NULL != bucket->key) {  // not the top slot
        for (b = bucket; 0 != b; b = b->next) {
            if (len == b->len && 0 == strncmp(b->key, key, len)) {
#if HAVE_PTHREAD_MUTEX_INIT
                pthread_mutex_unlock(&str_hash.mutex);
#else
                rb_mutex_unlock(str_hash.mutex);
#endif
                return b->val;
            }
            bucket = b;
        }
        b            = ALLOC(struct _keyVal);
        b->next      = NULL;
        bucket->next = b;
        bucket       = b;
    }
    bucket->key = oj_strndup(key, len);
    bucket->len = len;
    bucket->val = rb_utf8_str_new(key, len);
    bucket->val = rb_str_freeze(bucket->val);
    rb_gc_register_address(&bucket->val);
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_unlock(&str_hash.mutex);
#else
    rb_mutex_unlock(str_hash.mutex);
#endif
    return bucket->val;
}

VALUE
oj_sym_intern(const char *key, size_t len) {
    uint32_t h      = hash_calc((const uint8_t *)key, len) & HASH_MASK;
    KeyVal   bucket = sym_hash.slots + h;
    KeyVal   b;

#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_lock(&sym_hash.mutex);
#else
    rb_mutex_lock(sym_hash.mutex);
#endif
    if (NULL != bucket->key) {  // not the top slot
        for (b = bucket; 0 != b; b = b->next) {
            if (len == b->len && 0 == strncmp(b->key, key, len)) {
#if HAVE_PTHREAD_MUTEX_INIT
                pthread_mutex_unlock(&sym_hash.mutex);
#else
                rb_mutex_unlock(sym_hash.mutex);
#endif
                return b->val;
            }
            bucket = b;
        }
        b            = ALLOC(struct _keyVal);
        b->next      = NULL;
        bucket->next = b;
        bucket       = b;
    }
    bucket->key = oj_strndup(key, len);
    bucket->len = len;
    bucket->val = rb_utf8_str_new(key, len);
    bucket->val = rb_str_intern(bucket->val);
    rb_gc_register_address(&bucket->val);
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_unlock(&sym_hash.mutex);
#else
    rb_mutex_unlock(sym_hash.mutex);
#endif
    return bucket->val;
}

static ID form_attr(const char *key, size_t klen) {
    char attr[256];
    ID   var_id;

    if ((int)sizeof(attr) <= klen + 2) {
        char *buf = ALLOC_N(char, klen + 2);

        if ('~' == *key) {
            memcpy(buf, key + 1, klen - 1);
            buf[klen - 1] = '\0';
        } else {
            *buf = '@';
            memcpy(buf + 1, key, klen);
            buf[klen + 1] = '\0';
        }
        var_id = rb_intern(buf);
        xfree(buf);
    } else {
        if ('~' == *key) {
            memcpy(attr, key + 1, klen - 1);
            attr[klen - 1] = '\0';
        } else {
            *attr = '@';
            memcpy(attr + 1, key, klen);
            attr[klen + 1] = '\0';
        }
        var_id = rb_intern(attr);
    }
    return var_id;
}

ID oj_attr_intern(const char *key, size_t len) {
    uint32_t h      = hash_calc((const uint8_t *)key, len) & HASH_MASK;
    KeyVal   bucket = attr_hash.slots + h;
    KeyVal   b;

#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_lock(&attr_hash.mutex);
#else
    rb_mutex_lock(attr_hash.mutex);
#endif
    if (NULL != bucket->key) {  // not the top slot
        for (b = bucket; 0 != b; b = b->next) {
            if (len == b->len && 0 == strncmp(b->key, key, len)) {
#if HAVE_PTHREAD_MUTEX_INIT
                pthread_mutex_unlock(&attr_hash.mutex);
#else
                rb_mutex_unlock(attr_hash.mutex);
#endif
                return (ID)b->val;
            }
            bucket = b;
        }
        b            = ALLOC(struct _keyVal);
        b->next      = NULL;
        bucket->next = b;
        bucket       = b;
    }
    bucket->key = oj_strndup(key, len);
    bucket->len = len;
    bucket->val = (VALUE)form_attr(key, len);
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_unlock(&attr_hash.mutex);
#else
    rb_mutex_unlock(attr_hash.mutex);
#endif
    return (ID)bucket->val;
}

static VALUE resolve_classname(VALUE mod, const char *classname, int auto_define) {
    VALUE clas;
    ID    ci = rb_intern(classname);

    if (rb_const_defined_at(mod, ci)) {
        clas = rb_const_get_at(mod, ci);
    } else if (auto_define) {
        clas = rb_define_class_under(mod, classname, oj_bag_class);
    } else {
        clas = Qundef;
    }
    return clas;
}

static VALUE resolve_classpath(ParseInfo pi, const char *name, size_t len, int auto_define, VALUE error_class) {
    char        class_name[1024];
    VALUE       clas;
    char *      end = class_name + sizeof(class_name) - 1;
    char *      s;
    const char *n = name;

    clas = rb_cObject;
    for (s = class_name; 0 < len; n++, len--) {
        if (':' == *n) {
            *s = '\0';
            n++;
            len--;
            if (':' != *n) {
                return Qundef;
            }
            if (Qundef == (clas = resolve_classname(clas, class_name, auto_define))) {
                return Qundef;
            }
            s = class_name;
        } else if (end <= s) {
            return Qundef;
        } else {
            *s++ = *n;
        }
    }
    *s = '\0';
    if (Qundef == (clas = resolve_classname(clas, class_name, auto_define))) {
        oj_set_error_at(pi, error_class, __FILE__, __LINE__, "class %s is not defined", name);
        if (Qnil != error_class) {
            pi->err_class = error_class;
        }
    }
    return clas;
}

VALUE oj_class_intern(const char *key, size_t len, bool safe, ParseInfo pi, int auto_define, VALUE error_class) {
    uint32_t h      = hash_calc((const uint8_t *)key, len) & HASH_MASK;
    KeyVal   bucket = class_hash.slots + h;
    KeyVal   b;

    if (safe) {
#if HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_lock(&class_hash.mutex);
#else
        rb_mutex_lock(class_hash.mutex);
#endif
        if (NULL != bucket->key) {  // not the top slot
            for (b = bucket; 0 != b; b = b->next) {
                if (len == b->len && 0 == strncmp(b->key, key, len)) {
#if HAVE_PTHREAD_MUTEX_INIT
                    pthread_mutex_unlock(&class_hash.mutex);
#else
                    rb_mutex_unlock(class_hash.mutex);
#endif
                    return b->val;
                }
                bucket = b;
            }
            b            = ALLOC(struct _keyVal);
            b->next      = NULL;
            bucket->next = b;
            bucket       = b;
        }
        bucket->key = oj_strndup(key, len);
        bucket->len = len;
        bucket->val = resolve_classpath(pi, key, len, auto_define, error_class);
#if HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_unlock(&class_hash.mutex);
#else
        rb_mutex_unlock(class_hash.mutex);
#endif
    } else {
        if (NULL != bucket->key) {
            for (b = bucket; 0 != b; b = b->next) {
                if (len == b->len && 0 == strncmp(b->key, key, len)) {
                    return (ID)b->val;
                }
                bucket = b;
            }
            b            = ALLOC(struct _keyVal);
            b->next      = NULL;
            bucket->next = b;
            bucket       = b;
        }
        bucket->key = oj_strndup(key, len);
        bucket->len = len;
        bucket->val = resolve_classpath(pi, key, len, auto_define, error_class);
    }
    return bucket->val;
}

char *oj_strndup(const char *s, size_t len) {
    char *d = ALLOC_N(char, len + 1);

    memcpy(d, s, len);
    d[len] = '\0';

    return d;
}
