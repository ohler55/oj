// Copyright (c) 2021, Peter Ohler, All rights reserved.

#include "cache.h"
#include "oj.h"
#include "parser.h"

// The Usual delegate builds Ruby objects during parsing. It makes use of
// three stacks. The first is the value stack. This is where parsed values are
// placed. With the value stack the bulk creation and setting can be used
// which is significantly faster than setting Array (15x) or Hash (3x)
// elements one at a time.
//
// The second stack is the collection stack. Each element on the collection
// stack marks the start of a Hash, Array, or Object.
//
// The third stack is the key stack which is used for Hash and Object
// members. The key stack elements store the keys that could be used for
// either a Hash or Object. Since the decision on whether the parent is a Hash
// or Object can not be made until the end of the JSON object the keys remain
// as strings until just before setting the Hash or Object members.
//
// The approach taken with the usual delegate is to configure the delegate for
// the parser up front so that the various options are not checked during
// parsing and thus avoiding conditionals as much as reasonably possible in
// the more time sensitive parsing. Configuration is simply setting the
// function pointers to point to the function to be used for the selected
// option.

#define DEBUG 0

// Used to mark the start of each Hash, Array, or Object. The members point at
// positions of the start in the value stack and if not an Array into the key
// stack.
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
    struct _cache *sym_cache;

    uint8_t cache_str;
    bool    cache_keys;
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

static VALUE form_str(const char *str, size_t len) {
    return rb_str_freeze(rb_utf8_str_new(str, len));
}

static VALUE form_sym(const char *str, size_t len) {
    return rb_str_intern(rb_utf8_str_new(str, len));
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

static VALUE str_key(ojParser p, Key kp) {
    if ((size_t)kp->len < sizeof(kp->buf) - 1) {
        return rb_utf8_str_new(kp->buf, kp->len);
    }
    return rb_utf8_str_new(kp->key, kp->len);
}

static VALUE sym_key(ojParser p, Key kp) {
    if ((size_t)kp->len < sizeof(kp->buf) - 1) {
        return rb_str_intern(rb_utf8_str_new(kp->buf, kp->len));
    }
    return rb_str_intern(rb_utf8_str_new(kp->key, kp->len));
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

static void add_float_as_big(ojParser p) {
    char buf[64];

    // fails on ubuntu
    // snprintf(buf, sizeof(buf), "%Lg", p->num.dub);
    sprintf(buf, "%Lg", p->num.dub);
    push(p, rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new2(buf)));
}

static void add_float_as_big_key(ojParser p) {
    char buf[64];

    snprintf(buf, sizeof(buf), "%Lg", p->num.dub);
    push_key(p);
    push2(p, rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new2(buf)));
}

static void add_big(ojParser p) {
    push(p, rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf))));
}

static void add_big_key(ojParser p) {
    push_key(p);
    push2(p, rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf))));
}

static void add_big_as_float(ojParser p) {
    volatile VALUE big = rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf)));

    push(p, rb_funcall(big, rb_intern("to_f"), 0));
}

static void add_big_as_float_key(ojParser p) {
    volatile VALUE big = rb_funcall(rb_cObject, oj_bigdecimal_id, 1, rb_str_new(buf_str(&p->buf), buf_len(&p->buf)));

    push_key(p);
    push2(p, rb_funcall(big, rb_intern("to_f"), 0));
}

static void add_big_as_ruby(ojParser p) {
    push(p, rb_funcall(rb_str_new(buf_str(&p->buf), buf_len(&p->buf)), rb_intern("to_f"), 0));
}

static void add_big_as_ruby_key(ojParser p) {
    push_key(p);
    push2(p, rb_funcall(rb_str_new(buf_str(&p->buf), buf_len(&p->buf)), rb_intern("to_f"), 0));
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
    if (NULL != d->sym_cache) {
        cache_free(d->sym_cache);
    }
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

///// options /////////////////////////////////////////////////////////////////

// Each option is handled by a separate function and then added to an assoc
// list (struct opt}. The list is then iterated over until there is a name
// match. This is done primarily to keep each option separate and easier to
// understand instead of placing all in one large function.

struct opt {
    const char *name;
    VALUE (*func)(ojParser p, VALUE value);
};

static VALUE opt_cache_keys(ojParser p, VALUE value) {
    Delegate d = (Delegate)p->ctx;

    return d->cache_keys ? Qtrue : Qfalse;
}

static VALUE opt_cache_keys_set(ojParser p, VALUE value) {
    Delegate d = (Delegate)p->ctx;

    if (Qtrue == value) {
        d->cache_keys = true;
        d->get_key    = cache_key;
        if (NULL == d->sym_cache) {
            d->key_cache = d->str_cache;
        } else {
            d->key_cache = d->sym_cache;
        }
    } else {
        d->cache_keys = false;
        if (NULL == d->sym_cache) {
            d->get_key = str_key;
        } else {
            d->get_key = sym_key;
        }
    }
    return d->cache_keys ? Qtrue : Qfalse;
}

static VALUE opt_cache_strings(ojParser p, VALUE value) {
    Delegate d = (Delegate)p->ctx;

    return INT2NUM((int)d->cache_str);
}

static VALUE opt_cache_strings_set(ojParser p, VALUE value) {
    Delegate d     = (Delegate)p->ctx;
    int      limit = NUM2INT(value);

    if (CACHE_MAX_KEY < limit) {
        limit = CACHE_MAX_KEY;
    } else if (limit < 0) {
        limit = 0;
    }
    d->cache_str = limit;

    return INT2NUM((int)d->cache_str);
}

static VALUE opt_symbol_keys(ojParser p, VALUE value) {
    Delegate d = (Delegate)p->ctx;

    return (NULL != d->sym_cache) ? Qtrue : Qfalse;
}

static VALUE opt_symbol_keys_set(ojParser p, VALUE value) {
    Delegate d = (Delegate)p->ctx;

    if (Qtrue == value) {
        d->sym_cache = cache_create(0, form_sym, true);
        d->key_cache = d->sym_cache;
        if (!d->cache_keys) {
            d->get_key = sym_key;
        }
    } else {
        if (NULL != d->sym_cache) {
            cache_free(d->sym_cache);
            d->sym_cache = NULL;
        }
        if (!d->cache_keys) {
            d->get_key = str_key;
        }
    }
    return (NULL != d->sym_cache) ? Qtrue : Qfalse;
}

static VALUE opt_capacity(ojParser p, VALUE value) {
    Delegate d = (Delegate)p->ctx;

    return ULONG2NUM(d->vend - d->vhead);
}

static VALUE opt_capacity_set(ojParser p, VALUE value) {
    Delegate d   = (Delegate)p->ctx;
    long     cap = NUM2LONG(value);

    if (d->vend - d->vhead < cap) {
        long pos = d->vtail - d->vhead;

        REALLOC_N(d->vhead, VALUE, cap);
        d->vtail = d->vhead + pos;
        d->vend  = d->vhead + cap;
    }
    if (d->kend - d->khead < cap) {
        long pos = d->ktail - d->khead;

        REALLOC_N(d->khead, union _key, cap);
        d->ktail = d->khead + pos;
        d->kend  = d->khead + cap;
    }
    return ULONG2NUM(d->vend - d->vhead);
}

static VALUE opt_decimal(ojParser p, VALUE value) {
    if (add_float_as_big == p->funcs[TOP_FUN].add_float) {
        return ID2SYM(rb_intern("bigdecimal"));
    }
    if (add_big == p->funcs[TOP_FUN].add_big) {
        return ID2SYM(rb_intern("auto"));
    }
    if (add_big_as_float == p->funcs[TOP_FUN].add_big) {
        return ID2SYM(rb_intern("float"));
    }
    if (add_big_as_ruby == p->funcs[TOP_FUN].add_big) {
        return ID2SYM(rb_intern("ruby"));
    }
    return Qnil;
}

static VALUE opt_decimal_set(ojParser p, VALUE value) {
    const char *   mode;
    volatile VALUE s;

    switch (rb_type(value)) {
    case T_STRING: mode = rb_string_value_ptr(&value); break;
    case T_SYMBOL:
        s    = rb_sym_to_s(value);
        mode = rb_string_value_ptr(&s);
        break;
    default:
        rb_raise(rb_eTypeError,
                 "the decimal options must be a Symbol or String, not %s.",
                 rb_class2name(rb_obj_class(value)));
        break;
    }
    if (0 == strcmp("auto", mode)) {
        p->funcs[TOP_FUN].add_big      = add_big;
        p->funcs[ARRAY_FUN].add_big    = add_big;
        p->funcs[OBJECT_FUN].add_big   = add_big_key;
        p->funcs[TOP_FUN].add_float    = add_float;
        p->funcs[ARRAY_FUN].add_float  = add_float;
        p->funcs[OBJECT_FUN].add_float = add_float_key;

        return opt_decimal(p, Qnil);
    }
    if (0 == strcmp("bigdecimal", mode)) {
        p->funcs[TOP_FUN].add_big      = add_big;
        p->funcs[ARRAY_FUN].add_big    = add_big;
        p->funcs[OBJECT_FUN].add_big   = add_big_key;
        p->funcs[TOP_FUN].add_float    = add_float_as_big;
        p->funcs[ARRAY_FUN].add_float  = add_float_as_big;
        p->funcs[OBJECT_FUN].add_float = add_float_as_big_key;

        return opt_decimal(p, Qnil);
    }
    if (0 == strcmp("float", mode)) {
        p->funcs[TOP_FUN].add_big      = add_big_as_float;
        p->funcs[ARRAY_FUN].add_big    = add_big_as_float;
        p->funcs[OBJECT_FUN].add_big   = add_big_as_float_key;
        p->funcs[TOP_FUN].add_float    = add_float;
        p->funcs[ARRAY_FUN].add_float  = add_float;
        p->funcs[OBJECT_FUN].add_float = add_float_key;

        return opt_decimal(p, Qnil);
    }
    if (0 == strcmp("ruby", mode)) {
        p->funcs[TOP_FUN].add_big      = add_big_as_ruby;
        p->funcs[ARRAY_FUN].add_big    = add_big_as_ruby;
        p->funcs[OBJECT_FUN].add_big   = add_big_as_ruby_key;
        p->funcs[TOP_FUN].add_float    = add_float;
        p->funcs[ARRAY_FUN].add_float  = add_float;
        p->funcs[OBJECT_FUN].add_float = add_float_key;

        return opt_decimal(p, Qnil);
    }
    rb_raise(rb_eArgError, "%s is not a valid option for the decimal option.", mode);

    return Qnil;
}

static VALUE option(ojParser p, const char *key, VALUE value) {
    struct opt opts[] = {
        {.name = "cache_keys", .func = opt_cache_keys},
        {.name = "cache_keys=", .func = opt_cache_keys_set},
        {.name = "cache_strings", .func = opt_cache_strings},
        {.name = "cache_strings=", .func = opt_cache_strings_set},
        {.name = "symbol_keys", .func = opt_symbol_keys},
        {.name = "symbol_keys=", .func = opt_symbol_keys_set},
        {.name = "capacity", .func = opt_capacity},
        {.name = "capacity=", .func = opt_capacity_set},
        {.name = "decimal", .func = opt_decimal},
        {.name = "decimal=", .func = opt_decimal_set},
        {.name = NULL},
    };

    for (struct opt *op = opts; NULL != op->name; op++) {
        if (0 == strcmp(key, op->name)) {
            return op->func(p, value);
        }
    }
    rb_raise(rb_eArgError, "%s is not an option for the Usual delegate", key);

    return Qnil;  // Never reached due to the raise but required by the compiler.
}

///// the set up //////////////////////////////////////////////////////////////

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

    d->get_key    = cache_key;
    d->cache_keys = true;
    d->cache_str  = 6;
    d->sym_cache  = NULL;

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
