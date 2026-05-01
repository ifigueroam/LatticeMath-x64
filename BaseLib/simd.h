/**
 * @file simd.h
 * @brief SIMD-optimized polynomial addition and subtraction with scalar tail support.
 *
 * Helper functions for poly_add_simd and poly_sub_simd to perform coefficient-wise addition and
 * subtraction of two polynomials a and b, storing the result in c, with modulus q. Utilizes AVX2
 * intrinsics to process 16 coefficients simultaneously for improved performance, and includes a scalar
 * tail handler to safely process any remaining coefficients when n is not a multiple of 16, preventing
 * out-of-bounds access and ensuring correctness.
 *
 * Hardware-Aware: The functions are designed to be efficient and secure, making use of SIMD operations
 * while also ensuring that they correctly handle cases where the number of coefficients is not a
 * multiple of the SIMD width, thus preventing potential vulnerabilities from out-of-bounds access.
 */
#ifndef SIMD_H
#define SIMD_H

#include "common.h"
#include "zq.h"

/**
 * @brief Montgomery-Optimized Multiplication for SSE (128-bit)
 * Multiplies elements in va and vb and reduces mod q using Montgomery.
 */
static inline __m128i zq_mul_sse(__m128i va, __m128i vb, __m128i v_q) {
    __m128i v_qinv = _mm_set1_epi16(7679);

    __m128i v_prod_low = _mm_mullo_epi16(va, vb);
    __m128i v_prod_high = _mm_mulhi_epi16(va, vb);

    __m128i v_u = _mm_mullo_epi16(v_prod_low, v_qinv);
    __m128i v_t = _mm_mulhi_epi16(v_u, v_q);

    __m128i v_res = _mm_sub_epi16(v_prod_high, v_t);

    __m128i v_zero = _mm_setzero_si128();
    __m128i mask = _mm_cmpgt_epi16(v_zero, v_res);
    v_res = _mm_add_epi16(v_res, _mm_and_si128(mask, v_q));

    return v_res;
}

#if defined(__AVX2__)
#include <immintrin.h>

/**
 * @brief Unsigned Comparison of 16-bit Integers using AVX2 Intrinsics
 *
 * Helper function for poly_add_simd and poly_sub_simd to perform
 * unsigned comparison of 16-bit integers using AVX2 intrinsics.
 *
 * Hardware-Aware: This function is designed to provide an efficient way
 * to compare unsigned 16-bit integers in SIMD registers, which is essential
 * for the modulus reduction steps in polynomial addition and subtraction.
 *
 * @param a first SIMD register containing 16 unsigned 16-bit integers
 * @param b second SIMD register containing 16 unsigned 16-bit integers
 * @return __m256i a SIMD register where each 16-bit element is 0xFFFF
 * if the corresponding element in a is greater than the corresponding element in b, and 0
 *
 */
static inline __m256i _mm256_cmpgt_epu16(__m256i a, __m256i b) {
    __m256i offset = _mm256_set1_epi16(0x8000);
    return _mm256_cmpgt_epi16(_mm256_xor_si256(a, offset), _mm256_xor_si256(b, offset));
}

/**
 * @brief Hardware-Exploiting Addition with Scalar Tail Support
 *
 * Helper function for poly_add_simd to perform coefficient-wise addition of two polynomials a and b,
 * storing the result in c, with modulus q. Utilizes AVX2 intrinsics to process 16 coefficients
 * simultaneously for improved performance, and includes a scalar tail handler to safely process any
 * remaining coefficients when n is not a multiple of 16, preventing out-of-bounds access and ensuring
 * correctness.
 *
 * Hardware-Aware: The function is designed to be efficient and secure, making use of SIMD operations
 * while also ensuring that it correctly handles cases where the number of coefficients is not a multiple
 * of the SIMD width, thus preventing potential vulnerabilities from out-of-bounds access.
 *
 * @param c output polynomial
 * @param a first input polynomial
 * @param b second input polynomial
 * @param n number of coefficients in a and b
 * @param q modulus for the coefficients
 */
static inline void poly_add_simd(T* restrict c, const T* restrict a, const T* restrict b, size_t n,
                                 T q) {
    size_t i = 0;
    __m256i v_q = _mm256_set1_epi16(q);

    // Process blocks of 16
    for (; i + 15 < n; i += 16) {
        __m256i va = _mm256_load_si256((const __m256i*)&a[i]);
        __m256i vb = _mm256_load_si256((const __m256i*)&b[i]);
        __m256i vc = _mm256_add_epi16(va, vb);
        __m256i mask = _mm256_cmpgt_epu16(vc, v_q);
        vc = _mm256_sub_epi16(vc, _mm256_and_si256(mask, v_q));
        mask = _mm256_cmpeq_epi16(vc, v_q);
        vc = _mm256_sub_epi16(vc, _mm256_and_si256(mask, v_q));
        _mm256_store_si256((__m256i*)&c[i], vc);
    }

    // Scalar Tail Handler: Prevents OOB vulnerabilities
    for (; i < n; i++) {
        c[i] = zq_mod(a[i] + b[i], q);
    }
}

/**
 * @brief Hardware-Exploiting Subtraction with Scalar Tail Support
 *
 * Helper function for poly_sub_simd to perform coefficient-wise subtraction of two polynomials a and b,
 * storing the result in c, with modulus q. Utilizes AVX2 intrinsics to process 16 coefficients
 * simultaneously for improved performance, and includes a scalar tail handler to safely process any
 * remaining coefficients when n is not a multiple of 16, preventing out-of-bounds access and ensuring
 * correctness.
 *
 * Hardware-Aware: The function is designed to be efficient and secure, making use of SIMD operations
 * while also ensuring that it correctly handles cases where the number of coefficients is not a multiple
 * of the SIMD width, thus preventing potential vulnerabilities from out-of-bounds access.
 *
 * @param c output polynomial
 * @param a first input polynomial
 * @param b second input polynomial
 * @param n number of coefficients in a and b
 * @param q modulus for the coefficients
 */
static inline void poly_sub_simd(T* restrict c, const T* restrict a, const T* restrict b, size_t n,
                                 T q) {
    size_t i = 0;
    __m256i v_q = _mm256_set1_epi16(q);
    for (; i + 15 < n; i += 16) {
        __m256i va = _mm256_load_si256((const __m256i*)&a[i]);
        __m256i vb = _mm256_load_si256((const __m256i*)&b[i]);
        __m256i vc = _mm256_add_epi16(_mm256_sub_epi16(va, vb), v_q);
        __m256i mask = _mm256_cmpgt_epu16(vc, v_q);
        vc = _mm256_sub_epi16(vc, _mm256_and_si256(mask, v_q));
        mask = _mm256_cmpeq_epi16(vc, v_q);
        vc = _mm256_sub_epi16(vc, _mm256_and_si256(mask, v_q));
        _mm256_store_si256((__m256i*)&c[i], vc);
    }
    for (; i < n; i++) {
        c[i] = zq_mod(a[i] + q - b[i], q);
    }
}

/**
 * @brief Montgomery-Optimized Multiplication for AVX2
 * Multiplies elements in va and vb and reduces mod q using Montgomery.
 * Note: Returns result in Montgomery domain if vb is a standard value,
 * or normal domain if one of them is in Montgomery domain.
 */
static inline __m256i zq_mul_avx2(__m256i va, __m256i vb, __m256i v_q) {
    // Montgomery reduction for q=7681
    // Reference: Seiler (2018)
    __m256i v_qinv = _mm256_set1_epi16(7679);

    __m256i v_prod_low = _mm256_mullo_epi16(va, vb);
    __m256i v_prod_high = _mm256_mulhi_epi16(va, vb);

    __m256i v_u = _mm256_mullo_epi16(v_prod_low, v_qinv);
    __m256i v_t = _mm256_mulhi_epi16(v_u, v_q);

    __m256i v_res = _mm256_sub_epi16(v_prod_high, v_t);

    // Final check: if res < 0, res += q
    __m256i mask = _mm256_cmpgt_epi16(_mm256_setzero_si256(), v_res);
    v_res = _mm256_add_epi16(v_res, _mm256_and_si256(mask, v_q));

    return v_res;
}

#else

static inline void poly_add_simd(T* restrict c, const T* restrict a, const T* restrict b, size_t n,
                                 T q) {
    for (size_t i = 0; i < n; i++) c[i] = zq_mod(a[i] + b[i], q);
}
static inline void poly_sub_simd(T* restrict c, const T* restrict a, const T* restrict b, size_t n,
                                 T q) {
    for (size_t i = 0; i < n; i++) c[i] = zq_mod(a[i] + q - b[i], q);
}
#endif

#endif
