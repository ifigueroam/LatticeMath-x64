/**
 * @file common.h
 * @brief Common code for pre-computation.
 * Helper functions and type definitions used across the project, particularly for polynomial operations.
 * - Defines T and T2 types based on coefficient size (16-bit or 32-bit).
 * - Provides declarations for bitreverse and log_base functions.
 *
 * Hardware-Aware: Includes intrinsics for x64 and defines alignment macros for efficient memory access.
 */
#ifndef COMMON_H
#define COMMON_H

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <time.h>

#if defined(__x86_64__) || defined(_M_X64)
#include <immintrin.h>
#define ALIGN_MEM __attribute__((aligned(32)))
#else
#define ALIGN_MEM
#endif

/**
 * @brief Returns current CPU cycle count using RDTSC for cycle-accurate benchmarking.
 */
static inline uint64_t rdtsc(void) {
#if defined(__x86_64__) || defined(_M_X64)
    unsigned int lo, hi;
    __asm__ __volatile__("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
#else
    return 0;  // Fallback for non-x86 architectures
#endif
}

/**
 * @brief Returns current time in nanoseconds for high-precision benchmarking.
 */
static inline uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

/*
 * Coefficient type selection: Default to 16-bit coefficients if neither is defined.
...
 * This allows flexibility in choosing between 16-bit and 32-bit coefficients based
 * on the application's needs.
 */
#if !defined(_16BIT_COEFFICIENTS) && !defined(_32BIT_COEFFICIENTS)
#define _16BIT_COEFFICIENTS
#pragma message("default to 16-bit coefficients")
#endif

#if defined(_16BIT_COEFFICIENTS)
typedef uint16_t T;
typedef uint32_t T2;
#elif defined(_32BIT_COEFFICIENTS)
typedef uint32_t T;
typedef uint64_t T2;
#endif

/**
 * @brief Bitreverse an array of length n inplace
 * Helper function for NTT operations, reorders elements in bit-reversed order.
 *
 * Needed for the Cooley-Tukey NTT algorithm, which requires input data to be in bit-reversed
 * order for efficient computation.
 *
 * Hardware-Aware: Can be optimized using SIMD intrinsics for larger arrays on x64 architectures.
 * @param src array
 * @param n length of array
 */
void bitreverse(T* src, size_t n);

/**
 * @brief Computes logarithm with arbitrary base
 * Helper function for calculating the number of stages in NTT based on the polynomial degree.
 *
 * Needed for determining the depth of the NTT computation, which is essential for performance tuning.
 *
 * Hardware-Aware: Can be optimized using compiler built-ins or intrinsics for logarithm
 * calculations on x64 architectures.
 * @param x input
 * @param n base
 * @return unsigned int log_n(x)
 */
unsigned int log_base(unsigned int x, unsigned int n);

#endif
