/* tp.c
 * Copyright (c) 2012, Peter Ohler
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  - Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 
 *  - Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 *  - Neither the name of Peter Ohler nor the names of its contributors may be
 *    used to endorse or promote products derived from this software without
 *    specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*** This is just a prototype. Turns out the waiting for processing is expensive so the approach is slower that just
 *** parsing with callbacks. The parsing is only 10% or less of the processing so not much gain would be expected even
 *** in the best case.
 ***/

#include <pthread.h>
#if !IS_WINDOWS
#include <sys/resource.h>  // for getrlimit() on linux
#endif
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include "oj.h"

typedef enum {
    UNDEF = 0,
    HASH_START,
    HASH_END,
    ARRAY_START,
    ARRAY_END,
    ADD_STR,
    ADD_BIG,
    ADD_FLOAT,
    ADD_FIXNUM,
    ADD_TRUE,
    ADD_FALSE,
    ADD_NIL,
    DONE
} OpType;

typedef struct _Op {
    OpType	op;
    const char	*key;
    union {
	VALUE		rval;
	double		d;
	uint64_t	i;
	const char	*s;
    };
} *Op;

typedef struct _CX {
    char	*json;
    VALUE	*cur;
    VALUE	*end;
    struct _Op	ops[4096];
    Op		ops_end;
    Op		iop;
    VALUE	stack[1024];
} *CX;

typedef struct _Wait {
    pthread_mutex_t	mutex;
    pthread_cond_t	cond;
    volatile int	waiting;
} *Wait;


static void	hash_start(void *context, const char *key);
static void	hash_end(void *context, const char *key);
static void	array_start(void *context, const char *key);
static void	array_end(void *context, const char *key);
static void	add_str(void *context, const char *str, const char *key);
static void	add_big(void *context, const char *str, const char *key);
static void	add_float(void *context, double num, const char *key);
static void	add_fixnum(void *context, int64_t num, const char *key);
static void	add_true(void *context, const char *key);
static void	add_false(void *context, const char *key);
static void	add_nil(void *context, const char *key);
static void	done(void *context);

static struct _SajCB	tcb = {
    hash_start,
    hash_end,
    array_start,
    array_end,
    add_str,
    add_big,
    add_float,
    add_fixnum,
    add_true,
    add_false,
    add_nil,
    done
};

static inline void
wakeup(Wait w) {
    if (w->waiting) {
	pthread_mutex_lock(&w->mutex);
	pthread_cond_signal(&w->cond);
	pthread_mutex_unlock(&w->mutex);
    }
}

static int
wait_init(Wait w) {
    int	err;

    w->waiting = 0;
    if (0 != (err = pthread_mutex_init(&w->mutex, 0)) ||
	0 != (err = pthread_cond_init(&w->cond, 0))) {
	return err;
    }
    return 0;
}

static void
wait_for_it(Wait w, double timeout) {
    struct timespec	done;
    struct timeval	tv;
    double		end;

    gettimeofday(&tv, 0);
    end = (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
    end += timeout;
    
    done.tv_sec = (time_t)end;
    done.tv_nsec = (long)((end - done.tv_sec) * 1000000000.0);
    pthread_mutex_lock(&w->mutex);
    w->waiting = 1;
    pthread_cond_timedwait(&w->cond, &w->mutex, &done);
    w->waiting = 0;
    pthread_mutex_unlock(&w->mutex);
}

static int
wait_destroy(Wait w) {
    int	err;
    
    if (0 != (err = pthread_mutex_destroy(&w->mutex))) {
	return err;
    }
    if (0 != (err = pthread_cond_destroy(&w->cond))) {
	return err;
    }
    return 0;
}

static void
cx_add(CX cx, VALUE obj, const char *key) {
    if (0 == cx->cur) {
	cx->cur = cx->stack;
	*cx->cur = obj;
    } else {
	if (0 != key) {
	    VALUE	ks = rb_str_new2(key);
#if HAS_ENCODING_SUPPORT
	    rb_enc_associate(ks, oj_utf8_encoding);
#endif
	    rb_hash_aset(*cx->cur, ks, obj);
	} else {
	    rb_ary_push(*cx->cur, obj);
	}
    }
}

static void
cx_push(CX cx, VALUE obj, const char *key) {
    if (0 == cx->cur) {
	cx->cur = cx->stack;
    } else {
	if (cx->end <= cx->cur) {
	    rb_raise(oj_parse_error_class, "too deeply nested");
	}
	cx_add(cx, obj, key);
	cx->cur++;
    }
    *cx->cur = obj;
}

static pthread_t	parse_thread;
static CX		job = 0;
static struct _Wait	parse_wait;
static struct _Wait	proc_wait;

static void*
parse_loop(void *data) {
    while (1) {
	if (0 == job) {
	    wait_for_it(&parse_wait, 0.1);
	}
	if (0 != job) {
	    CX	cx = job;

	    job = 0;
	    oj_saj_parse(cx->json, &tcb, cx);
	}
    }
    return 0;
}

void oj_tp_init() {
    wait_init(&parse_wait);
    wait_init(&proc_wait);
    pthread_create(&parse_thread, 0, parse_loop, (void*)job);
}

VALUE
oj_t_parse(char *json) {
    // TBD change to allocated for each thread or each call, make smaller and block parser when full
    struct _CX		cx;
    Op			op;

    cx.json = json;
    cx.cur = 0;
    cx.end = cx.stack + (sizeof(cx.stack) / sizeof(*cx.stack));
    *cx.stack = Qnil;

    cx.ops_end = cx.ops + (sizeof(cx.ops) / sizeof(*cx.ops));
    cx.iop = cx.ops;
    op = cx.ops;
    op->op = UNDEF;

#if 1
    job = &cx;
    wakeup(&parse_wait);
#else
    oj_saj_parse(cx.json, &tcb, &cx);
#endif
    for (; op < cx.ops_end; op++) {
	while (UNDEF == op->op) {
	    wait_for_it(&proc_wait, 0.0001);
	}
	switch (op->op) {
	case HASH_START:
	    cx_push(&cx, rb_hash_new(), op->key);
	    break;
	case HASH_END:
	    cx.cur--;
	    break;
	case ARRAY_START:
	    cx_push(&cx, rb_ary_new(), op->key);
	    break;
	case ARRAY_END:
	    cx.cur--;
	    break;
	case ADD_STR:
	    {
		VALUE	s;

		s = rb_str_new2(op->s);
#if HAS_ENCODING_SUPPORT
		rb_enc_associate(s, oj_utf8_encoding);
#endif
		cx_add(&cx, s, op->key);
	    }
	    break;
	case ADD_BIG:
	    cx_add(&cx, rb_funcall(oj_bigdecimal_class, oj_new_id, 1, rb_str_new2(op->s)), op->key);
	    break;
	case ADD_FLOAT:
	    cx_add(&cx, rb_float_new(op->d), op->key);
	    break;
	case ADD_FIXNUM:
	    cx_add(&cx, LONG2NUM(op->i), op->key);
	    break;
	case ADD_TRUE:
	    cx_add(&cx, Qtrue, op->key);
	    break;
	case ADD_FALSE:
	    cx_add(&cx, Qfalse, op->key);
	    break;
	case ADD_NIL:
	    cx_add(&cx, Qnil, op->key);
	    break;
	case DONE:
	    return *cx.stack;
	default:
	    // TBD raise
	    printf("*** unknown op type %d\n", op->op);
	    return Qnil;
	}
    }
    return *cx.stack;
}

inline static void
append_op(CX cx, OpType op, const char *key) {
    (cx->iop + 1)->op = UNDEF;
    cx->iop->key = key;
    cx->iop->op = op;
    wakeup(&proc_wait);
    cx->iop++;
}

static void
hash_start(void *context, const char *key) {
    append_op((CX)context, HASH_START, key);
}

static void
hash_end(void *context, const char *key) {
    append_op((CX)context, HASH_END, key);
}

static void
array_start(void *context, const char *key) {
    append_op((CX)context, ARRAY_START, key);
}

static void
array_end(void *context, const char *key) {
    append_op((CX)context, ARRAY_END, key);
}

static void
add_str(void *context, const char *str, const char *key) {
    CX	cx = (CX)context;

    cx->iop->s = str;
    append_op(cx, ADD_STR, key);
}

static void
add_big(void *context, const char *str, const char *key) {
    CX	cx = (CX)context;

    cx->iop->s = str;
    append_op(cx, ADD_BIG, key);
}

static void
add_float(void *context, double num, const char *key) {
    CX	cx = (CX)context;

    cx->iop->d = num;
    append_op(cx, ADD_FLOAT, key);
}

static void
add_fixnum(void *context, int64_t num, const char *key) {
    CX	cx = (CX)context;

    cx->iop->i = num;
    append_op(cx, ADD_FIXNUM, key);
}

static void
add_true(void *context, const char *key) {
    append_op((CX)context, ADD_TRUE, key);
}

static void
add_false(void *context, const char *key) {
    append_op((CX)context, ADD_FALSE, key);
}

static void
add_nil(void *context, const char *key) {
    append_op((CX)context, ADD_NIL, key);
}

static void
done(void *context) {
    append_op((CX)context, DONE, 0);
}

