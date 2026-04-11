/**
 * @file 01schoolbook.c
 * @brief Section 2.2.1: Schoolbook multiplication (Synchronized to Linear Convolution)
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "poly.h"
#include "zq.h"

static void polymul_schoolbook(T* c, const T* a, size_t aN, const T* b, size_t bN, T q) {
    size_t i, j;
    uint32_t t;
    for (i = 0; i < aN + bN - 1; i++) {
        c[i] = 0;
    }

    for (i = 0; i < aN; i++) {
        for (j = 0; j < bN; j++) {
            t = zq_mod((uint32_t)a[i] * b[j], q);
            c[i + j] = zq_mod(c[i + j] + t, q);
        }
    }
}

int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[n] ALIGN_MEM, b[n] ALIGN_MEM;
    T c[2 * n - 1] ALIGN_MEM;

    printf("--- Schoolbook Multiplication Test (Linear) ---\n");
    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("A", b, n) != 0) return 1;

    poly_print("a", a, n);
    poly_print("b", b, n);

    polymul_schoolbook(c, a, n, b, n, q);

    poly_print("c (full)", c, 2 * n - 1);
    printf("----------------------------------------------\n");

    return 0;
}
