// Copyright (c) 2011 Peter Ohler. All rights reserved.

#ifndef OJ_ENCODE_H
#define OJ_ENCODE_H

#include "ruby.h"
#include "ruby/encoding.h"

#include "oj.h"

static inline VALUE
oj_encode(VALUE rstr) {
    rb_enc_associate(rstr, oj_utf8_encoding);
    return rstr;
}

#endif /* OJ_ENCODE_H */
