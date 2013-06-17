/* circarray.c
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

#include "circarray.h"

CircArray
oj_circ_array_new() {
    CircArray	ca;
    
    if (0 == (ca = ALLOC(struct _CircArray))) {
	rb_raise(rb_eNoMemError, "not enough memory\n");
    }
    ca->objs = ca->obj_array;
    ca->size = sizeof(ca->obj_array) / sizeof(VALUE);
    ca->cnt = 0;
    
    return ca;
}

void
oj_circ_array_free(CircArray ca) {
    if (ca->objs != ca->obj_array) {
	xfree(ca->objs);
    }
    xfree(ca);
}

void
oj_circ_array_set(CircArray ca, VALUE obj, unsigned long id) {
    if (0 < id && 0 != ca) {
	unsigned long	i;

	if (ca->size < id) {
	    unsigned long	cnt = id + 512;

	    if (ca->objs == ca->obj_array) {
		if (0 == (ca->objs = ALLOC_N(VALUE, cnt))) {
		    rb_raise(rb_eNoMemError, "not enough memory\n");
		}
		memcpy(ca->objs, ca->obj_array, sizeof(VALUE) * ca->cnt);
	    } else { 
		REALLOC_N(ca->objs, VALUE, cnt);
	    }
	    ca->size = cnt;
	}
	id--;
	for (i = ca->cnt; i < id; i++) {
	    ca->objs[i] = Qnil;
	}
	ca->objs[id] = obj;
	if (ca->cnt <= id) {
	    ca->cnt = id + 1;
	}
    }
}

VALUE
oj_circ_array_get(CircArray ca, unsigned long id) {
    VALUE	obj = Qnil;

    if (id <= ca->cnt && 0 != ca) {
	obj = ca->objs[id - 1];
    }
    return obj;
}

