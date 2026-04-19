/**
 * @file 05-winograd.c
 * @brief Section 3: 2-D Winograd-based polynomial multiplication (F(3x3, 3x3))
 *
 * Implements the 2-D Winograd algorithm optimized for modulus q=7681.
 * Reference: Wang et al. (2025). "An Efficient Polynomial Multiplication Accelerator..."
 *
 * STANDARDIZATION LOGIC:
 * To equalize the output with Schoolbook/NTT methods, this implementation:
 * 1. Reverses the filter polynomial (B) to transform the Winograd correlation into a convolution.
 * 2. Employs a zero-padded sliding window to capture the phase-correct start (c_0).
 * 3. Accumulates multiple 9-point kernel blocks to form the full 15-coefficient product.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

// Transformation Matrices for F(3x3, 3x3) - Constant matrices from the paper
static const int16_t MAT_MIN[5][5] = {
    {2, -1, -2, 1, 0}, {0, -2, -1, 1, 0}, {0, 2, -3, 1, 0}, {0, -1, 0, 1, 0}, {0, 2, -1, -2, 1}};

static const int16_t MAT_MK[5][3] = {{3, 0, 0}, {-3, -3, -3}, {-1, 1, -1}, {1, 2, 4}, {0, 0, 6}};

static const int16_t MAT_MOUT[3][5] = {{1, 1, 1, 1, 0}, {0, 1, -1, 2, 0}, {0, 1, 1, 4, 1}};

// Normalization constant: (L^2)^-1 mod 7681 where L=6 (the LCM of Mk denominators)
#define SCALE_INV 2347

/**
 * @brief Core 2-D Winograd Kernel for F(3x3, 3x3)
 * This function handles the matrix transformations and Hadamard product.
 * Core Math remains unchanged from the reference paper.
 */
static void winograd_kernel_3x3(T out[3][3], const T tile[5][5], const T filter[3][3], T q) {
    T t_in[5][5], t_k[5][5], t_had[5][5];
    T tmp[5][5];

    // 1. Transform Tile: D' = MIN * D * MIN^T
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int32_t sum = 0;
            for (int k = 0; k < 5; k++) sum += (int32_t)MAT_MIN[i][k] * tile[k][j];
            tmp[i][j] = zq_mod((T2)(sum % q + q), q);
        }
    }
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int32_t sum = 0;
            for (int k = 0; k < 5; k++) sum += (int32_t)tmp[i][k] * MAT_MIN[j][k];
            t_in[i][j] = zq_mod((T2)(sum % q + q), q);
        }
    }

    // 2. Transform Filter: K' = MK * K * MK^T
    T tmp_k[5][3];
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 3; j++) {
            int32_t sum = 0;
            for (int k = 0; k < 3; k++) sum += (int32_t)MAT_MK[i][k] * filter[k][j];
            tmp_k[i][j] = zq_mod((T2)(sum % q + q), q);
        }
    }
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            int32_t sum = 0;
            for (int k = 0; k < 3; k++) sum += (int32_t)tmp_k[i][k] * MAT_MK[j][k];
            t_k[i][j] = zq_mod((T2)(sum % q + q), q);
        }
    }

    // 3. Hadamard Product: t_had = t_in . t_k
    for (int i = 0; i < 5; i++) {
        for (int j = 0; j < 5; j++) {
            t_had[i][j] = zq_mod((T2)t_in[i][j] * t_k[i][j], q);
        }
    }

    // 4. Inverse Transform: Out = MOUT * t_had * MOUT^T
    T tmp_out[3][5];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 5; j++) {
            int32_t sum = 0;
            for (int k = 0; k < 5; k++) sum += (int32_t)MAT_MOUT[i][k] * t_had[k][j];
            tmp_out[i][j] = zq_mod((T2)(sum % q + q), q);
        }
    }
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            int32_t sum = 0;
            for (int k = 0; k < 5; k++) sum += (int32_t)tmp_out[i][k] * MAT_MOUT[j][k];
            T raw = zq_mod((T2)(sum % q + q), q);
            // Final normalization by (L^2)^-1 to eliminate divisions
            out[i][j] = zq_mod((T2)raw * SCALE_INV, q);
        }
    }
}

/**
 * @brief Standardized Winograd Polynomial Multiplication Wrapper
 * Orchestrates filter reversal and tiling to produce a linear convolution.
 */
void polymul_winograd(T* c, const T* a, size_t aN, const T* b, size_t bN, T q) {
    size_t outN = aN + bN - 1;
    for (size_t i = 0; i < outN; i++) c[i] = 0;

    // 1. Prepare Filter (Length 9): Reverse B to convert Winograd correlation to convolution
    T filter_1d[9] = {0};
    for (size_t i = 0; i < bN && i < 9; i++) {
        filter_1d[i] = b[bN - 1 - i];
    }
    T filter_2d[3][3];
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) filter_2d[i][j] = filter_1d[i * 3 + j];
    }

    /**
     * 2. Overlap-Add Tiling:
     * To compute the full 15-coefficient product for n=8, we use two kernel blocks.
     * Block 0 offset -7: produces c[0] to c[8]
     * Block 1 offset  2: produces c[9] to c[17] (truncated to 15)
     */
    for (int block = 0; block < 2; block++) {
        T tile_1d[17] = {0};
        int start_offset = (block == 0) ? -7 : 2;

        for (int i = 0; i < 17; i++) {
            int idx = start_offset + i;
            if (idx >= 0 && (size_t)idx < aN) {
                tile_1d[i] = a[idx];
            }
        }

        T tile_2d[5][5];
        for (int i = 0; i < 5; i++) {
            for (int j = 0; j < 5; j++) tile_2d[i][j] = tile_1d[i * 3 + j];
        }

        T res_2d[3][3];
        winograd_kernel_3x3(res_2d, tile_2d, filter_2d, q);

        // 3. Reshape and Accumulate results into standardized output
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                int c_idx = (block == 0) ? (i * 3 + j) : (9 + i * 3 + j);
                if ((size_t)c_idx < outN) {
                    c[c_idx] = res_2d[i][j];
                }
            }
        }
    }
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[8], b[8], c[15];

    printf("--- 2-D Winograd Multiplication Test ---\n");
    printf("Method: F(3x3, 3x3) 2-D Winograd Accelerator.\n");
    printf("Step 1: Reverse filter B to convert Winograd correlation into convolution.\n");
    printf("Step 2: Reshape zero-padded 1-D tile (17) and filter (9) into 2-D matrices.\n");
    printf("Step 3: Point-wise Hadamard product in Division-Free transformed domain.\n");
    printf("Step 4: Standardize output via overlap-add to equalize with Schoolbook/NTT.\n\n");

    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("A", b, n) != 0) return 1;

    poly_print("a", a, n);
    poly_print("b", b, n);

    polymul_winograd(c, a, n, b, n, q);

    poly_print("c (full product)", c, 15);
    printf("--------------------------------------\n");

    return 0;
}
#endif
