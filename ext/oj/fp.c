// Copyright (c) 2026 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

// Single translation unit for the vendored float conversion code so the
// power-of-ten tables exist only once in the extension.

#include "fp.h"

#include "vendor/ryu.h"

#ifndef JSON_DEBUG
#define JSON_DEBUG 0
#endif

#include "vendor/fpconv.c"

size_t oj_dtoa_shortest(double d, char *buf) {
    int len = fpconv_dtoa(d, buf);

    buf[len] = '\0';
    return (size_t)len;
}

// Powers of ten that are exactly representable as doubles.
static const double exact_pow10[23] = {1e0,  1e1,  1e2,  1e3,  1e4,  1e5,  1e6,  1e7,
                                       1e8,  1e9,  1e10, 1e11, 1e12, 1e13, 1e14, 1e15,
                                       1e16, 1e17, 1e18, 1e19, 1e20, 1e21, 1e22};

double oj_s2d_from_parts(uint64_t m10, int m10digits, int32_t e10, bool neg) {
    // Clinger fast path: with the mantissa below 2^53 and the power of ten
    // exactly representable, one IEEE multiply or divide rounds once and is
    // therefore already the correctly rounded result. This covers the short
    // decimals that dominate most documents at a fraction of the full Ryu
    // conversion cost.
    if (m10digits <= 15 && -22 <= e10 && e10 <= 22) {
        double d = (double)m10;

        if (e10 < 0) {
            d /= exact_pow10[-e10];
        } else {
            d *= exact_pow10[e10];
        }
        return neg ? -d : d;
    }
    return ryu_s2d_from_parts(m10, m10digits, e10, neg);
}

bool oj_fast_dtod(const char *str, const char *end, double *dp) {
    const char *s      = str;
    uint64_t    m      = 0;
    int         digits = 0;
    int32_t     e10    = 0;
    bool        neg    = false;

    if (s < end && ('-' == *s || '+' == *s)) {
        neg = ('-' == *s);
        s++;
    }
    for (; s < end && '0' <= *s && *s <= '9'; s++) {
        if (0 != m || '0' != *s) {
            if (17 <= digits) {
                return false;  // too many significant digits for Ryu
            }
            m = m * 10 + (uint64_t)(*s - '0');
            digits++;
        }
    }
    if (s < end && '.' == *s) {
        s++;
        for (; s < end && '0' <= *s && *s <= '9'; s++) {
            if (0 != m || '0' != *s) {
                if (17 <= digits) {
                    return false;
                }
                m = m * 10 + (uint64_t)(*s - '0');
                digits++;
            }
            e10--;
        }
    }
    if (s < end && ('e' == *s || 'E' == *s)) {
        int         ex   = 0;
        bool        eneg = false;
        const char *estart;

        s++;
        if (s < end && ('-' == *s || '+' == *s)) {
            eneg = ('-' == *s);
            s++;
        }
        estart = s;
        for (; s < end && '0' <= *s && *s <= '9'; s++) {
            if (ex < 100000) {  // larger exponents saturate, the value is over/underflowed anyway
                ex = ex * 10 + (*s - '0');
            }
        }
        if (estart == s) {
            return false;  // an exponent with no digits is not a valid float
        }
        e10 += eneg ? -ex : ex;
    }
    if (s != end) {
        return false;  // something in the token this scan does not handle
    }
    if (0 == m) {
        *dp = neg ? -0.0 : 0.0;
        return true;
    }
    if (digits + (int)e10 < -307) {
        return false;  // possible subnormal, Ryu misrounds near 1e-310
    }
    *dp = ryu_s2d_from_parts(m, digits, e10, neg);
    return true;
}
