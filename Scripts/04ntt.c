/**
 * @file 04ntt.c
 * @brief Section 2.2.4: Number-theoretic transform
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

static void fast_ntt(T* a, size_t n, T q, const T* twiddles, size_t twiddle_step) {
    bitreverse(a, n);
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len >> 1;
        size_t step = (n / len) * twiddle_step;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; j++) {
                T w = twiddles[j * step];
                T u = a[i + j];
                T v = zq_mod((T2)a[i + j + half] * w, q);
                a[i + j] = zq_mod((T2)u + v, q);
                a[i + j + half] = zq_mod((T2)u + q - v, q);
            }
        }
    }
}

int polymul_ntt(T* c, const T* a, const T* b, size_t n, T q) {
    size_t N = 1;
    while (N < 2 * n - 1) N <<= 1;

    T root = zq_primitiveRootOfUnity(N, q);
    T a_pad[N], b_pad[N], antt[N], bntt[N], cntt[N], twiddles[N], twiddles_inv[N];

    memset(a_pad, 0, N * sizeof(T));
    memset(b_pad, 0, N * sizeof(T));
    memcpy(a_pad, a, n * sizeof(T));
    memcpy(b_pad, b, n * sizeof(T));

    twiddles[0] = 1;
    for (size_t i = 1; i < N; i++) twiddles[i] = zq_pow(root, i, q);

    memcpy(antt, a_pad, N * sizeof(T));
    fast_ntt(antt, N, q, twiddles, 1);
    memcpy(bntt, b_pad, N * sizeof(T));
    fast_ntt(bntt, N, q, twiddles, 1);

    for (size_t i = 0; i < N; i++) cntt[i] = zq_mod((T2)antt[i] * bntt[i], q);

    T rootinv = zq_inverse(root, q);
    twiddles_inv[0] = 1;
    for (size_t i = 1; i < N; i++) twiddles_inv[i] = zq_pow(rootinv, i, q);
    fast_ntt(cntt, N, q, twiddles_inv, 1);

    T ninv = zq_inverse(N, q);
    for (size_t i = 0; i < 2 * n - 1; i++) c[i] = zq_mod((T2)cntt[i] * ninv, q);
    return 0;
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[n] ALIGN_MEM, b[n] ALIGN_MEM;
    T c[2 * n - 1] ALIGN_MEM;

    printf("--- NTT Multiplication Test ---\n");
    printf("Method: O(n log n) Frequency-domain transform.\n");
    printf("Step 1: Pad polynomials to power-of-2 size N.\n");
    printf("Step 2: Forward NTT transforms a, b into frequency domain.\n");
    printf("Step 3: Point-wise multiplication of coefficients.\n");
    printf("Step 4: Inverse NTT transform back to time domain.\n\n");

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
