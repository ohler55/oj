/* gen_load.c
 * Copyright (c) 2011, Peter Ohler
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

#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "ruby.h"
#include "oj.h"

static void	add_obj(PInfo pi);
static void	end_obj(PInfo pi);
static void	add_array(PInfo pi);
static void	end_array(PInfo pi);
static void	add_key(PInfo pi, char *text);
static void	add_str(PInfo pi, char *text);
static void	add_int(PInfo pi, int64_t val);
static void	add_dub(PInfo pi, double val);
static void	add_true(PInfo pi);
static void	add_false(PInfo pi);
static void	add_nil(PInfo pi);

struct _ParseCallbacks   _oj_gen_callbacks = {
    add_obj,
    end_obj,
    add_array,
    end_array,
    add_key,
    add_str,
    add_int,
    add_dub,
    add_nil,
    add_true,
    add_false
};

ParseCallbacks   oj_gen_callbacks = &_oj_gen_callbacks;

static inline void
add_val(PInfo pi, VALUE val) {
    if (0 == pi->h) {
	pi->obj = val;
    } else if (ArrayCode == pi->h->type) {
        rb_ary_push(pi->h->obj, val);
    } else if (ObjectCode == pi->h->type) {
	// TBD
    } else {
	raise_error("expected to be in an Array or Hash", pi->str, pi->s);
    }
}

static void
add_obj(PInfo pi) {
    printf("*** add_obj\n");
}

static void
end_obj(PInfo pi) {
    printf("*** end_obj\n");
}

static void
add_array(PInfo pi) {
    VALUE	a = rb_ary_new();
    
    if (0 == pi->h) {
	pi->h = pi->helpers;
	pi->h->obj = a;
	pi->h->type = ArrayCode;
	pi->obj = a;
    } else if (ArrayCode == pi->h->type) {
        rb_ary_push(pi->h->obj, a);
	pi->h++;
	pi->h->obj = a;
	pi->h->type = ArrayCode;
    } else if (ObjectCode == pi->h->type) {
	// TBD
    } else {
	raise_error("expected to be in an Array or Hash", pi->str, pi->s);
    }
}

static void
end_array(PInfo pi) {
    if (0 == pi->h) {
	// TBD error
    } else if (pi->helpers < pi->h) {
	pi->h--;
    } else {
	pi->h = 0;
    }
}

static void
add_key(PInfo pi, char *text) {
    printf("*** add_key %s\n", text);
}

static void
add_str(PInfo pi, char *text) {
    printf("*** add_str %s\n", text);
}

static void
add_int(PInfo pi, int64_t val) {
    printf("*** add_int %lld\n", val);
}

static void
add_dub(PInfo pi, double val) {
    printf("*** add_dub %f\n", val);
}

static void
add_true(PInfo pi) {
    add_val(pi, Qtrue);
}

static void
add_false(PInfo pi) {
    add_val(pi, Qfalse);
}

static void
add_nil(PInfo pi) {
    add_val(pi, Qnil);
}
