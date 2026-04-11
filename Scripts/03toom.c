/**
 * @file 03toom.c
 * @brief Section 2.2.3: Toom--Cook multiplication (Synchronized to n=9 padding)
 */
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

static void polymul_toom3(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q) {
    if (n % 3 != 0) return;
    if (q % 3 == 0) return;

    T inv2 = zq_inverse(2, q);
    T2 qq = q;
    if (inv2 == 0) qq = 2 * q;

    T inv3 = zq_inverse(3, qq);
    size_t nsplit = n / 3;

    T* a_1 = poly_get_workspace(nsplit);
    T* a_m1 = poly_get_workspace(nsplit);
    T* a_m2 = poly_get_workspace(nsplit);
    T* b_1 = poly_get_workspace(nsplit);
    T* b_m1 = poly_get_workspace(nsplit);
    T* b_m2 = poly_get_workspace(nsplit);

    const T* a0 = &a[0 * nsplit];
    const T* a1 = &a[1 * nsplit];
    const T* a2 = &a[2 * nsplit];
    const T* b0 = &b[0 * nsplit];
    const T* b1 = &b[1 * nsplit];
    const T* b2 = &b[2 * nsplit];

    for (size_t i = 0; i < nsplit; i++) {
        a_1[i] = zq_mod(a0[i] + a2[i], qq);
        a_m1[i] = zq_mod(a_1[i] + qq - a1[i], qq);
        a_1[i] = zq_mod(a_1[i] + a1[i], qq);
        a_m2[i] = zq_mod((a0[i] + 2 * qq - 2 * a1[i] + 4 * a2[i]), qq);

        b_1[i] = zq_mod(b0[i] + b2[i], qq);
        b_m1[i] = zq_mod(b_1[i] + qq - b1[i], qq);
        b_1[i] = zq_mod(b_1[i] + b1[i], qq);
        b_m2[i] = zq_mod(b0[i] + 2 * qq - 2 * b1[i] + 4 * b2[i], qq);
    }

    size_t prod_size = 2 * nsplit - 1;
    T* c_0 = poly_get_workspace(prod_size);
    T* c_1 = poly_get_workspace(prod_size);
    T* c_m1 = poly_get_workspace(prod_size);
    T* c_m2 = poly_get_workspace(prod_size);
    T* c_inf = poly_get_workspace(prod_size);

    poly_polymul_ref(c_0, a0, nsplit, b0, nsplit, qq);
    poly_polymul_ref(c_inf, a2, nsplit, b2, nsplit, qq);
    poly_polymul_ref(c_1, a_1, nsplit, b_1, nsplit, qq);
    poly_polymul_ref(c_m1, a_m1, nsplit, b_m1, nsplit, qq);
    poly_polymul_ref(c_m2, a_m2, nsplit, b_m2, nsplit, qq);

    for (size_t i = 0; i < 2 * n - 1; i++) c[i] = 0;

    T v1, v2, v3;
    for (size_t i = 0; i < prod_size; i++) {
        v3 = zq_mod((c_m2[i] + qq - c_1[i]) * inv3, qq);
        if (inv2)
            v1 = zq_mod((c_1[i] + qq - c_m1[i]) * inv2, qq);
        else
            v1 = zq_mod((c_1[i] + qq - c_m1[i]) >> 1, qq);
        v2 = zq_mod(c_m1[i] + qq - c_0[i], qq);
        if (inv2)
            v3 = zq_mod((v2 + qq - v3) * inv2 + 2 * c_inf[i], qq);
        else
            v3 = zq_mod(((v2 + qq - v3) >> 1) + 2 * c_inf[i], qq);
        v2 = zq_mod(v2 + v1 + qq - c_inf[i], qq);
        v1 = zq_mod(v1 + qq - v3, qq);

        c[0 * nsplit + i] = zq_mod(c[0 * nsplit + i] + c_0[i], q);
        c[1 * nsplit + i] = zq_mod(c[1 * nsplit + i] + v1, q);
        c[2 * nsplit + i] = zq_mod(c[2 * nsplit + i] + v2, q);
        c[3 * nsplit + i] = zq_mod(c[3 * nsplit + i] + v3, q);
        c[4 * nsplit + i] = zq_mod(c[4 * nsplit + i] + c_inf[i], q);
    }

    poly_release_workspace(prod_size * 5);
    poly_release_workspace(nsplit * 6);
}

int main(void) {
    size_t n = 9;  // Synchronized to multiple of 3
    T q = 7681;
    T a[n] ALIGN_MEM, b[n] ALIGN_MEM;
    T c[2 * n - 1] ALIGN_MEM;

    printf("--- Toom-3 Multiplication Test (Linear, n=9) ---\n");
    memset(a, 0, n * sizeof(T));
    memset(b, 0, n * sizeof(T));
    if (poly_load("A", a, 8) != 0) return 1;  // Load your 8 coeffs into 9 spots
    if (poly_load("A", b, 8) != 0) return 1;

    poly_print("a", a, n);
    poly_print("b", b, n);

    poly_reset_workspace();
    polymul_toom3(c, a, b, n, q);

    poly_print("c (full)", c, 2 * n - 1);
    printf("------------------------------------------------\n");

    return 0;
}
