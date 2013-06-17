/* odd.c
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

#include "odd.h"

struct _Odd	odds[5]; // bump up if new Odd classes are added

static void
set_class(Odd odd, const char *classname) {
    const char	**np;
    ID		*idp;

    odd->classname = classname;
    odd->clen = strlen(classname);
    odd->clas = rb_const_get(rb_cObject, rb_intern(classname));
    odd->create_obj = odd->clas;
    odd->create_op = rb_intern("new");
    for (np = odd->attr_names, idp = odd->attrs; 0 != *np; np++, idp++) {
	*idp = rb_intern(*np);
    }
    *idp = 0;
}

void
oj_odd_init() {
    Odd		odd;
    const char	**np;

    odd = odds;
    // Rational
    np = odd->attr_names;
    *np++ = "numerator";
    *np++ = "denominator";
    *np = 0;
    set_class(odd, "Rational");
    odd->create_obj = rb_cObject;
    odd->create_op = rb_intern("Rational");
    odd->attr_cnt = 2;
    // Date
    odd++;
    np = odd->attr_names;
    *np++ = "year";
    *np++ = "month";
    *np++ = "day";
    *np++ = "start";
    *np++ = 0;
    set_class(odd, "Date");
    odd->attr_cnt = 4;
    // DateTime
    odd++;
    np = odd->attr_names;
    *np++ = "year";
    *np++ = "month";
    *np++ = "day";
    *np++ = "hour";
    *np++ = "min";
    *np++ = "sec";
    *np++ = "offset";
    *np++ = "start";
    *np++ = 0;
    set_class(odd, "DateTime");
    odd->attr_cnt = 8;
    // Range
    odd++;
    np = odd->attr_names;
    *np++ = "begin";
    *np++ = "end";
    *np++ = "exclude_end?";
    *np++ = 0;
    set_class(odd, "Range");
    odd->attr_cnt = 3;
    // The end. bump up the size of odds if a new class is added.
    odd++;
    odd->clas = Qundef;
}

Odd
oj_get_odd(VALUE clas) {
    Odd	odd = odds;

    for (; Qundef != odd->clas; odd++) {
	if (clas == odd->clas) {
	    return odd;
	}
    }
    return 0;
}

Odd
oj_get_oddc(const char *classname, size_t len) {
    Odd	odd = odds;

    for (; Qundef != odd->clas; odd++) {
	if (len == odd->clen && 0 == strncmp(classname, odd->classname, len)) {
	    return odd;
	}
    }
    return 0;
}

OddArgs
oj_odd_alloc_args(Odd odd) {
    OddArgs	oa = ALLOC_N(struct _OddArgs, 1);
    VALUE	*a;
    int		i;

    oa->odd = odd;
    for (i = odd->attr_cnt, a = oa->args; 0 < i; i--, a++) {
	*a = Qnil;
    }
    return oa;
}

void
oj_odd_free(OddArgs args) {
    xfree(args);
}

int
oj_odd_set_arg(OddArgs args, const char *key, size_t klen, VALUE value) {
    const char	**np;
    VALUE	*vp;
    int		i;

    for (i = args->odd->attr_cnt, np = args->odd->attr_names, vp = args->args; 0 < i; i--, np++, vp++) {
	if (0 == strncmp(key, *np, klen) && '\0' == *((*np) + klen)) {
	    *vp = value;
	    return 0;
	}
    }
    return -1;
}
