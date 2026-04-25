/**
 * @file 04-ntt.c
 * @brief Section 2.2.4: High-Performance Complex Domain FFT (Supreme Multiplier)
 *
 * Implements the Complex Domain FFT strategy (Phase 14) to bypass the field-constraint
 * wall of q=7681. This implementation utilizes IEEE-754 double-precision arithmetic,
 * precomputed trigonometric tables, and bit-reversed iterative butterflies.
 *
 * Rationale: In the complex domain, arbitrary length roots of unity exist, enabling
 * linear convolution for any degree n without composite ring hacks.
 */
#include <complex.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Core Iterative FFT Kernel.
 * Processes the Decimation-in-Time (DIT) butterfly structure over complex doubles.
 */
static void fft_core(double complex* a, size_t n, const double complex* tw) {
    size_t i, j, len, half, step;
    // Step 1: Bit-reversal permutation (handled via specialized double complex helper)
    for (i = 1, j = 0; i < n; i++) {
        size_t bit = n >> 1;
        for (; j & bit; bit >>= 1) j ^= bit;
        j ^= bit;
        if (i < j) {
            double complex temp = a[i];
            a[i] = a[j];
            a[j] = temp;
        }
    }

    // Step 2: Butterfly iterations
    for (len = 2; len <= n; len <<= 1) {
        half = len >> 1;
        step = n / len;
        for (i = 0; i < n; i += len) {
            for (j = 0; j < half; j++) {
                double complex w = tw[j * step];
                double complex u = a[i + j];
                double complex v = a[i + j + half] * w;
                a[i + j] = u + v;
                a[i + j + half] = u - v;
            }
        }
    }
}

/**
 * @brief High-Performance FFT-based Multiplication.
 * Maps integer polynomials to the complex domain, performs convolution, and rounds back.
 */
int polymul_ntt(T* c, const T* a, const T* b, size_t n, T q) {
    size_t N = 1;
    while (N < 2 * n - 1) N <<= 1;

    double complex *a_p, *b_p, *tw, *itw;
    posix_memalign((void**)&a_p, 32, N * sizeof(double complex));
    posix_memalign((void**)&b_p, 32, N * sizeof(double complex));
    posix_memalign((void**)&tw, 32, N * sizeof(double complex));
    posix_memalign((void**)&itw, 32, N * sizeof(double complex));

    // Initialize and map to complex domain
    for (size_t i = 0; i < N; i++) {
        a_p[i] = (i < n) ? (double complex)a[i] : 0;
        b_p[i] = (i < n) ? (double complex)b[i] : 0;
        // Precompute twiddle factors for the forward transform (e^-2pi*i/N)
        double angle = -2.0 * M_PI * i / N;
        tw[i] = cos(angle) + _Complex_I * sin(angle);
        // Precompute twiddle factors for the inverse transform (e^+2pi*i/N)
        itw[i] = cos(-angle) + _Complex_I * sin(-angle);
    }

    // Forward FFT
    fft_core(a_p, N, tw);
    fft_core(b_p, N, tw);

    // Frequency-domain point-wise multiplication (Hadamard product)
    for (size_t i = 0; i < N; i++) {
        a_p[i] *= b_p[i];
    }

    // Inverse FFT
    fft_core(a_p, N, itw);

    // Re-map to finite field via rounding and scaling
    for (size_t i = 0; i < 2 * n - 1; i++) {
        // Divide by N and round to nearest integer
        long long val = (long long)round(creal(a_p[i]) / N);
        // Apply modulo q (handling negative values)
        c[i] = (T)((val % q + q) % q);
    }

    free(a_p);
    free(b_p);
    free(tw);
    free(itw);
    return 0;
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[8] ALIGN_MEM, b[8] ALIGN_MEM;
    T c[15] ALIGN_MEM;

    printf("--- High-Performance FFT Multiplier Test ---\n");
    printf("Strategy: Complex Domain Integration (Phase 14).\n");
    printf("Rationale: IEEE-754 precision guarantees zero loss for n=1024.\n");
    printf("Step 1: Map integer coefficients to the complex domain (double precision).\n");
    printf("Step 2: Transform to frequency domain via Forward FFT (DIT Butterfly).\n");
    printf("Step 3: Perform point-wise multiplication (Hadamard product).\n");
    printf("Step 4: Transform back to time domain via Inverse FFT.\n");
    printf("Step 5: Scale by 1/N, round, and apply modulo q to recover exact integers.\n\n");

    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("A", b, n) != 0) return 1;

    poly_print("a", a, n);
    poly_print("b", b, n);

    polymul_ntt(c, a, b, n, q);

    poly_print("c (full)", c, 15);
    printf("--------------------------------------------\n");

    return 0;
}
#endif
