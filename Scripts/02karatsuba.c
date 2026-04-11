/**
 * @file 02karatsuba.c
 * @brief Section 2.2.2: Karatsuba multiplication
 */
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "poly.h"
#include "simd.h"
#include "zq.h"

void polymul_karatsuba_recursive(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q,
                                 size_t threshold) {
    size_t nhalf = n >> 1;
    size_t i;

    if (n <= threshold) {
        poly_polymul_ref(c, a, n, b, n, q);
        return;
    }

    size_t tmp_size = 2 * (n - nhalf) - 1;
    T* tmp = poly_get_workspace(tmp_size);
    T* a0a1 = poly_get_workspace(nhalf);
    T* b0b1 = poly_get_workspace(nhalf);

    const T* a0 = &a[0];
    const T* a1 = &a[nhalf];
    const T* b0 = &b[0];
    const T* b1 = &b[nhalf];

    if (nhalf % 16 == 0) {
        poly_add_simd(a0a1, a0, a1, nhalf, q);
        poly_add_simd(b0b1, b0, b1, nhalf, q);
    } else {
        for (i = 0; i < nhalf; i++) {
            a0a1[i] = zq_mod(a0[i] + a1[i], q);
            b0b1[i] = zq_mod(b0[i] + b1[i], q);
        }
    }

    polymul_karatsuba_recursive(tmp, a0a1, b0b1, nhalf, q, threshold);
    polymul_karatsuba_recursive(&c[0], a0, b0, nhalf, q, threshold);
    polymul_karatsuba_recursive(&c[2 * nhalf], a1, b1, nhalf, q, threshold);

    c[2 * nhalf - 1] = 0;
    for (i = 0; i < 2 * nhalf - 1; i++) {
        tmp[i] = zq_mod(tmp[i] + q - c[i], q);
        tmp[i] = zq_mod(tmp[i] + q - c[2 * nhalf + i], q);
    }
    for (i = 0; i < 2 * nhalf - 1; i++) {
        c[i + nhalf] = zq_mod(c[i + nhalf] + tmp[i], q);
    }

    poly_release_workspace(nhalf);
    poly_release_workspace(nhalf);
    poly_release_workspace(tmp_size);
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    size_t threshold = 2;
    T a[n] ALIGN_MEM, b[n] ALIGN_MEM;
    T c[2 * n - 1] ALIGN_MEM;

    printf("--- Karatsuba Multiplication Test ---\n");
    printf("Method: O(n^1.58) Divide-and-Conquer.\n");
    printf("Step 1: Split a into a0, a1 and b into b0, b1.\n");
    printf("Step 2: Compute three sub-products: c0 = a0*b0, c2 = a1*b1, and c_mid = (a0+a1)*(b0+b1).\n");
    printf("Step 3: Combine: c_mid - c0 - c2 is the middle term of the final product.\n\n");

    poly_reset_workspace();
    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("A", b, n) != 0) return 1;

    poly_print("a", a, n);
    poly_print("b", b, n);

    polymul_karatsuba_recursive(c, a, b, n, q, threshold);

    poly_print("c (full)", c, 2 * n - 1);
    printf("--------------------------------------\n");

    return 0;
}
#endif
