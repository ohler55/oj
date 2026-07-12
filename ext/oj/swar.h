// Copyright (c) 2026 Peter Ohler. All rights reserved.
// Licensed under the MIT License. See LICENSE file in the project root for license details.

#ifndef OJ_SWAR_H
#define OJ_SWAR_H

#include <stdbool.h>
#include <stdint.h>
#include <string.h>

// SWAR (SIMD Within A Register) helpers for parsing eight consecutive ASCII
// digits at a time using 64-bit integer math only (no vector intrinsics, so
// this is architecture independent).
//
// oj_parse_8_digits() assumes little-endian lane order. is_8_digits() is a
// per-byte test and is therefore endian independent. On big-endian targets we
// disable the fast path entirely (OJ_SWAR_LE == 0) and callers fall back to the
// existing scalar digit loops, which keeps behaviour identical everywhere.

#if defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define OJ_SWAR_LE 0
#else
#define OJ_SWAR_LE 1
#endif

// True when all four bytes of val are ASCII digits. Used as a cheap
// entry prefilter: an eight-digit run is impossible unless the high four bytes
// (offsets 4..7) are all digits, and for any run shorter than eight digits in a
// dense array a delimiter always lands within those four bytes. Rejecting here
// avoids the wider load and is_8_digits test for the common short-integer case,
// keeping the non-firing fast path free of regression across all short lengths.
static inline bool oj_is_4_digits(uint32_t val) {
    return (((val & 0xF0F0F0F0u) | (((val + 0x06060606u) & 0xF0F0F0F0u) >> 4)) == 0x33333333u);
}

// True when all eight bytes of val (as loaded from memory) are ASCII digits
// ('0'..'9'). Bytes adjacent to the digit range such as '/' (0x2F) and ':'
// (0x3A) are correctly rejected.
static inline bool oj_is_8_digits(uint64_t val) {
    return (((val & 0xF0F0F0F0F0F0F0F0ULL) | (((val + 0x0606060606060606ULL) & 0xF0F0F0F0F0F0F0F0ULL) >> 4)) ==
            0x3333333333333333ULL);
}

// Convert eight ASCII digit bytes (little-endian lane order, i.e. as produced
// by memcpy from the input buffer) into the integer 0..99999999.
static inline uint32_t oj_parse_8_digits(uint64_t val) {
    const uint64_t mask = 0x000000FF000000FFULL;
    const uint64_t mul1 = 0x000F424000000064ULL;  // 100 + (1000000 << 32)
    const uint64_t mul2 = 0x0000271000000001ULL;  // 1 + (10000 << 32)

    val -= 0x3030303030303030ULL;
    val = (val * 10) + (val >> 8);
    val = (((val & mask) * mul1) + (((val >> 16) & mask) * mul2)) >> 32;
    return (uint32_t)val;
}

// Fold as many full eight-digit chunks as fit safely into *accum without the
// accumulator reaching `limit`, so the result is bit-identical to a scalar
// digit loop and never overflows past the caller's boundary (Fixnum/Bignum for
// Oj::Parser, int64 for Oj.load); the caller passes the appropriate limit and
// leaves the boundary/big handling to its own scalar loop. Advances and returns
// the input pointer. When dec_cnt is non-NULL it is bumped by the number of
// digits consumed after the first significant one (+8 per chunk, or +7 for the
// first chunk that carries the first significant digit), matching Oj.load's
// dec_cnt rule. Only fires on little-endian targets and never reads past end.
static inline const uint8_t *
oj_swar_accum(const uint8_t *b, const uint8_t *end, int64_t *accum, uint64_t limit, int *dec_cnt) {
#if OJ_SWAR_LE
    // Entry prefilter over the high four bytes: only engage SWAR when a long
    // run is plausible (b[4..7] all digits). This rejects short integers of any
    // length before the wider load, and the loop below stays free of per-chunk
    // prefilter cost on long runs.
    if (b + 8 <= end) {
        uint32_t hi;

        memcpy(&hi, b + 4, 4);
        if (oj_is_4_digits(hi)) {
            while (b + 8 <= end && (uint64_t)*accum < limit) {
                uint64_t v;

                memcpy(&v, b, 8);
                if (!oj_is_8_digits(v)) {
                    break;
                }
                if (NULL != dec_cnt) {
                    *dec_cnt += (0 != *accum) ? 8 : 7;
                }
                *accum = *accum * 100000000LL + (int64_t)oj_parse_8_digits(v);
                b += 8;
            }
        }
    }
#else
    (void)end;
    (void)accum;
    (void)limit;
    (void)dec_cnt;
#endif
    return b;
}

#endif /* OJ_SWAR_H */
