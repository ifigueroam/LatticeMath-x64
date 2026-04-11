/**
 * @file poly_api.c
 * @brief Phase 5: Implementation of high-level Poly API
 */
#define _POSIX_C_SOURCE 200112L
#include <stdio.h>
#include <stdlib.h>

#include "api.h"
#include "poly.h"
#include "zq.h"

void poly_init(Poly* p, size_t n, T q) {
    p->n = n;
    p->q = q;
    // Use posix_memalign for C99 compatibility with alignment
    void* ptr = NULL;
    if (posix_memalign(&ptr, 32, n * sizeof(T)) != 0) {
        fprintf(stderr, "FATAL: Memory allocation failed\n");
        exit(1);
    }
    p->coeffs = (T*)ptr;
    for (size_t i = 0; i < n; i++) p->coeffs[i] = 0;
}

void poly_free(Poly* p) {
    if (p->coeffs) free(p->coeffs);
}

void parse_config(int argc, char** argv, size_t* n, T* q) {
    if (argc > 1) *n = (size_t)atoi(argv[1]);
    if (argc > 2) *q = (T)atoi(argv[2]);
}
