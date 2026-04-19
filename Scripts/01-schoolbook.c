/**
 * @file 01-schoolbook.c
 * @brief Section 2.2.1: Schoolbook multiplication (Optimized)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

static void polymul_schoolbook(T* c, const T* a, size_t aN, const T* b, size_t bN, T q) {
    size_t i, j, ii, jj;

    // Scientific Vanguard: Lazy Reduction & Loop Tiling
    // Deferring the modular reduction using an accumulator avoids costly modulo ops
    // in the inner loop (Alkim et al., 2016). Tiling improves L1 cache hit rates for large N.
    uint32_t* acc = calloc(aN + bN - 1, sizeof(uint32_t));
    size_t TILE = 32;

    for (ii = 0; ii < aN; ii += TILE) {
        size_t limit_i = (ii + TILE < aN) ? ii + TILE : aN;
        for (jj = 0; jj < bN; jj += TILE) {
            size_t limit_j = (jj + TILE < bN) ? jj + TILE : bN;
            for (i = ii; i < limit_i; i++) {
                uint32_t ai = a[i];
#pragma omp simd
                for (j = jj; j < limit_j; j++) {
                    acc[i + j] += ai * b[j];
                }
            }
        }
        // Partial reduction to prevent 32-bit overflow for large aN
        for (i = ii; i < ii + aN + TILE && i < aN + bN - 1; i++) {
            acc[i] = zq_mod(acc[i], q);
        }
    }

    for (i = 0; i < aN + bN - 1; i++) {
        c[i] = zq_mod(acc[i], q);
    }
    free(acc);
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[n] ALIGN_MEM, b[n] ALIGN_MEM;
    T c[2 * n - 1] ALIGN_MEM;

    printf("--- Schoolbook Multiplication Test ---\n");
    printf("Method: O(n^2) Naive with Cache Tiling & Lazy Reduction.\n");
    printf("Step 1: Divide loops into 32x32 tiles to maximize L1 cache hits.\n");
    printf("Step 2: Accumulate products into 32-bit registers without reduction (Lazy).\n");
    printf("Step 3: Perform final modular reduction into the result array.\n\n");

    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("A", b, n) != 0) return 1;

    poly_print("a", a, n);
    poly_print("b", b, n);

    polymul_schoolbook(c, a, n, b, n, q);

    poly_print("c (full)", c, 2 * n - 1);
    printf("--------------------------------------\n");

    return 0;
}
#endif
