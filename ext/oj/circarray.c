// Copyright (c) 2012 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#include "circarray.h"

#include "mem.h"

CircArray oj_circ_array_new(void) {
    CircArray ca;

    if (0 == (ca = OJ_R_ALLOC(struct _circArray))) {
        rb_raise(rb_eNoMemError, "not enough memory\n");
    }
    ca->objs = ca->obj_array;
    ca->size = sizeof(ca->obj_array) / sizeof(VALUE);
    ca->cnt  = 0;

    return ca;
}

void oj_circ_array_free(CircArray ca) {
    if (ca->objs != ca->obj_array) {
        OJ_R_FREE(ca->objs);
    }
    OJ_R_FREE(ca);
}

// Objects are numbered in encounter order starting at 1, so the only id that
// can extend the array is one past the highest already seen. Anything further
// is not a reference to an object the document can contain and must not be
// used to size an allocation.
void oj_circ_array_set(CircArray ca, VALUE obj, unsigned long id) {
    if (0 != ca && 0 < id && id <= ca->cnt + 1) {
        unsigned long i;

        if (ca->size < id) {
            unsigned long cnt = id + 512;

            if (ca->objs == ca->obj_array) {
                if (0 == (ca->objs = OJ_R_ALLOC_N(VALUE, cnt))) {
                    rb_raise(rb_eNoMemError, "not enough memory\n");
                }
                memcpy(ca->objs, ca->obj_array, sizeof(VALUE) * ca->cnt);
            } else {
                OJ_R_REALLOC_N(ca->objs, VALUE, cnt);
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
    VALUE obj = Qnil;

    if (0 != ca && 0 < id && id <= ca->cnt) {
        obj = ca->objs[id - 1];
    }
    return obj;
}
