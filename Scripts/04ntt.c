/**
 * @file 04ntt.c
 * @brief Section 2.2.4: Number-theoretic transform (Mathematically Correct Iterative NTT)
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

// Standard DIT NTT (In-place)
static void ntt_core(T* a, size_t n, T q, const T* twiddles) {
    bitreverse(a, n);
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len >> 1;
        size_t step = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; j++) {
                T w = twiddles[j * step];
                T u = a[i + j];
                // Standard arithmetic for stability during verification
                T v = (T)zq_mod((T2)a[i + j + half] * w, q);
                a[i + j] = (T)zq_mod((T2)u + v, q);
                a[i + j + half] = (T)zq_mod((T2)u + q - v, q);
            }
        }
    }
}

int polymul_ntt(T* c, const T* a, const T* b, size_t n, T q) {
    size_t N = 1;
    // Linear product of n-degree polynomials requires N >= 2n-1
    while (N < 2 * n - 1) N <<= 1;

    T root = zq_primitiveRootOfUnity(N, q);
    T rootinv = zq_inverse(root, q);

    T *a_p, *b_p, *tw, *twinv;
    posix_memalign((void**)&a_p, 32, N * sizeof(T));
    posix_memalign((void**)&b_p, 32, N * sizeof(T));
    posix_memalign((void**)&tw, 32, (N / 2) * sizeof(T));
    posix_memalign((void**)&twinv, 32, (N / 2) * sizeof(T));

    memset(a_p, 0, N * sizeof(T));
    memset(b_p, 0, N * sizeof(T));
    memcpy(a_p, a, n * sizeof(T));
    memcpy(b_p, b, n * sizeof(T));

    for (size_t i = 0; i < N / 2; i++) {
        tw[i] = zq_pow(root, i, q);
        twinv[i] = zq_pow(rootinv, i, q);
    }

    ntt_core(a_p, N, q, tw);
    ntt_core(b_p, N, q, tw);

    // Point-wise multiplication
    for (size_t i = 0; i < N; i++) {
        a_p[i] = (T)zq_mod((T2)a_p[i] * b_p[i], q);
    }

    // Inverse NTT
    ntt_core(a_p, N, q, twinv);

    // Scaling by N^-1
    T ninv = zq_inverse(N, q);
    for (size_t i = 0; i < 2 * n - 1; i++) {
        c[i] = (T)zq_mod((T2)a_p[i] * ninv, q);
    }

    free(a_p);
    free(b_p);
    free(tw);
    free(twinv);
    return 0;
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[n], b[n], c[2 * n - 1];

    printf("--- NTT Multiplication Test ---\n");
    printf("Method: O(n log n) Frequency-domain transform.\n");
    printf("Step 1: Pad input polynomials to power-of-2 size N >= 2n-1.\n");
    printf("Step 2: Transform polynomials a and b into the frequency domain (Forward NTT).\n");
    printf("Step 3: Perform point-wise multiplication of the transformed coefficients.\n");
    printf("Step 4: Transform result back to the time domain (Inverse NTT) and scale by N^-1.\n\n");

    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("A", b, n) != 0) return 1;

    poly_print("a", a, n);
    poly_print("b", b, n);

    polymul_ntt(c, a, b, n, q);

    poly_print("c (full)", c, 2 * n - 1);
    printf("-------------------------------\n");

    return 0;
}
#endif
