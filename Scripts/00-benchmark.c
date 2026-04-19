/**
 * @file 00-benchmark.c
 * @brief High-performance benchmarking for LatticeMath-x64.
 * Evaluates O(n^2), O(n^1.58), O(n^1.46), and O(n log n) algorithms.
 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

// Internal Prototypes
void polymul_schoolbook_parallel(T* c, const T* a, size_t n, const T* b, T q);
void toom3_bench_wrapper(T* c, const T* a, const T* b, size_t n, T q);
void winograd_bench_wrapper(T* c, const T* a, const T* b, size_t n, T q);
void run_bench(size_t n, T q, int num_threads);

// External Algorithm Prototypes
void polymul_karatsuba_recursive(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q,
                                 size_t threshold);
void polymul_toom3(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q);
int polymul_ntt(T* c, const T* a, const T* b, size_t n, T q);
void polymul_winograd(T* c, const T* a, size_t aN, const T* b, size_t bN, T q);

void polymul_schoolbook_parallel(T* c, const T* a, size_t n, const T* b, T q) {
    size_t i, j;
    uint32_t* c32 = calloc(2 * n - 1, sizeof(uint32_t));
#pragma omp parallel for private(j) schedule(dynamic)
    for (i = 0; i < n; i++) {
        uint32_t ai = (uint32_t)a[i];
        for (j = 0; j < n; j++) {
            uint32_t prod = ai * (uint32_t)b[j];
#pragma omp atomic
            c32[i + j] += prod;
        }
    }
    for (i = 0; i < 2 * n - 1; i++) c[i] = zq_mod(c32[i], q);
    free(c32);
}

void toom3_bench_wrapper(T* c, const T* a, const T* b, size_t n, T q) {
    size_t n_padded = (n % 3 == 0) ? n : ((n / 3) + 1) * 3;
    T *a_p, *b_p, *c_p;
    posix_memalign((void**)&a_p, 32, n_padded * sizeof(T));
    posix_memalign((void**)&b_p, 32, n_padded * sizeof(T));
    posix_memalign((void**)&c_p, 32, (2 * n_padded - 1) * sizeof(T));
    memset(a_p, 0, n_padded * sizeof(T));
    memset(b_p, 0, n_padded * sizeof(T));
    memcpy(a_p, a, n * sizeof(T));
    memcpy(b_p, b, n * sizeof(T));
    polymul_toom3(c_p, a_p, b_p, n_padded, q);
    memcpy(c, c_p, (2 * n - 1) * sizeof(T));
    free(a_p);
    free(b_p);
    free(c_p);
}

void winograd_bench_wrapper(T* c, const T* a, const T* b, size_t n, T q) {
    polymul_winograd(c, a, n, b, n, q);
}

void run_bench(size_t n, T q, int num_threads) {
    T *a, *b, *c;
    posix_memalign((void**)&a, 32, n * sizeof(T));
    posix_memalign((void**)&b, 32, n * sizeof(T));
    posix_memalign((void**)&c, 32, (2 * n - 1) * sizeof(T));
    poly_random(a, n, q);
    poly_random(b, n, q);
    omp_set_num_threads(num_threads);

    printf("| %4zu | %5d |", n, num_threads);

    uint64_t start = get_time_ns();
    if (num_threads == 1)
        poly_polymul_ref(c, a, n, b, n, q);
    else
        polymul_schoolbook_parallel(c, a, n, b, q);
    printf(" %15.2f |", (double)(get_time_ns() - start) / 1000.0);

    poly_reset_workspace();
    start = get_time_ns();
    polymul_karatsuba_recursive(c, a, b, n, q, 32);
    printf(" %14.2f |", (double)(get_time_ns() - start) / 1000.0);

    poly_reset_workspace();
    start = get_time_ns();
    toom3_bench_wrapper(c, a, b, n, q);
    printf(" %13.2f |", (double)(get_time_ns() - start) / 1000.0);

    start = get_time_ns();
    winograd_bench_wrapper(c, a, b, n, q);
    printf(" %13.2f |", (double)(get_time_ns() - start) / 1000.0);

    start = get_time_ns();
    polymul_ntt(c, a, b, n, q);
    printf(" %11.2f |\n", (double)(get_time_ns() - start) / 1000.0);

    free(a);
    free(b);
    free(c);
}

int main(void) {
    size_t sizes[] = {256, 512, 768, 1024};
    T q = 7681;
    int max_threads = omp_get_max_threads();
    printf("# LatticeMath-x64 Performance Benchmark (q=%d)\n\n", q);
    printf(
        "|  n   | Cores | Schoolbook (us) | Karatsuba (us) | Toom-3 (us)   | Winograd (us) |  NTT (us)  "
        " |\n");
    printf(
        "|:----:|:-----:|:---------------:|:--------------:|:-------------:|:-------------:|:-----------"
        ":|\n");
    for (int i = 0; i < 4; i++) {
        run_bench(sizes[i], q, 1);
        if (max_threads > 1) run_bench(sizes[i], q, max_threads);
        printf(
            "|------|-------|-----------------|----------------|---------------|---------------|--------"
            "-----|\n");
    }
    return 0;
}
