#ifndef OJ_SIMD_H
#define OJ_SIMD_H

// SIMD architecture detection and configuration
// This header provides unified SIMD support across different CPU architectures
// with cross-platform runtime detection (Windows/Linux/Mac)

// SIMD implementation enum - used for runtime selection
typedef enum _simd_implementation {
    SIMD_NONE,
    SIMD_NEON,
    SIMD_SSE2,
    SIMD_SSE42
} SIMD_Implementation;

// Runtime CPU detection function (implemented in oj.c)
SIMD_Implementation oj_get_simd_implementation(void);

// =============================================================================
// Compiler compatibility macros
// =============================================================================

// Branch prediction hints
#if defined(__GNUC__) || defined(__clang__)
#define OJ_LIKELY(x)   __builtin_expect(!!(x), 1)
#define OJ_UNLIKELY(x) __builtin_expect(!!(x), 0)
#else
#define OJ_LIKELY(x)   (x)
#define OJ_UNLIKELY(x) (x)
#endif

// Prefetch hints
#if defined(__GNUC__) || defined(__clang__)
#define OJ_PREFETCH(addr) __builtin_prefetch(addr, 0, 0)
#elif defined(_MSC_VER)
#include <intrin.h>
#define OJ_PREFETCH(addr) _mm_prefetch((const char *)(addr), _MM_HINT_T0)
#else
#define OJ_PREFETCH(addr) ((void)0)
#endif

// Count trailing zeros (for SSE2 mask scanning)
#if defined(__GNUC__) || defined(__clang__)
#define OJ_CTZ(x) __builtin_ctz(x)
#elif defined(_MSC_VER)
#include <intrin.h>
static __inline int oj_ctz_msvc(unsigned int x) {
    unsigned long index;
    _BitScanForward(&index, x);
    return (int)index;
}
#define OJ_CTZ(x) oj_ctz_msvc(x)
#else
// Fallback: naive implementation
static inline int oj_ctz_fallback(unsigned int x) {
    int count = 0;
    while ((x & 1) == 0 && count < 32) {
        x >>= 1;
        count++;
    }
    return count;
}
#define OJ_CTZ(x) oj_ctz_fallback(x)
#endif

// =============================================================================
// x86/x86_64 SIMD detection
// =============================================================================
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#define HAVE_SIMD_X86 1

// Include appropriate SIMD headers
#if defined(_MSC_VER)
// MSVC: use intrin.h for all intrinsics
#include <intrin.h>
#define HAVE_SIMD_SSE4_2 1
#define HAVE_SIMD_SSE2 1
#elif defined(__GNUC__) || defined(__clang__)
// GCC/Clang: check for header availability and include them
// We include headers but use target attributes to enable instructions per-function
#if defined(__SSE4_2__) || defined(__SSE2__)
// If any SSE is enabled globally, x86intrin.h should be available
#include <x86intrin.h>
#define HAVE_SIMD_SSE4_2 1
#define HAVE_SIMD_SSE2 1
#else
// Try to include headers anyway for target attribute functions
#if __has_include(<x86intrin.h>)
#include <x86intrin.h>
#define HAVE_SIMD_SSE4_2 1
#define HAVE_SIMD_SSE2 1
#elif __has_include(<nmmintrin.h>)
#include <nmmintrin.h>
#define HAVE_SIMD_SSE4_2 1
#define HAVE_SIMD_SSE2 1
#elif __has_include(<emmintrin.h>)
#include <emmintrin.h>
#define HAVE_SIMD_SSE2 1
#endif
#endif
#endif

// Target attribute macros for function-level SIMD enabling
#if defined(__clang__) || defined(__GNUC__)
#define OJ_TARGET_SSE42 __attribute__((target("sse4.2")))
#define OJ_TARGET_SSE2  __attribute__((target("sse2")))
#else
// MSVC doesn't need target attributes - intrinsics are always available
#define OJ_TARGET_SSE42
#define OJ_TARGET_SSE2
#endif

#endif  // x86/x86_64

// =============================================================================
// ARM NEON detection
// =============================================================================
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64)
#define HAVE_SIMD_NEON 1
#define SIMD_MINIMUM_THRESHOLD 6
#include <arm_neon.h>
#endif

// =============================================================================
// SIMD type string for debugging/logging
// =============================================================================
#if defined(HAVE_SIMD_SSE4_2) || defined(HAVE_SIMD_SSE2)
#define HAVE_SIMD_STRING_SCAN 1
#define SIMD_TYPE "x86 (runtime detected)"
#elif defined(HAVE_SIMD_NEON)
#define HAVE_SIMD_STRING_SCAN 1
#define SIMD_TYPE "NEON"
#else
#define SIMD_TYPE "none"
#endif

#endif /* OJ_SIMD_H */
