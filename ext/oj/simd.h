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

#endif /* OJ_SIMD_H */
