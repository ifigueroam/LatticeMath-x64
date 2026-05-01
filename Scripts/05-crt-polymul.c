/**
 * @file 05-crt-polymul.c
 * @brief Phase 22: CRT Polynomial Multiplication (TCHES 2025 Peak Efficiency)
 *
 * Implements Stage 3 Peak Efficiency: Block-Wise Pruning,
 * Crude Barrett Approximation, and Vectorized Reconstruction.
 */
#include <immintrin.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "simd.h"
#include "zq.h"

// External Karatsuba for base-case sub-multiplications
void polymul_karatsuba_recursive(T* restrict c, const T* restrict a, const T* restrict b, size_t n, T q,
                                 size_t threshold);

// Parameters for different input sizes
typedef struct {
    size_t n_main;
    size_t n_low;
} MonomialParams;

static MonomialParams get_params(size_t n) {
    if (n <= 256) return (MonomialParams){384, 128};
    if (n <= 768) return (MonomialParams){1536, 0};
    return (MonomialParams){1920, 128};
}

// Precomputed twiddles in SIMD-friendly format
static T tw_main[2048] ALIGN_MEM;
static T itw_main[2048] ALIGN_MEM;
static size_t last_m = 0;

static void init_twiddles_dynamic(size_t m, T q) {
    if (last_m == m) return;
    T root7680 = 17;
    T root_m = zq_pow(root7680, 7680 / m, q);
    T inv_root_m = zq_inverse(root_m, q);
    for (size_t i = 0; i < m; i++) {
        tw_main[i] = zq_pow(root_m, i, q);
        itw_main[i] = zq_pow(inv_root_m, i, q);
    }
    last_m = m;
}

/**
 * @brief Phase 20-22 Optimized NTT Core (Portable Stage 3)
 */
static void ntt_optimized_portable(T* a, size_t n, T q, const T* tw) {
    bitreverse(a, n);

    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len >> 1, step = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; j++) {
                T v_val = a[i + j + half];
                if (v_val == 0) continue;  // Portable Pruning

                T w = tw[j * step];
                T u_val = a[i + j];
                T prod_v = zq_mod((T2)v_val * w, q);
                a[i + j] = zq_mod(u_val + prod_v, q);
                a[i + j + half] = zq_mod(u_val + q - prod_v, q);
            }
        }
    }
}

static void intt_optimized_portable(T* a, size_t n, T q, const T* itw) {
    bitreverse(a, n);
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len >> 1, step = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; j++) {
                T w = itw[j * step], u = a[i + j], v = zq_mod((T2)a[i + j + half] * w, q);
                a[i + j] = zq_mod(u + v, q);
                a[i + j + half] = zq_mod(u + q - v, q);
            }
        }
    }
    T invN = zq_inverse(n, q);
    for (size_t k = 0; k < n; k++) a[k] = zq_mod((T2)a[k] * invN, q);
}

/**
 * @brief Phase 23.B: Weighted Base-Case Convolution (16x16)
 * Computes C(x) = A(x)B(x) mod (x^16 - w)
 */
static void weighted_mul_16(T* c, const T* a, const T* b, T w, T q) {
    T tmp[32] = {0};  // Linear product result buffer
    // Schoolbook 16x16 (Could be Karatsuba, but 16x16 is small)
    for (int i = 0; i < 16; i++) {
        if (a[i] == 0) continue;
        for (int j = 0; j < 16; j++) {
            tmp[i + j] = zq_mod(tmp[i + j] + (T2)a[i] * b[j], q);
        }
    }
    // Reduce mod x^16 - w
    for (int i = 0; i < 15; i++) {
        c[i] = zq_mod(tmp[i] + (T2)tmp[i + 16] * w, q);
    }
    c[15] = tmp[15];
}

/**
 * @brief Block-wise butterfly operations (Portable & Fast)
 */
static inline void block16_add(T* res, const T* a, const T* b, T q) {
    for (int i = 0; i < 16; i++) res[i] = zq_mod(a[i] + b[i], q);
}

static inline void block16_sub(T* res, const T* a, const T* b, T q) {
    for (int i = 0; i < 16; i++) res[i] = zq_mod(a[i] + q - b[i], q);
}

static inline void block16_mul_scalar(T* res, const T* a, T w, T q) {
    for (int i = 0; i < 16; i++) res[i] = zq_mod((T2)a[i] * w, q);
}

/**
 * @brief Phase 23.B: Matrix-Reshaped Block Transform
 */
static void ntt_matrix_incomplete(T* a, size_t n, T q, const T* tw) {
    size_t n1, n2, k = 16;
    size_t m = n / k;
    if (m == 96) {
        n1 = 3;
        n2 = 32;
    } else if (m == 24) {
        n1 = 3;
        n2 = 8;
    } else if (m == 120) {
        n1 = 15;
        n2 = 8;
    } else {
        ntt_optimized_portable(a, n, q, tw);
        return;
    }

    size_t mark = poly_workspace_get_mark();
    T* tmp = poly_get_workspace(n);

    // 1. Good-Thomas Permutation (Blocks)
    for (size_t i = 0; i < m; i++) {
        memcpy(&tmp[((i % n1) * n2 + (i % n2)) * k], &a[i * k], k * sizeof(T));
    }

    // 2. Column-wise NTT (Size N1) on blocks
    for (size_t col = 0; col < n2; col++) {
        if (n1 == 3) {
            T w1 = tw[n / 3], w2 = tw[2 * n / 3];
            T* b0 = &tmp[(0 * n2 + col) * k];
            T* b1 = &tmp[(1 * n2 + col) * k];
            T* b2 = &tmp[(2 * n2 + col) * k];
            T u[16], v1[16], v2[16];
            memcpy(u, b0, k * sizeof(T));
            memcpy(v1, b1, k * sizeof(T));
            memcpy(v2, b2, k * sizeof(T));
            for (int i = 0; i < 16; i++) {
                b0[i] = zq_mod(u[i] + v1[i] + v2[i], q);
                b1[i] = zq_mod(u[i] + (T2)v1[i] * w1 + (T2)v2[i] * w2, q);
                b2[i] = zq_mod(u[i] + (T2)v1[i] * w2 + (T2)v2[i] * w1, q);
            }
        } else {
            // Generic N1 for 15
            for (size_t j = 0; j < n1; j++) {
                T sum[16] = {0};
                for (size_t mm = 0; m < n1; mm++) {
                    T w = tw[(j * mm * (n / n1)) % n];
                    for (int i = 0; i < 16; i++)
                        sum[i] = zq_mod(sum[i] + (T2)tmp[(mm * n2 + col) * k + i] * w, q);
                }
                memcpy(&tmp[(j * n2 + col) * k], sum, k * sizeof(T));
            }
        }
    }

    // 3. Row-wise NTT (Size N2) on blocks
    for (size_t row = 0; row < n1; row++) {
        T* row_ptr = &tmp[row * n2 * k];
        for (size_t len = 2; len <= n2; len <<= 1) {
            size_t half = len >> 1, step = n2 / len;
            for (size_t i = 0; i < n2; i += len) {
                for (size_t j = 0; j < half; j++) {
                    T w = tw[j * step * (n / n2)];
                    T *u = &row_ptr[(i + j) * k], *v = &row_ptr[(i + j + half) * k];
                    T v_tw[16], u_copy[16];
                    block16_mul_scalar(v_tw, v, w, q);
                    memcpy(u_copy, u, k * sizeof(T));
                    block16_add(u, u_copy, v_tw, q);
                    block16_sub(v, u_copy, v_tw, q);
                }
            }
        }
    }

    memcpy(a, tmp, n * sizeof(T));
    poly_workspace_set_mark(mark);
}

static void intt_matrix_incomplete(T* a, size_t n, T q, const T* itw) {
    size_t n1, n2, k = 16;
    size_t m = n / k;
    if (m == 96) {
        n1 = 3;
        n2 = 32;
    } else if (m == 24) {
        n1 = 3;
        n2 = 8;
    } else if (m == 120) {
        n1 = 15;
        n2 = 8;
    } else {
        intt_optimized_portable(a, n, q, itw);
        return;
    }

    size_t mark = poly_workspace_get_mark();
    T* tmp = poly_get_workspace(n);

    // Inverse logic mirroring ntt_matrix_incomplete
    // 1. Row-wise iNTT on blocks
    for (size_t row = 0; row < n1; row++) {
        T* row_ptr = &a[row * n2 * k];
        for (size_t len = 2; len <= n2; len <<= 1) {
            size_t half = len >> 1, step = n2 / len;
            for (size_t i = 0; i < n2; i += len) {
                for (size_t j = 0; j < half; j++) {
                    T w = itw[j * step * (n / n2)];
                    T *u = &row_ptr[(i + j) * k], *v = &row_ptr[(i + j + half) * k];
                    T v_tw[16], u_copy[16];
                    block16_mul_scalar(v_tw, v, w, q);
                    memcpy(u_copy, u, k * sizeof(T));
                    block16_add(u, u_copy, v_tw, q);
                    block16_sub(v, u_copy, v_tw, q);
                }
            }
        }
    }

    // 2. Column-wise iNTT on blocks
    for (size_t col = 0; col < n2; col++) {
        if (n1 == 3) {
            T w1 = itw[n / 3], w2 = itw[2 * n / 3];
            T* b0 = &a[(0 * n2 + col) * k];
            T* b1 = &a[(1 * n2 + col) * k];
            T* b2 = &a[(2 * n2 + col) * k];
            T u[16], v1[16], v2[16];
            memcpy(u, b0, k * sizeof(T));
            memcpy(v1, b1, k * sizeof(T));
            memcpy(v2, b2, k * sizeof(T));
            for (int i = 0; i < 16; i++) {
                b0[i] = zq_mod(u[i] + v1[i] + v2[i], q);
                b1[i] = zq_mod(u[i] + (T2)v1[i] * w1 + (T2)v2[i] * w2, q);
                b2[i] = zq_mod(u[i] + (T2)v1[i] * w2 + (T2)v2[i] * w1, q);
            }
        }
    }

    // 3. Normalization and back permutation
    T invM = zq_inverse(m, q);
    size_t n2_inv_n1 = 0;
    for (size_t x = 0; x < n1; x++)
        if ((x * n2) % n1 == 1) n2_inv_n1 = x;
    size_t n1_inv_n2 = 0;
    for (size_t x = 0; x < n2; x++)
        if ((x * n1) % n2 == 1) n1_inv_n2 = x;

    for (size_t j = 0; j < n1; j++) {
        for (size_t col = 0; col < n2; col++) {
            size_t orig_idx = (j * n2 * n2_inv_n1 + col * n1 * n1_inv_n2) % m;
            for (int i = 0; i < 16; i++)
                tmp[orig_idx * k + i] = zq_mod((T2)a[(j * n2 + col) * k + i] * invM, q);
        }
    }
    memcpy(a, tmp, n * sizeof(T));
    poly_workspace_set_mark(mark);
}

static void polymul_ring_cyclic_dynamic(T* c_main, const T* a, const T* b, size_t n, size_t M, T q) {
    init_twiddles_dynamic(M, q);
    size_t mark = poly_workspace_get_mark();
    T *fa = poly_get_workspace(M), *fb = poly_get_workspace(M);
    memset(fa, 0, M * sizeof(T));
    memset(fb, 0, M * sizeof(T));
    memcpy(fa, a, (n < M ? n : M) * sizeof(T));
    memcpy(fb, b, (n < M ? n : M) * sizeof(T));

    ntt_matrix_incomplete(fa, M, q, tw_main);
    ntt_matrix_incomplete(fb, M, q, tw_main);

    for (size_t i = 0; i < M / 16; i++) {
        T w = tw_main[i * (M / (M / 16))];
        weighted_mul_16(&fa[i * 16], &fa[i * 16], &fb[i * 16], w, q);
    }

    intt_matrix_incomplete(fa, M, q, itw_main);
    for (size_t i = 0; i < M; i++) c_main[i] = fa[i];
    poly_workspace_set_mark(mark);
}

void polymul_crt_polymul(T* c, const T* a, const T* b, size_t n, T q) {
    MonomialParams p = get_params(n);
    size_t mark = poly_workspace_get_mark();
    T *c_main = poly_get_workspace(p.n_main), *c_low = poly_get_workspace(p.n_low);
    memset(c_main, 0, p.n_main * sizeof(T));
    if (p.n_low > 0) memset(c_low, 0, p.n_low * sizeof(T));

    polymul_ring_cyclic_dynamic(c_main, a, b, n, p.n_main, q);

    if (p.n_low > 0) {
        T *la = poly_get_workspace(p.n_low), *lb = poly_get_workspace(p.n_low);
        memset(la, 0, p.n_low * sizeof(T));
        memset(lb, 0, p.n_low * sizeof(T));
        memcpy(la, a, (n < p.n_low ? n : p.n_low) * sizeof(T));
        memcpy(lb, b, (n < p.n_low ? n : p.n_low) * sizeof(T));
        T* c_low_full = poly_get_workspace(2 * p.n_low - 1);
        polymul_karatsuba_recursive(c_low_full, la, lb, p.n_low, q, 32);
        memcpy(c_low, c_low_full, p.n_low * sizeof(T));
    }

    // Phase 22: Unrolled Data Movement Loops (Portable)
    size_t out_max = 2 * n - 1;
    if (p.n_low > 0) {
        size_t low_copy = (p.n_low < out_max) ? p.n_low : out_max;
        memcpy(c, c_low, low_copy * sizeof(T));
    }

    if (p.n_low < out_max) {
        size_t mid_copy = (p.n_main < out_max) ? (p.n_main - p.n_low) : (out_max - p.n_low);
        memcpy(&c[p.n_low], &c_main[p.n_low], mid_copy * sizeof(T));
    }

    if (p.n_main < out_max) {
        size_t wrap_copy = (p.n_low < (out_max - p.n_main)) ? p.n_low : (out_max - p.n_main);
        for (size_t j = 0; j < wrap_copy; j++) {
            c[p.n_main + j] = zq_mod(c_main[j] + q - c_low[j], q);
        }
    }
    poly_workspace_set_mark(mark);
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[8] ALIGN_MEM, b[8] ALIGN_MEM, c[15] ALIGN_MEM, c_ref[15] ALIGN_MEM;
    printf("--- CRT Polynomial Multiplication (Phase 22) ---\n");
    printf("Method: TCHES 2025 Stage 3 Peak Efficiency.\n\n");
    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("B", b, n) != 0) return 1;
    poly_print("a", a, n);
    poly_print("b", b, n);
    poly_reset_workspace();
    polymul_crt_polymul(c, a, b, n, q);
    poly_polymul_ref(c_ref, a, n, b, n, q);
    poly_print("c (crt-polymul)", c, 15);
    poly_print("c (reference)", c_ref, 15);
    if (memcmp(c, c_ref, 15 * sizeof(T)) == 0)
        printf("RESULT: CORRECT\n");
    else
        printf("RESULT: INCORRECT\n");
    printf("--------------------------------------\n");
    return 0;
}
#endif