// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "cache.h"
#include "oj.h"
#include "parser.h"

#define DEBUG 0

typedef struct _col {
    long vi;  // value stack index
    long ki;  // key stack index if an hash else -1 for an array
} * Col;

typedef union _key {
    struct {
        int16_t len;
        char    buf[22];
    };
    struct {
        int16_t xlen;  // should be the same as len
        char *  key;
    };
} * Key;

typedef struct _delegate {
    VALUE *vhead;
    VALUE *vtail;
    VALUE *vend;

    Col chead;
    Col ctail;
    Col cend;

    Key khead;
    Key ktail;
    Key kend;

    VALUE (*get_key)(ojParser p, Key kp);
    struct _cache *key_cache;  // same as str_cache or sym_cache
    struct _cache *str_cache;

    uint8_t cache_str;
    bool    cache_keys;
    bool    thread_safe;
} * Delegate;

#if DEBUG
struct debug_line {
    char pre[16];
    char val[120];
};
static void debug(const char *label, Delegate d) {
    struct debug_line  lines[100];
    struct debug_line *lp;

    memset(lines, 0, sizeof(lines));
    printf("\n%s\n", label);
    lp = lines;
    for (VALUE *vp = d->vhead; vp < d->vtail; vp++, lp++) {
        if (Qundef == *vp) {
            strncpy(lp->val, "<undefined>", sizeof(lp->val));
        } else {
            volatile VALUE rstr = rb_any_to_s(*vp);
            strncpy(lp->val, rb_string_value_cstr(&rstr), sizeof(lp->val));
        }
    }
    Key kp = NULL;
    Key np = NULL;
    for (Col cp = d->chead; cp < d->ctail; cp++) {
        if (cp->ki < 0) {
            strncpy(lines[cp->vi].pre, "<array>", sizeof(lines->pre));
        } else {
            strncpy(lines[cp->vi].pre, "<hash>", sizeof(lines->pre));
            if (NULL == kp) {
                kp = d->khead + cp->ki;
            } else {
                np = d->khead + cp->ki;
                for (int i = 0; kp < np; kp++, i++) {
                    const char *key = kp->buf;
                    if ((int)(sizeof(kp->buf) - 1) <= kp->len) {
                        key = kp->key;
                    }
                    snprintf(lines[(cp - 1)->vi + 1 + 2 * i].pre, sizeof(lines->pre), "  '%s'", key);
                }
                kp = np;
            }
        }
    }
    if (NULL != kp) {
        for (int i = 0; kp < d->ktail; kp++, i++) {
            const char *key = kp->buf;
            if ((int)(sizeof(kp->buf) - 1) <= kp->len) {
                key = kp->key;
            }
            snprintf(lines[(d->ctail - 1)->vi + 1 + 2 * i].pre, sizeof(lines->pre), "  '%s'", key);
        }
    }
    for (lp = lines; '\0' != *lp->val; lp++) {
        printf("  %-16s %s\n", lp->pre, lp->val);
    }
}
#endif

static char *str_dup(const char *s, size_t len) {
    char *d = ALLOC_N(char, len + 1);

    memcpy(d, s, len);
    d[len] = '\0';

    return d;
}

static void assure_cstack(Delegate d) {
    if (d->cend <= d->ctail + 1) {
        size_t cap = d->cend - d->chead;
        long   pos = d->ctail - d->chead;

        cap *= 2;
        REALLOC_N(d->chead, struct _col, cap);
        d->ctail = d->chead + pos;
        d->cend  = d->chead + cap;
    }
}

static void push(ojParser p, VALUE v) {
    Delegate d = (Delegate)p->ctx;

    if (d->vend <= d->vtail) {
        size_t cap = d->vend - d->vhead;
        long   pos = d->vtail - d->vhead;

        cap *= 2;
        REALLOC_N(d->vhead, VALUE, cap);
        d->vtail = d->vhead + pos;
        d->vend  = d->vhead + cap;
    }
    *d->vtail = v;
    d->vtail++;
}

static VALUE cache_key(ojParser p, Key kp) {
    Delegate d = (Delegate)p->ctx;

    if ((size_t)kp->len < sizeof(kp->buf) - 1) {
        return cache_intern(d->key_cache, kp->buf, kp->len);
    }
    return cache_intern(d->key_cache, kp->key, kp->len);
}

static VALUE create_key(ojParser p, Key kp) {
    if ((size_t)kp->len < sizeof(kp->buf) - 1) {
        return rb_utf8_str_new(kp->buf, kp->len);
    }
    return rb_utf8_str_new(kp->key, kp->len);
}

static void push_key(ojParser p) {
    Delegate    d    = (Delegate)p->ctx;
    size_t      klen = buf_len(&p->key);
    const char *key  = buf_str(&p->key);

    if (d->kend <= d->ktail) {
        size_t cap = d->kend - d->khead;
        long   pos = d->ktail - d->khead;

        cap *= 2;
        REALLOC_N(d->khead, union _key, cap);
        d->ktail = d->khead + pos;
        d->kend  = d->khead + cap;
    }
    d->ktail->len = klen;
    if (klen <= sizeof(d->ktail->buf) + 1) {
        memcpy(d->ktail->buf, key, klen);
        d->ktail->buf[klen] = '\0';
    } else {
        d->ktail->key = str_dup(key, klen);
    }
    d->ktail++;
}

static void push2(ojParser p, VALUE v) {
    Delegate d = (Delegate)p->ctx;

    if (d->vend <= d->vtail + 1) {
        size_t cap = d->vend - d->vhead;
        long   pos = d->vtail - d->vhead;

        cap *= 2;
        REALLOC_N(d->vhead, VALUE, cap);
        d->vtail = d->vhead + pos;
        d->vend  = d->vhead + cap;
    }
    *d->vtail = Qundef;  // key place holder
    d->vtail++;
    *d->vtail = v;
    d->vtail++;
}

static void open_object(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    assure_cstack(d);
    d->ctail->vi = d->vtail - d->vhead;
    d->ctail->ki = d->ktail - d->khead;
    d->ctail++;
    push(p, Qundef);
}

static void open_object_key(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    push_key(p);
    assure_cstack(d);
    d->ctail->vi = d->vtail - d->vhead + 1;
    d->ctail->ki = d->ktail - d->khead;
    d->ctail++;
    push2(p, Qundef);
}

static void open_array(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    assure_cstack(d);
    d->ctail->vi = d->vtail - d->vhead;
    d->ctail->ki = -1;
    d->ctail++;
    push(p, Qundef);
}

static void open_array_key(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    push_key(p);
    assure_cstack(d);
    d->ctail->vi = d->vtail - d->vhead + 1;
    d->ctail->ki = -1;
    d->ctail++;
    push2(p, Qundef);
}

static void close_object(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    d->ctail--;

    Col            c    = d->ctail;
    Key            kp   = d->khead + c->ki;
    VALUE *        head = d->vhead + c->vi + 1;
    volatile VALUE obj;

    // TBD deal with provided hash and/or object
    obj = rb_hash_new();
    for (VALUE *vp = head; kp < d->ktail; kp++, vp += 2) {
        // TBD symbol or string
        *vp = d->get_key(p, kp);
        if (sizeof(kp->buf) - 1 < (size_t)kp->len) {
            xfree(kp->key);
        }
    }
    d->ktail = d->khead + c->ki;
    rb_hash_bulk_insert(d->vtail - head, head, obj);
    d->vtail = head;
    head--;
    *head = obj;
}

static void close_array(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    d->ctail--;
    VALUE *        head = d->vhead + d->ctail->vi + 1;
    volatile VALUE a    = rb_ary_new_from_values(d->vtail - head, head);

    d->vtail = head;
    head--;
    *head = a;
}

static void add_null(ojParser p) {
    push(p, Qnil);
}

static void add_null_key(ojParser p) {
    push_key(p);
    push2(p, Qnil);
}

static void add_true(ojParser p) {
    push(p, Qtrue);
}

static void add_true_key(ojParser p) {
    push_key(p);
    push2(p, Qtrue);
}

static void add_false(ojParser p) {
    push(p, Qfalse);
}

static void add_false_key(ojParser p) {
    push_key(p);
    push2(p, Qfalse);
}

static void add_int(ojParser p) {
    push(p, LONG2NUM(p->num.fixnum));
}

static void add_int_key(ojParser p) {
    push_key(p);
    push2(p, LONG2NUM(p->num.fixnum));
}

static void add_float(ojParser p) {
    push(p, rb_float_new(p->num.dub));
}

static void add_float_key(ojParser p) {
    push_key(p);
    push2(p, rb_float_new(p->num.dub));
}

static void add_big(ojParser p) {
    push(p, rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf))));
}

static void add_big_key(ojParser p) {
    push_key(p);
    push2(p, rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf))));
}

static void add_str(ojParser p) {
    Delegate       d = (Delegate)p->ctx;
    volatile VALUE rstr;
    const char *   str = buf_str(&p->buf);
    size_t         len = buf_len(&p->buf);

    if (len < d->cache_str) {
        rstr = cache_intern(d->str_cache, str, len);
    } else {
        rstr = rb_utf8_str_new(str, len);
    }
    push(p, rstr);
}

static void add_str_key(ojParser p) {
    Delegate       d = (Delegate)p->ctx;
    volatile VALUE rstr;
    const char *   str = buf_str(&p->buf);
    size_t         len = buf_len(&p->buf);

    if (len < d->cache_str) {
        rstr = cache_intern(d->str_cache, str, len);
    } else {
        rstr = rb_utf8_str_new(str, len);
    }
    push_key(p);
    push2(p, rstr);
}

static VALUE option(ojParser p, const char *key, VALUE value) {
    Delegate d = (Delegate)p->ctx;

    if (0 == strcmp(key, "cache_keys")) {
        return d->cache_keys ? Qtrue : Qfalse;
    }
    if (0 == strcmp(key, "cache_keys=")) {
	if (Qtrue == value) {
	    d->cache_keys = true;
	    d->get_key = cache_key;
	    // TBD check symbol_keys
	    d->key_cache = d->str_cache;
	} else {
	    d->cache_keys = false;
	    d->get_key = create_key;
	    // TBD check symbol_keys
	}
        return d->cache_keys ? Qtrue : Qfalse;
    }
    if (0 == strcmp(key, "cache_strings")) {
        return INT2NUM((int)d->cache_str);
    }
    if (0 == strcmp(key, "cache_strings=")) {
        int limit = NUM2INT(value);

        if (CACHE_MAX_KEY < limit) {
            limit = CACHE_MAX_KEY;
        } else if (limit < 0) {
            limit = 0;
        }
        d->cache_str = limit;

        return INT2NUM((int)d->cache_str);
    }
    rb_raise(rb_eArgError, "%s is not an option for the Usual delegate", key);

    return Qnil;  // Never reached due to the raise but required by the compiler.
}

static VALUE result(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    if (d->vhead < d->vtail) {
        return *d->vhead;
    }
    return Qnil;
}

static void start(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    d->vtail = d->vhead;
    d->ctail = d->chead;
    d->ktail = d->khead;
}

static void dfree(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    cache_free(d->str_cache);
    xfree(d->vhead);
    xfree(d->chead);
    xfree(d->khead);
    xfree(p->ctx);
}

static void mark(ojParser p) {
    if (NULL == p->ctx) {
        return;
    }
    Delegate d = (Delegate)p->ctx;

    for (VALUE *vp = d->vhead; vp < d->vtail; vp++) {
        if (Qundef != *vp) {
            rb_gc_mark(*vp);
        }
    }
}
static VALUE form_str(const char *str, size_t len) {
    return rb_str_freeze(rb_utf8_str_new(str, len));
}

void oj_set_parser_usual(ojParser p) {
    Delegate d   = ALLOC(struct _delegate);
    int      cap = 4096;

    d->vhead = ALLOC_N(VALUE, cap);
    d->vend  = d->vhead + cap;
    d->vtail = d->vhead;

    d->khead = ALLOC_N(union _key, cap);
    d->kend  = d->khead + cap;
    d->ktail = d->khead;
    d->get_key = cache_key;
    d->cache_keys = true;
    d->cache_str = 6;

    cap      = 256;
    d->chead = ALLOC_N(struct _col, cap);
    d->cend  = d->chead + cap;
    d->ctail = d->chead;

    Funcs f         = &p->funcs[TOP_FUN];
    f->add_null     = add_null;
    f->add_true     = add_true;
    f->add_false    = add_false;
    f->add_int      = add_int;
    f->add_float    = add_float;
    f->add_big      = add_big;
    f->add_str      = add_str;
    f->open_array   = open_array;
    f->close_array  = close_array;
    f->open_object  = open_object;
    f->close_object = close_object;

    f               = &p->funcs[ARRAY_FUN];
    f->add_null     = add_null;
    f->add_true     = add_true;
    f->add_false    = add_false;
    f->add_int      = add_int;
    f->add_float    = add_float;
    f->add_big      = add_big;
    f->add_str      = add_str;
    f->open_array   = open_array;
    f->close_array  = close_array;
    f->open_object  = open_object;
    f->close_object = close_object;

    f               = &p->funcs[OBJECT_FUN];
    f->add_null     = add_null_key;
    f->add_true     = add_true_key;
    f->add_false    = add_false_key;
    f->add_int      = add_int_key;
    f->add_float    = add_float_key;
    f->add_big      = add_big_key;
    f->add_str      = add_str_key;
    f->open_array   = open_array_key;
    f->close_array  = close_array;
    f->open_object  = open_object_key;
    f->close_object = close_object;

    d->str_cache = cache_create(0, form_str, true);
    d->key_cache = d->str_cache;

    p->ctx    = (void *)d;
    p->option = option;
    p->result = result;
    p->free   = dfree;
    p->mark   = mark;
    p->start  = start;
}
