// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "encode.h"
#include "hash.h"
#include "oj.h"
#include "parser.h"

typedef struct _delegate {
    VALUE  handler;
    VALUE *keys;
    VALUE *tail;
    size_t klen;
} * Delegate;

static VALUE get_key(ojParser p) {
    const char *   key = buf_str(&p->key);
    volatile VALUE rkey;

    if (p->cache_keys) {
        VALUE *slot;
        size_t len = strlen(key);

        if (Qnil == (rkey = oj_str_hash_get(key, len, &slot))) {
            rkey  = oj_encode(rb_str_new(key, len));
            rkey  = rb_str_freeze(rkey);
            *slot = rkey;
            rb_gc_register_address(slot);
        }
    } else {
        rkey = oj_encode(rb_str_new2(key));
    }
    return rkey;
}

static void push_key(Delegate d, VALUE key) {
    if (d->klen <= (size_t)(d->tail - d->keys)) {
        size_t off = d->tail - d->keys;

        d->klen += d->klen / 2;
        REALLOC_N(d->keys, VALUE, d->klen);
        d->tail = d->keys + off;
    }
    *d->tail = key;
    d->tail++;
}

static void noop(struct _ojParser *p) {
}

static void open_object(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_hash_start_id, 1, Qnil);
}

static void open_object_key(struct _ojParser *p) {
    Delegate       d   = (Delegate)p->ctx;
    volatile VALUE key = get_key(p);

    push_key(d, key);
    rb_funcall(d->handler, oj_hash_start_id, 1, key);
}

static void open_array(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_array_start_id, 1, Qnil);
}

static void open_array_key(struct _ojParser *p) {
    Delegate       d   = (Delegate)p->ctx;
    volatile VALUE key = get_key(p);

    push_key(d, key);
    rb_funcall(d->handler, oj_array_start_id, 1, key);
}

static void close_object(struct _ojParser *p) {
    Delegate d   = (Delegate)p->ctx;
    VALUE    key = Qnil;

    if (OBJECT_FUN == p->stack[p->depth]) {
        d->tail--;
        if (d->tail < d->keys) {
            rb_raise(rb_eIndexError, "accessing key stack");
        }
        key = *d->tail;
    }
    rb_funcall(d->handler, oj_hash_end_id, 1, key);
}

static void close_array(struct _ojParser *p) {
    Delegate d   = (Delegate)p->ctx;
    VALUE    key = Qnil;

    if (OBJECT_FUN == p->stack[p->depth]) {
        d->tail--;
        if (d->tail < d->keys) {
            rb_raise(rb_eIndexError, "accessing key stack");
        }
        key = *d->tail;
    }
    rb_funcall(d->handler, oj_array_end_id, 1, key);
}

static void add_null(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, Qnil, Qnil);
}

static void add_null_key(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, Qnil, get_key(p));
}

static void add_true(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, Qtrue, Qnil);
}

static void add_true_key(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, Qtrue, get_key(p));
}

static void add_false(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, Qfalse, Qnil);
}

static void add_false_key(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, Qfalse, get_key(p));
}

static void add_int(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, LONG2NUM(p->num.fixnum), Qnil);
}

static void add_int_key(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, LONG2NUM(p->num.fixnum), get_key(p));
}

static void add_float(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, rb_float_new(p->num.dub), Qnil);
}

static void add_float_key(struct _ojParser *p) {
    rb_funcall(((Delegate)p->ctx)->handler,
               oj_add_value_id,
               2,
               rb_float_new(p->num.dub),
               get_key(p));
}

static void add_big(struct _ojParser *p) {
    rb_funcall(
        (VALUE)p->ctx,
        oj_add_value_id,
        2,
        rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf))),
        Qnil);
}

static void add_big_key(struct _ojParser *p) {
    rb_funcall(
        (VALUE)p->ctx,
        oj_add_value_id,
        2,
        rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf))),
        get_key(p));
}

static void add_str(struct _ojParser *p) {
    volatile VALUE rstr;
    const char *   str = buf_str(&p->buf);
    size_t         len = buf_len(&p->buf);

    if (p->cache_str <= len) {
        VALUE *slot;

        if (Qnil == (rstr = oj_str_hash_get(str, len, &slot))) {
            rstr  = oj_encode(rb_str_new(str, len));
            rstr  = rb_str_freeze(rstr);
            *slot = rstr;
            rb_gc_register_address(slot);
        }
    } else {
        rstr = oj_encode(rb_str_new(str, len));
    }
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, rstr, Qnil);
}

static void add_str_key(struct _ojParser *p) {
    volatile VALUE rstr;
    const char *   str = buf_str(&p->buf);
    size_t         len = buf_len(&p->buf);

    if (p->cache_str <= len) {
        VALUE *slot;

        if (Qnil == (rstr = oj_str_hash_get(str, len, &slot))) {
            rstr  = oj_encode(rb_str_new(str, len));
            rstr  = rb_str_freeze(rstr);
            *slot = rstr;
            rb_gc_register_address(slot);
        }
    } else {
        rstr = oj_encode(rb_str_new(str, len));
    }
    rb_funcall(((Delegate)p->ctx)->handler, oj_add_value_id, 2, rstr, get_key(p));
}

static void reset(ojParser p) {
    Funcs end = p->funcs + 3;

    for (Funcs f = p->funcs; f < end; f++) {
        f->add_null     = noop;
        f->add_true     = noop;
        f->add_false    = noop;
        f->add_int      = noop;
        f->add_float    = noop;
        f->add_big      = noop;
        f->add_str      = noop;
        f->open_array   = noop;
        f->close_array  = noop;
        f->open_object  = noop;
        f->close_object = noop;
    }
}

static VALUE option(ojParser p, const char *key, VALUE value) {
    if (0 == strcmp(key, "handler")) {
        return (VALUE)p->ctx;
    }
    if (0 == strcmp(key, "handler=")) {
        Delegate d = (Delegate)p->ctx;

        d->tail    = d->keys;
        d->handler = value;
	reset(p);
        if (rb_respond_to(value, oj_hash_start_id)) {
            p->funcs[0].open_object = open_object;
            p->funcs[1].open_object = open_object;
            p->funcs[2].open_object = open_object_key;
        }
        if (rb_respond_to(value, oj_array_start_id)) {
            p->funcs[0].open_array = open_array;
            p->funcs[1].open_array = open_array;
            p->funcs[2].open_array = open_array_key;
        }
        if (rb_respond_to(value, oj_hash_end_id)) {
            p->funcs[0].close_object = close_object;
            p->funcs[1].close_object = close_object;
            p->funcs[2].close_object = close_object;
        }
        if (rb_respond_to(value, oj_array_end_id)) {
            p->funcs[0].close_array = close_array;
            p->funcs[1].close_array = close_array;
            p->funcs[2].close_array = close_array;
        }
        if (rb_respond_to(value, oj_add_value_id)) {
            p->funcs[0].add_null = add_null;
            p->funcs[1].add_null = add_null;
            p->funcs[2].add_null = add_null_key;

            p->funcs[0].add_true = add_true;
            p->funcs[1].add_true = add_true;
            p->funcs[2].add_true = add_true_key;

            p->funcs[0].add_false = add_false;
            p->funcs[1].add_false = add_false;
            p->funcs[2].add_false = add_false_key;

            p->funcs[0].add_int = add_int;
            p->funcs[1].add_int = add_int;
            p->funcs[2].add_int = add_int_key;

            p->funcs[0].add_float = add_float;
            p->funcs[1].add_float = add_float;
            p->funcs[2].add_float = add_float_key;

            p->funcs[0].add_big = add_big;
            p->funcs[1].add_big = add_big;
            p->funcs[2].add_big = add_big_key;

            p->funcs[0].add_str = add_str;
            p->funcs[1].add_str = add_str;
            p->funcs[2].add_str = add_str_key;
        }
        return Qnil;
    }
    rb_raise(rb_eArgError, "%s is not an option for the validate delegate", key);
    return Qnil;
}

static VALUE result(struct _ojParser *p) {
    return Qnil;
}

static void start(struct _ojParser *p) {
    Delegate d = (Delegate)p->ctx;

    d->tail = d->keys;
}

static void dfree(struct _ojParser *p) {
    Delegate d = (Delegate)p->ctx;

    if (NULL != d->keys) {
        xfree(d->keys);
    }
    xfree(p->ctx);
}

static void mark(struct _ojParser *p) {
    if (NULL == p->ctx) {
        return;
    }
    Delegate d = (Delegate)p->ctx;

    if (Qnil != d->handler) {
        rb_gc_mark(d->handler);
    }
    if (!p->cache_keys) {
        for (VALUE *kp = d->keys; kp < d->tail; kp++) {
            rb_gc_mark(*kp);
        }
    }
}

void oj_set_parser_saj(ojParser p) {
    Delegate d   = ALLOC(struct _delegate);

    d->klen = 256;
    d->keys = ALLOC_N(VALUE, d->klen);
    d->tail = d->keys;
    p->ctx  = (void *)d;
    reset(p);
    p->option = option;
    p->result = result;
    p->free   = dfree;
    p->mark   = mark;
    p->start = start;
}
