#ifndef OJ_SIMD_H
#define OJ_SIMD_H

// SIMD architecture detection and configuration
// This header provides unified SIMD support across different CPU architectures

typedef enum _simd_implementation { SIMD_NONE, SIMD_NEON, SIMD_SSE2, SIMD_SSE42 } SIMD_Implementation;

SIMD_Implementation find_simd_implementation(void);

// x86/x86_64 SIMD detection
#if defined(__x86_64__) || defined(__i386__) || defined(_M_IX86) || defined(_M_X64)
#define HAVE_SIMD_X86 1

#if defined(HAVE_X86INTRIN_H)
#include <x86intrin.h>
#elif defined(HAVE_NMMINTRIN_H)
#include <nmmintrin.h>
#elif HAVE_EMMINTRIN_H
#include <emmintrin.h>
#endif

// SSE4.2 support (Intel Core i7+, AMD Bulldozer+)
// Enabled automatically when compiler has -msse4.2 flag
#if defined(__SSE4_2__)
#define HAVE_SIMD_SSE4_2 1
#endif

// SSE2 support (fallback for older x86_64 CPUs - all x86_64 CPUs support SSE2)
#if defined(__SSE2__)
#define HAVE_SIMD_SSE2 1
#endif

#endif  // x86/x86_64

// ARM NEON detection
#if defined(__ARM_NEON) || defined(__ARM_NEON__) || defined(__aarch64__) || defined(_M_ARM64)
#define HAVE_SIMD_NEON 1
#define SIMD_MINIMUM_THRESHOLD 6
#include <arm_neon.h>
#endif

#endif /* OJ_SIMD_H */
