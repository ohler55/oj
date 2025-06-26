#ifndef OJ_SIMD_H
#define OJ_SIMD_H

#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64)
#define HAVE_SIMD_NEON 1
#define SIMD_MINIMUM_THRESHOLD 6
#include <arm_neon.h>
#endif

#if defined(__x86_64__) && (defined(__GNUC__) || defined(__clang__))

#if defined(OJ_USE_SSE4_2)
#define SIMD_MINIMUM_THRESHOLD 6
#include <nmmintrin.h>

extern void initialize_sse42(void);

static inline __attribute__((target("sse4.2,ssse3"))) __m128i vector_lookup_sse42(__m128i input,  __m128i *lookup_table, int tab_size) {    
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
        __m128i mask = _mm_cmpeq_epi8(hi_index, _mm_set1_epi8(i));
        __m128i masked_result = _mm_and_si128(mask, results[i]);
        final_result = _mm_or_si128(final_result, masked_result);
    }
    
    return final_result;
}

#endif /* defined(HAVE_X86INTRIN_H) && defined(HAVE_TYPE___M128I) */
#endif
#endif /* OJ_SIMD_H */