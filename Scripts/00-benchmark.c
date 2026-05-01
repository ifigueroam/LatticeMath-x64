/**
 * @file 00-benchmark.c
 * @brief Phase 15: Robust Benchmarking Suite (Statistical Professionalization)
 *
 * Methodology: Median of 1000 iterations with 10 warm-up passes.
 * Metric: CPU Kilocycles (kCyc) - Frequency Independent.
 */
#define _GNU_SOURCE
#include <math.h>
#include <omp.h>
#include <sched.h>
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
void polymul_toom_cook(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q);
int polymul_ntt(T* c, const T* a, const T* b, size_t n, T q);
void polymul_winograd(T* c, const T* a, size_t aN, const T* b, size_t bN, T q);
void polymul_crt_polymul(T* c, const T* a, const T* b, size_t n, T q);

// Function wrappers for iterative benchmarking
typedef void (*mul_func)(T*, const T*, const T*, size_t, T);

/**
 * @brief Pins the current thread to a specific CPU core.
 */
static void pin_core(int core_id) {
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(core_id, &cpuset);
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
}

// Comparison function for qsort to find the median
static int compare_u64(const void* a, const void* b) {
    uint64_t arg1 = *(const uint64_t*)a;
    uint64_t arg2 = *(const uint64_t*)b;
    if (arg1 < arg2) return -1;
    if (arg1 > arg2) return 1;
    return 0;
}

static void toom_cook_bench_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    size_t n_padded = (n % 4 == 0) ? n : ((n / 4) + 1) * 4;
    T *a_p, *b_p, *c_p;
    posix_memalign((void**)&a_p, 32, n_padded * sizeof(T));
    posix_memalign((void**)&b_p, 32, n_padded * sizeof(T));
    posix_memalign((void**)&c_p, 32, (2 * n_padded - 1) * sizeof(T));
    memset(a_p, 0, n_padded * sizeof(T));
    memset(b_p, 0, n_padded * sizeof(T));
    memcpy(a_p, a, n * sizeof(T));
    memcpy(b_p, b, n * sizeof(T));
    polymul_toom_cook(c_p, a_p, b_p, n_padded, q);
    memcpy(c, c_p, (2 * n - 1) * sizeof(T));
    free(a_p);
    free(b_p);
    free(c_p);
}

static void winograd_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    polymul_winograd(c, a, n, b, n, q);
}

static void crt_polymul_wrapper_internal(T* c, const T* a, const T* b, size_t n, T q) {
    polymul_crt_polymul(c, a, b, n, q);
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
 * @brief Measures the performance metrics of an algorithm.
 */
static void measure_stats(mul_func func, T* c, const T* a, const T* b, size_t n, T q, double* min_k,
                          double* med_k, double* jitter) {
    uint64_t results[ITERATIONS];

    // Cache Warming (Dummy Passes)
    for (int i = 0; i < WARMUP; i++) {
        poly_reset_workspace();
        func(c, a, b, n, q);
    }

    // Timed Measurements
    for (int i = 0; i < ITERATIONS; i++) {
        poly_reset_workspace();
        uint64_t start = rdtsc_start();
        func(c, a, b, n, q);
        results[i] = rdtsc_end() - start;
    }

    qsort(results, ITERATIONS, sizeof(uint64_t), compare_u64);

    *min_k = (double)results[0] / 1000.0;
    *med_k = (double)results[ITERATIONS / 2] / 1000.0;

    double sum = 0, sum_sq = 0;
    for (int i = 0; i < ITERATIONS; i++) {
        double val = (double)results[i] / 1000.0;
        sum += val;
        sum_sq += val * val;
    }
    double mean = sum / ITERATIONS;
    *jitter = sqrt(sum_sq / ITERATIONS - mean * mean);
}

static void run_bench(size_t n, T q, int num_threads) {
    T *a, *b, *c;
    posix_memalign((void**)&a, 32, n * sizeof(T));
    posix_memalign((void**)&b, 32, n * sizeof(T));
    posix_memalign((void**)&c, 32, (2 * n - 1) * sizeof(T));
    poly_random(a, n, q);
    poly_random(b, n, q);
    omp_set_num_threads(num_threads);

    if (num_threads == 1) pin_core(0);

    printf("| %4zu | %5d |", n, num_threads);

    mul_func funcs[] = {schoolbook_wrapper_internal,      karatsuba_wrapper_internal,
                        toom_cook_bench_wrapper_internal, ntt_wrapper_internal,
                        crt_polymul_wrapper_internal,     winograd_wrapper_internal};

    for (int i = 0; i < 6; i++) {
        double min_k, med_k, jitter;
        measure_stats(funcs[i], c, a, b, n, q, &min_k, &med_k, &jitter);
        printf(" %7.1f/%7.1f (+-%5.1f) |", min_k, med_k, jitter);
    }
    printf("\n");

    free(a);
    free(b);
    free(c);
}

int main(void) {
    size_t sizes[] = {256, 512, 768, 1024};
    T q = 7681;
    int max_threads = omp_get_max_threads();

    printf("# LatticeMath-x64 Shielded Performance Benchmark (q=%d)\n", q);
    printf("Methodology: Min/Median (+-StdDev) of %d iterations. RDTSCP Serialized.\n", ITERATIONS);
    printf("Metric: CPU Kilocycles (kCyc) - Frequency Independent.\n\n");

    printf(
        "|  n   | Cores |   Schoolbook (Min/Med)    |    Karatsuba (Min/Med)    |    Toom-Cook "
        "(Min/Med)    |       NTT (Min/Med)       |    CRT-Poly (Min/Med)     |    Winograd (Min/Med)   "
        "  |\n");
    printf(
        "|      |       |    Min / Med / Jitter     |    Min / Med / Jitter     |    Min / Med / Jitter "
        "    |    Min / Med / Jitter     |    Min / Med / Jitter     |    Min / Med / Jitter     |\n");
    printf(
        "|:----:|:-----:|:-------------------------:|:-------------------------:|:----------------------"
        "---:|:-------------------------:|:-------------------------:|:-------------------------:|\n");

    for (int i = 0; i < 4; i++) {
        run_bench(sizes[i], q, 1);
        if (max_threads > 1) run_bench(sizes[i], q, max_threads);
        printf(
            "|------|-------|---------------------------|---------------------------|-------------------"
            "--------|---------------------------|---------------------------|--------------------------"
            "-|\n");
    }
    return 0;
}
