// Copyright (c) 2011, 2021 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#include "intern.h"

#include <stdint.h>

#if HAVE_PTHREAD_MUTEX_INIT
#include <pthread.h>
#endif

#include "cache.h"
#include "mem.h"
#include "parse.h"

// Only used for the class cache so 256 should be sufficient.
#define HASH_SLOT_CNT ((uint64_t)256)
#define HASH_MASK (HASH_SLOT_CNT - 1)

typedef struct _keyVal {
    struct _keyVal *next;
    const char     *key;
    size_t          len;
    VALUE           val;
} *KeyVal;

typedef struct _hash {
    struct _keyVal slots[HASH_SLOT_CNT];
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_t mutex;
#else
    VALUE mutex;
#endif
} *Hash;

struct _hash class_hash;
struct _hash attr_hash;

static VALUE str_cache_obj;

static VALUE sym_cache_obj;

static VALUE attr_cache_obj;

static VALUE form_str(const char *str, size_t len) {
    return rb_str_freeze(rb_utf8_str_new(str, len));
}

static VALUE form_sym(const char *str, size_t len) {
    VALUE sym = rb_check_symbol_cstr(str, (long)len, oj_utf8_encoding);

    if (Qnil == sym) {
        sym = rb_str_intern(rb_utf8_str_new(str, len));
    }
    return sym;
}

static VALUE form_attr(const char *str, size_t len) {
    char buf[256];

    if (sizeof(buf) - 2 <= len) {
        VALUE v;
        char *b = ALLOCV_N(char, v, len + 2);
        ID    id;

        if ('~' == *str) {
            memcpy(b, str + 1, len - 1);
            b[len - 1] = '\0';
            len -= 2;
        } else {
            *b = '@';
            memcpy(b + 1, str, len);
            b[len + 1] = '\0';
        }
        id = rb_intern3(b, len + 1, oj_utf8_encoding);
        ALLOCV_END(v);
        return id;
    }
    if ('~' == *str) {
        memcpy(buf, str + 1, len - 1);
        buf[len - 1] = '\0';
        len -= 2;
    } else {
        *buf = '@';
        memcpy(buf + 1, str, len);
        buf[len + 1] = '\0';
    }
    return (VALUE)rb_intern3(buf, len + 1, oj_utf8_encoding);
}

static const rb_data_type_t oj_cache_type = {
    "Oj/cache",
    {
        cache_mark,
        cache_free,
        NULL,
    },
    0,
    0,
};

void oj_hash_init(void) {
    oj_hash_seed_init();

    VALUE cache_class = rb_define_class_under(Oj, "Cache", rb_cObject);
    rb_undef_alloc_func(cache_class);

    struct _cache *str_cache = cache_create(0, form_str, true, true);
    str_cache_obj            = TypedData_Wrap_Struct(cache_class, &oj_cache_type, str_cache);
    rb_gc_register_address(&str_cache_obj);

    struct _cache *sym_cache = cache_create(0, form_sym, true, true);
    sym_cache_obj            = TypedData_Wrap_Struct(cache_class, &oj_cache_type, sym_cache);
    rb_gc_register_address(&sym_cache_obj);

    struct _cache *attr_cache = cache_create(0, form_attr, false, true);
    attr_cache_obj            = TypedData_Wrap_Struct(cache_class, &oj_cache_type, attr_cache);
    rb_gc_register_address(&attr_cache_obj);

    memset(class_hash.slots, 0, sizeof(class_hash.slots));
#if HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_init(&class_hash.mutex, NULL);
#else
    class_hash.mutex = rb_mutex_new();
    rb_gc_register_address(&class_hash.mutex);
#endif
}

VALUE
oj_str_intern(const char *key, size_t len) {
    // For huge cache sizes over half a million the rb_enc_interned_str
    // performs slightly better but at more "normal" size of a several
    // thousands the cache intern performs about 20% better.
#if HAVE_RB_ENC_INTERNED_STR && 0
    return rb_enc_interned_str(key, len, rb_utf8_encoding());
#else
    Cache c;
    TypedData_Get_Struct(str_cache_obj, struct _cache, &oj_cache_type, c);
    return cache_intern(c, key, len);
#endif
}

VALUE
oj_sym_intern(const char *key, size_t len) {
    Cache c;
    TypedData_Get_Struct(sym_cache_obj, struct _cache, &oj_cache_type, c);
    return cache_intern(c, key, len);
}

ID oj_attr_intern(const char *key, size_t len) {
    Cache c;
    TypedData_Get_Struct(attr_cache_obj, struct _cache, &oj_cache_type, c);
    return cache_intern(c, key, len);
}

static VALUE resolve_classname(VALUE mod, const char *classname, int auto_define) {
    VALUE clas;
    ID    ci = rb_intern(classname);

    if (rb_const_defined_at(mod, ci)) {
        clas = rb_const_get_at(mod, ci);
        if (!RB_TYPE_P(clas, T_CLASS) && !RB_TYPE_P(clas, T_MODULE)) {
            clas = Qundef;
        }
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
    char       *end = class_name + sizeof(class_name) - 1;
    char       *s;
    const char *n    = name;
    size_t      nlen = len;

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
        if (sizeof(class_name) <= nlen) {
            nlen = sizeof(class_name) - 1;
        }
        strncpy(class_name, name, nlen);
        class_name[nlen] = '\0';
        oj_set_error_at(pi, error_class, __FILE__, __LINE__, "class '%s' is not defined", class_name);
        if (Qnil != error_class) {
            pi->err_class = error_class;
        }
    }
    return clas;
}

static KeyVal find_class(KeyVal bucket, const char *key, size_t len, VALUE *clas) {
    if (NULL != bucket->key) {  // not the top slot
        for (KeyVal b = bucket; 0 != b; b = b->next) {
            if (len == b->len && 0 == memcmp(b->key, key, len)) {
                *clas = b->val;
                return NULL;
            }
            bucket = b;
        }
    }
    return bucket;
}

static void class_hash_lock(bool safe) {
    if (safe) {
#if HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_lock(&class_hash.mutex);
#else
        rb_mutex_lock(class_hash.mutex);
#endif
    }
}

static void class_hash_unlock(bool safe) {
    if (safe) {
#if HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_unlock(&class_hash.mutex);
#else
        rb_mutex_unlock(class_hash.mutex);
#endif
    }
}

// Resolving a class calls into Ruby, where an autoload can load a class with
// Oj or let another thread run, so the lock is only held to search the cache
// and to add an entry. Nothing that can raise is called while it is held.
VALUE oj_class_intern(const char *key, size_t len, bool safe, ParseInfo pi, int auto_define, VALUE error_class) {
    KeyVal slot  = class_hash.slots + (oj_hash_calc((const uint8_t *)key, len) & HASH_MASK);
    VALUE  clas  = Qundef;
    VALUE  found = Qundef;
    KeyVal bucket;
    KeyVal b;
    char  *k;

    class_hash_lock(safe);
    bucket = find_class(slot, key, len, &clas);
    class_hash_unlock(safe);
    if (NULL == bucket) {
        return clas;
    }
    if (Qundef == (clas = resolve_classpath(pi, key, len, auto_define, error_class))) {
        return clas;
    }
    k = oj_strndup(key, len);
    b = OJ_R_ALLOC(struct _keyVal);

    class_hash_lock(safe);
    if (NULL == (bucket = find_class(slot, key, len, &found))) {
        class_hash_unlock(safe);
        OJ_R_FREE(b);
        OJ_R_FREE(k);
        return found;
    }
    if (NULL != bucket->key) {  // not the top slot
        bucket->next = b;
        bucket       = b;
        b            = NULL;
    }
    bucket->next = NULL;
    bucket->key  = k;
    bucket->len  = len;
    bucket->val  = clas;
    class_hash_unlock(safe);
    if (NULL != b) {
        OJ_R_FREE(b);
    }
    rb_gc_register_mark_object(clas);

    return clas;
}

char *oj_strndup(const char *s, size_t len) {
    char *d = OJ_R_ALLOC_N(char, len + 1);

    memcpy(d, s, len);
    d[len] = '\0';

    return d;
}

/*
void intern_cleanup(void) {
    cache_free(str_cache);
    cache_free(sym_cache);
    cache_free(attr_cache);
}
*/
