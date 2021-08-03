// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "intern.h"
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
    bool thread_safe;
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

static VALUE intern_key(ojParser p, Key kp) {
    Delegate d = (Delegate)p->ctx;

    if ((size_t)kp->len < sizeof(kp->buf) - 1) {
        return oj_str_intern(kp->buf, kp->len, d->thread_safe);
    }
    return oj_str_intern(kp->key, kp->len, d->thread_safe);
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
        d->ktail->key = oj_strndup(key, klen);
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
    }
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
    volatile VALUE rstr;
    const char *   str = buf_str(&p->buf);
    size_t         len = buf_len(&p->buf);

    if (len < p->cache_str) {
        rstr = oj_str_intern(str, len, ((Delegate)p->ctx)->thread_safe);
    } else {
        rstr = rb_utf8_str_new(str, len);
    }
    push(p, rstr);
}

static void add_str_key(ojParser p) {
    volatile VALUE rstr;
    const char *   str = buf_str(&p->buf);
    size_t         len = buf_len(&p->buf);

    if (len < p->cache_str) {
        rstr = oj_str_intern(str, len, ((Delegate)p->ctx)->thread_safe);
    } else {
        rstr = rb_utf8_str_new(str, len);
    }
    push_key(p);
    push2(p, rstr);
}

static VALUE option(ojParser p, const char *key, VALUE value) {
    if (0 == strcmp(key, "ractor_safe") || 0 == strcmp(key, "thread_safe")) {
        return ((Delegate)p->ctx)->thread_safe ? Qtrue : Qfalse;
    }
    if (0 == strcmp(key, "ractor_safe=") || 0 == strcmp(key, "thread_safe=")) {
        if (Qtrue == value) {
            ((Delegate)p->ctx)->thread_safe = true;
        } else if (Qfalse == value) {
            ((Delegate)p->ctx)->thread_safe = false;
        } else {
            rb_raise(rb_eArgError, "invalid value for thread_safe/ractor_safe option");
        }
        return value;
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

#if HAVE_RB_EXT_RACTOR_SAFE
    rb_ext_ractor_safe(d->thread_safe || (!p->cache_keys && p->cache_str <= 0));
#endif
    d->vtail = d->vhead;
    d->ctail = d->chead;
    d->ktail = d->khead;
}

static void dfree(ojParser p) {
    Delegate d = (Delegate)p->ctx;

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

void oj_set_parser_usual(ojParser p) {
    Delegate d   = ALLOC(struct _delegate);
    int      cap = 4096;

    d->vhead = ALLOC_N(VALUE, cap);
    d->vend  = d->vhead + cap;
    d->vtail = d->vhead;

    d->khead   = ALLOC_N(union _key, cap);
    d->kend    = d->khead + cap;
    d->ktail   = d->khead;
    d->get_key = intern_key;
    //d->get_key = create_key;

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

    p->ctx    = (void *)d;
    p->option = option;
    p->result = result;
    p->free   = dfree;
    p->mark   = mark;
    p->start  = start;
}
