/**
 * @file 00-benchmark.c
 * @brief Phase 15: Robust Benchmarking Suite (Statistical Professionalization)
 *
 * Methodology: Median of 1000 iterations with 10 warm-up passes.
 * Metric: CPU Kilocycles (kCyc) - Frequency Independent.
 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "zq.h"

#define ITERATIONS 1000
#define WARMUP 10

// External Algorithm Prototypes
void polymul_karatsuba_recursive(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q,
                                 size_t threshold);
void polymul_toom3(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q);
int polymul_ntt(T* c, const T* a, const T* b, size_t n, T q);
void polymul_winograd(T* c, const T* a, size_t aN, const T* b, size_t bN, T q);

// Function wrappers for iterative benchmarking
typedef void (*mul_func)(T*, const T*, const T*, size_t, T);

// Comparison function for qsort to find the median
static int compare_u64(const void* a, const void* b) {
    uint64_t arg1 = *(const uint64_t*)a;
    uint64_t arg2 = *(const uint64_t*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

static void toom4_bench_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    size_t n_padded = (n % 4 == 0) ? n : ((n / 4) + 1) * 4;
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

static void winograd_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    polymul_winograd(c, a, n, b, n, q);
}

static void ntt_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    polymul_ntt(c, a, b, n, q);
}

static void karatsuba_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    polymul_karatsuba_recursive(c, a, b, n, q, 32);
}

static void schoolbook_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    poly_polymul_ref(c, a, n, b, n, q);
}

/**
 * @brief Measures the median CPU cycles of an algorithm.
 */
static uint64_t measure_median(mul_func func, T* c, const T* a, const T* b, size_t n, T q) {
    uint64_t results[ITERATIONS];

    // Cache Warming (Dummy Passes)
    for (int i = 0; i < WARMUP; i++) {
        poly_reset_workspace();
        func(c, a, b, n, q);
    }

    // Timed Measurements
    for (int i = 0; i < ITERATIONS; i++) {
        poly_reset_workspace();
        uint64_t start = rdtsc();
        func(c, a, b, n, q);
        results[i] = rdtsc() - start;
    }

    qsort(results, ITERATIONS, sizeof(uint64_t), compare_u64);
    return results[ITERATIONS / 2];
}

static void run_bench(size_t n, T q, int num_threads) {
    T *a, *b, *c;
    posix_memalign((void**)&a, 32, n * sizeof(T));
    posix_memalign((void**)&b, 32, n * sizeof(T));
    posix_memalign((void**)&c, 32, (2 * n - 1) * sizeof(T));
    poly_random(a, n, q);
    poly_random(b, n, q);
    omp_set_num_threads(num_threads);

    printf("| %4zu | %5d |", n, num_threads);

    // Symmetric Alignment: 1 space + width + 1 space
    uint64_t cyc;

    cyc = measure_median(schoolbook_wrapper_internal, c, a, b, n, q);
    printf(" %15.1f |", (double)cyc / 1000.0);

    cyc = measure_median(karatsuba_wrapper_internal, c, a, b, n, q);
    printf(" %14.1f |", (double)cyc / 1000.0);

    cyc = measure_median(toom4_bench_wrapper_internal, c, a, b, n, q);
    printf(" %11.1f |", (double)cyc / 1000.0);

    cyc = measure_median(ntt_wrapper_internal, c, a, b, n, q);
    printf(" %8.1f |", (double)cyc / 1000.0);

    cyc = measure_median(winograd_wrapper_internal, c, a, b, n, q);
    printf(" %13.1f |\n", (double)cyc / 1000.0);

    free(a);
    free(b);
    free(c);
}

int main(void) {
    size_t sizes[] = {256, 512, 768, 1024};
    T q = 7681;
    int max_threads = omp_get_max_threads();

    printf("# LatticeMath-x64 Robust Performance Benchmark (q=%d)\n", q);
    printf("Methodology: Median of %d iterations with %d warm-up passes.\n", ITERATIONS, WARMUP);
    printf("Metric: CPU Kilocycles (kCyc) - Frequency Independent.\n\n");

    printf(
        "|  n   | Cores | Schoolbook (kC) | Karatsuba (kC) | Toom-4 (kC) | NTT (kC) | Winograd (kC) "
        "|\n");
    printf(
        "|:----:|:-----:|:---------------:|:--------------:|:-----------:|:--------:|:-------------:|"
        "\n");

    for (int i = 0; i < 4; i++) {
        run_bench(sizes[i], q, 1);
        if (max_threads > 1) run_bench(sizes[i], q, max_threads);
        printf(
            "|------|-------|-----------------|----------------|-------------|----------|---------------"
            "|\n");
    }
    return 0;
}
