/**
 * @file common.c
 * @brief Common code for pre-computation.
 *
 */
#include "common.h"

/**
 * @brief Bitreverse an array of length n inplace
 *
 * Hardware-Aware: Standard bit-reversal using tight loops.
 * For x64, the compiler can optimize the index calculation.
 *
 * @param src array
 * @param n length of array
 */
void bitreverse(T* src, size_t n) {
    for (size_t i = 0, j = 0; i < n; i++) {
        if (i < j) {
            T tmp = src[i];
            src[i] = src[j];
            src[j] = tmp;
        }
        for (size_t k = n >> 1; (j ^= k) < k; k >>= 1);
    }
}

/**
 * @brief Computes logarithm with arbitrary base
 *
 * Hardware-Aware: Uses the x64 BSR (Bit Scan Reverse) instruction
 * via __builtin_clz for base-2 logarithms to avoid slow floating point.
 *
 * @param x input
 * @param n base
 * @return unsigned int log_n(x)
 */
unsigned int log_base(unsigned int x, unsigned int n) {
    if (n == 2 && x > 0) {
        return (unsigned int)(31 - __builtin_clz(x));
    }
    /* Fallback for other bases or non-x64 architectures */
    double logd = log(x) / log(n);
    return (unsigned int)logd;
}
