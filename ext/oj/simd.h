#ifndef OJ_SIMD_H
#define OJ_SIMD_H

// SIMD architecture detection and configuration
// This header provides unified SIMD support across different CPU architectures

// x86/x86_64 SIMD detection
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#define HAVE_SIMD_X86 1

// SSE4.2 support (Intel Core i7+, AMD Bulldozer+)
// Enabled automatically when compiler has -msse4.2 flag
#if defined(__SSE4_2__)
#define HAVE_SIMD_SSE4_2 1
#include <nmmintrin.h>

#define SIMD_MINIMUM_THRESHOLD 6

extern void initialize_sse42(void);

static inline __m128i vector_lookup_sse42(__m128i  input,
                                                                                  __m128i *lookup_table,
                                                                                  int      tab_size) {
    // Extract high 4 bits to determine which 16-byte chunk (0-15)
    __m128i hi_index = _mm_and_si128(_mm_srli_epi32(input, 4), _mm_set1_epi8(0x0F));

    // Extract low 4 bits for index within the chunk (0-15)
    __m128i low_index = _mm_and_si128(input, _mm_set1_epi8(0x0F));

    // Perform lookups in all 16 tables
    __m128i results[16];
    for (int i = 0; i < tab_size; i++) {
        results[i] = _mm_shuffle_epi8(lookup_table[i], low_index);
    }

    // Create masks for each chunk and blend results
    __m128i final_result = _mm_setzero_si128();

    for (int i = 0; i < tab_size; i++) {
        __m128i mask          = _mm_cmpeq_epi8(hi_index, _mm_set1_epi8(i));
        __m128i masked_result = _mm_and_si128(mask, results[i]);
        final_result          = _mm_or_si128(final_result, masked_result);
    }

    return final_result;
}

#endif

// SSE2 support (fallback for older x86_64 CPUs - all x86_64 CPUs support SSE2)
#if defined(__SSE2__) && !defined(HAVE_SIMD_SSE4_2)
#define HAVE_SIMD_SSE2 1
#include <emmintrin.h>
#endif

#endif  // x86/x86_64

// ARM NEON detection
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64)
#define HAVE_SIMD_NEON 1
#define SIMD_MINIMUM_THRESHOLD 6
#include <arm_neon.h>
#endif

// Define which SIMD implementation to use (priority order: SSE4.2 > NEON > SSE2)
#if defined(HAVE_SIMD_SSE4_2)
#define HAVE_SIMD_STRING_SCAN 1
#define SIMD_TYPE "SSE4.2"
#elif defined(HAVE_SIMD_NEON)
#define HAVE_SIMD_STRING_SCAN 1
#define SIMD_TYPE "NEON"
#elif defined(HAVE_SIMD_SSE2)
#define HAVE_SIMD_STRING_SCAN 1
#define SIMD_TYPE "SSE2"
#else
#define SIMD_TYPE "none"
#endif
#endif