// Copyright (c) 2011 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef OJ_VAL_STACK_H
#define OJ_VAL_STACK_H

#include <stdint.h>

#include "mem.h"
#include "odd.h"
#include "ruby.h"
#ifdef HAVE_PTHREAD_MUTEX_INIT
#include <pthread.h>
#endif

#define STACK_INC 64

typedef enum {
    NEXT_NONE          = 0,
    NEXT_ARRAY_NEW     = 'a',
    NEXT_ARRAY_ELEMENT = 'e',
    NEXT_ARRAY_COMMA   = ',',
    NEXT_HASH_NEW      = 'h',
    NEXT_HASH_KEY      = 'k',
    NEXT_HASH_COLON    = ':',
    NEXT_HASH_VALUE    = 'v',
    NEXT_HASH_COMMA    = 'n',
} ValNext;

typedef struct _val {
    volatile VALUE val;
    const char    *key;
    char           karray[32];
    volatile VALUE key_val;
    const char    *classname;
    VALUE          clas;
    OddArgs        odd_args;
    size_t         klen;
    size_t         clen;
    size_t         pcnt;  // VALUEs this open hash holds in the stack pair buffer
    char           next;  // ValNext
    char           k1;    // first original character in the key
    char           kalloc;
} *Val;

#define PAIR_BASE_CNT 64

typedef struct _valStack {
    struct _val base[STACK_INC];
    VALUE       pbase[PAIR_BASE_CNT];  // initial buffer for pairs
    // Flat key/value pair buffer for hash modes that build each Hash in one
    // rb_hash_bulk_insert at the closing brace instead of one rb_hash_aset
    // per pair. Nesting is strictly LIFO so each open hash's pairs occupy
    // the top of the buffer; the per-hash count lives in its Val's clen.
    VALUE *pairs;
    size_t pcnt;  // number of live VALUEs in pairs
    size_t pend;  // capacity of pairs in VALUEs
    Val    head;  // current stack
    Val    end;   // stack end
    Val    tail;  // pointer to one past last element name on stack
#ifdef HAVE_PTHREAD_MUTEX_INIT
    pthread_mutex_t mutex;
#else
    VALUE mutex;
#endif

} *ValStack;

extern VALUE oj_stack_init(ValStack stack);

inline static int stack_empty(ValStack stack) {
    return (stack->head == stack->tail);
}

inline static void stack_cleanup(ValStack stack) {
    Val v;

    // end_hash() frees the args of a completed odd class. Anything still here
    // belongs to a parse that was abandoned part way through.
    for (v = stack->head; v < stack->tail; v++) {
        if (NULL != v->odd_args) {
            oj_odd_free(v->odd_args);
            v->odd_args = NULL;
        }
    }
    if (stack->base != stack->head) {
        OJ_R_FREE(stack->head);
        stack->head = NULL;
    }
    if (stack->pbase != stack->pairs) {
        OJ_R_FREE(stack->pairs);
        stack->pairs = NULL;
    }
    stack->pcnt = 0;
}

inline static void stack_push(ValStack stack, VALUE val, ValNext next) {
    if (stack->end <= stack->tail) {
        size_t      len  = stack->end - stack->head;
        size_t      toff = stack->tail - stack->head;
        Val         head = stack->head;
        const char *old  = (const char *)stack->head;
        size_t      i;

        // A realloc can trigger a GC so make sure it happens outside the lock
        // but lock before changing pointers.
        if (stack->base == stack->head) {
            head = OJ_R_ALLOC_N(struct _val, len + STACK_INC);
            memcpy(head, stack->base, sizeof(struct _val) * len);
        } else {
            OJ_R_REALLOC_N(head, struct _val, len + STACK_INC);
        }
        // A key short enough to be kept in the Val itself points into the
        // block that just moved.
        for (i = 0; i < toff; i++) {
            if (old <= head[i].key && head[i].key < old + sizeof(struct _val) * len) {
                head[i].key = head[i].karray;
            }
        }
#ifdef HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_lock(&stack->mutex);
#else
        rb_mutex_lock(stack->mutex);
#endif
        stack->head = head;
        stack->tail = stack->head + toff;
        stack->end  = stack->head + len + STACK_INC;
#ifdef HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_unlock(&stack->mutex);
#else
        rb_mutex_unlock(stack->mutex);
#endif
    }
    stack->tail->val       = val;
    stack->tail->next      = next;
    stack->tail->classname = NULL;
    stack->tail->clas      = Qundef;
    stack->tail->odd_args  = NULL;
    stack->tail->key       = 0;
    stack->tail->key_val   = Qundef;
    stack->tail->clen      = 0;
    stack->tail->klen      = 0;
    stack->tail->pcnt      = 0;
    stack->tail->kalloc    = 0;
    stack->tail++;
}

inline static size_t stack_size(ValStack stack) {
    return stack->tail - stack->head;
}

inline static Val stack_peek(ValStack stack) {
    if (stack->head < stack->tail) {
        return stack->tail - 1;
    }
    return 0;
}

inline static Val stack_peek_up(ValStack stack) {
    if (stack->head < stack->tail - 1) {
        return stack->tail - 2;
    }
    return 0;
}

inline static Val stack_prev(ValStack stack) {
    return stack->tail;
}

inline static VALUE stack_head_val(ValStack stack) {
    if (Qundef != stack->head->val) {
        return stack->head->val;
    }
    return Qnil;
}

inline static Val stack_pop(ValStack stack) {
    if (stack->head < stack->tail) {
        stack->tail--;
        return stack->tail;
    }
    return 0;
}

// Append one key/value pair to the pair buffer. The same GC discipline as
// stack_push: the allocation happens outside the mutex (it can trigger a GC
// which takes the mutex in stack_mark) and the buffer pointer only changes
// under the mutex.
inline static void stack_pair_push(ValStack stack, VALUE key, VALUE value) {
    if (stack->pend <= stack->pcnt + 2) {
        size_t cnt = stack->pend * 2;
        VALUE *pairs;

        if (stack->pbase == stack->pairs) {
            pairs = OJ_R_ALLOC_N(VALUE, cnt);
            memcpy(pairs, stack->pairs, sizeof(VALUE) * stack->pcnt);
        } else {
            pairs = stack->pairs;
            OJ_R_REALLOC_N(pairs, VALUE, cnt);
        }
#ifdef HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_lock(&stack->mutex);
#else
        rb_mutex_lock(stack->mutex);
#endif
        stack->pairs = pairs;
        stack->pend  = cnt;
#ifdef HAVE_PTHREAD_MUTEX_INIT
        pthread_mutex_unlock(&stack->mutex);
#else
        rb_mutex_unlock(stack->mutex);
#endif
    }
    stack->pairs[stack->pcnt]     = key;
    stack->pairs[stack->pcnt + 1] = value;
    stack->pcnt += 2;
}

extern const char *oj_stack_next_string(ValNext n);

#endif /* OJ_VAL_STACK_H */
