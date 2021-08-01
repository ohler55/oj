// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "encode.h"
#include "intern.h"
#include "oj.h"
#include "parser.h"

typedef struct _col {
    long vi;  // value stack index
    long ki;  // key stack index if an hash else -1 for an array
} * Col;

typedef union _key {
    struct {
        int16_t len;
        byte    buf[22];
    };
    struct {
        int16_t xlen;  // should be the same as len
        char *  key;
    };
} * Key;

typedef struct _delegate {
    // TBD change keys type to *Key

    VALUE *vhead;
    VALUE *vtail;
    VALUE *vend;

    Col chead;
    Col ctail;
    Col cend;

    Key khead;
    Key ktail;
    Key kend;

    bool thread_safe;
} * Delegate;

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

static void open_object(ojParser p) {
    // TBD
}

static void open_object_key(ojParser p) {
    // TBD
}

static void open_array(ojParser p) {
    Delegate d = (Delegate)p->ctx;

    if (d->cend <= d->ctail) {
        size_t cap = d->cend - d->chead;
        long   pos = d->ctail - d->chead;

        cap *= 2;
        REALLOC_N(d->chead, struct _col, cap);
        d->ctail = d->chead + pos;
        d->cend  = d->chead + cap;
    }
    d->ctail->vi = d->vtail - d->vhead;
    d->ctail->ki = -1;
    d->ctail++;
    push(p, Qundef);
}

static void open_array_key(ojParser p) {
    // TBD
}

static void close_object(ojParser p) {
    // TBD
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
    // TBD
}

static void add_true(ojParser p) {
    push(p, Qtrue);
}

static void add_true_key(ojParser p) {
    // TBD
}

static void add_false(ojParser p) {
    push(p, Qfalse);
}

static void add_false_key(ojParser p) {
    // TBD
}

static void add_int(ojParser p) {
    push(p, LONG2NUM(p->num.fixnum));
}

static void add_int_key(ojParser p) {
    // TBD
}

static void add_float(ojParser p) {
    push(p, rb_float_new(p->num.dub));
}

static void add_float_key(ojParser p) {
    // TBD
}

static void add_big(ojParser p) {
    push(
        p,
        rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf))));
}

static void add_big_key(ojParser p) {
    // TBD
}

static void add_str(ojParser p) {
    volatile VALUE rstr;
    const char *   str = buf_str(&p->buf);
    size_t         len = buf_len(&p->buf);

    if (p->cache_str <= len) {
        rstr = oj_str_intern(str, len, ((Delegate)p->ctx)->thread_safe);
    } else {
        rstr = oj_encode(rb_str_new(str, len));
    }
    push(p, rstr);
}

static void add_str_key(ojParser p) {
    // TBD
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
    rb_raise(rb_eArgError, "%s is not an option for the SAJ (Simple API for JSON) delegate", key);

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

    d->khead = ALLOC_N(union _key, cap);
    d->kend  = d->khead + cap;
    d->ktail = d->khead;

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
