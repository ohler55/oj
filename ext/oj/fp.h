// Copyright (c) 2026 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef OJ_FP_H
#define OJ_FP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Shortest round-trip representation of a finite double (Grisu2 via the
// vendored fpconv). Writes at most 32 characters plus a NUL terminator into
// buf and returns the length written. Appends ".0" to integral values. The
// caller must handle NaN and the infinities itself.
extern size_t oj_dtoa_shortest(double d, char *buf);

// Correctly rounded decimal-to-double for value = m10 * 10^e10 (vendored
// Ryu). Requires m10digits <= 17 and m10digits + e10 > -307 (no subnormals);
// the caller must fall back to strtod outside that range.
extern double oj_s2d_from_parts(uint64_t m10, int m10digits, int32_t e10, bool neg);

// Correctly rounded conversion of the number token in [str, end). Returns
// true and sets *dp on success. Returns false when the token is outside the
// fast contract (more than 17 significant digits, potential subnormals, or
// unexpected characters); the caller then falls back to a slow conversion.
extern bool oj_fast_dtod(const char *str, const char *end, double *dp);

#endif /* OJ_FP_H */
