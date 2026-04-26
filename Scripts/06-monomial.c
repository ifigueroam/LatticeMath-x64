/**
 * @file 06-monomial.c
 * @brief Phase 19: Monomial Factor CRT (Dynamic & Pruned)
 *
 * Implements size-aware domain selection and pruned NTT butterflies.
 * Alignment: TCHES 2025 (Chiu et al.).
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "poly.h"
#include "simd.h"
#include "zq.h"

// Parameters for different input sizes
typedef struct {
    size_t n_main;
    size_t n_low;
} MonomialParams;

static MonomialParams get_params(size_t n) {
    if (n <= 256) return (MonomialParams){384, 128}; // 2n-1 = 511; 384+128 = 512
    if (n <= 768) return (MonomialParams){1536, 0};   // NTT fallback for small n
    return (MonomialParams){1920, 128};              // 2n-1 = 2047; 1920+128 = 2048
}

// Precomputed twiddles
static T tw_main[2048] ALIGN_MEM;
static T itw_main[2048] ALIGN_MEM;
static size_t last_m = 0;

static void init_twiddles_dynamic(size_t m, T q) {
    if (last_m == m) return;
    // Base root for 7680
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
 * @brief Pruned DIT NTT Core (Phase V)
 * Skips butterflies where the second input is known to be zero.
 */
static void ntt_pruned(T* a, size_t n, size_t active_n, T q, const T* tw) {
    bitreverse(a, n);
    // Standard iterative NTT with zero-checks
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len >> 1, step = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; j++) {
                T w = tw[j * step], u = a[i + j], v = a[i + j + half];
                if (v == 0 && w == 1) continue; // Basic pruning
                T prod_v = zq_mod((T2)v * w, q);
                a[i + j] = zq_mod(u + prod_v, q);
                a[i + j + half] = zq_mod(u + q - prod_v, q);
            }
        }
    }
}

static void intt_generic(T* a, size_t n, T q, const T* itw) {
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

static void polymul_ring_cyclic_dynamic(T* c_main, const T* a, const T* b, size_t n, size_t M, T q) {
    init_twiddles_dynamic(M, q);
    
    size_t mark = poly_workspace_get_mark();
    T *fa = poly_get_workspace(M);
    T *fb = poly_get_workspace(M);
    
    // Explicit padding
    memset(fa, 0, M * sizeof(T));
    memset(fb, 0, M * sizeof(T));
    memcpy(fa, a, (n < M ? n : M) * sizeof(T));
    memcpy(fb, b, (n < M ? n : M) * sizeof(T));

    ntt_pruned(fa, M, n, q, tw_main);
    ntt_pruned(fb, M, n, q, tw_main);
    for (size_t i = 0; i < M; i++) fa[i] = zq_mod((T2)fa[i] * fb[i], q);
    intt_generic(fa, M, q, itw_main);
    
    for (size_t i = 0; i < M; i++) c_main[i] = fa[i];

    poly_workspace_set_mark(mark);
}

void polymul_monomial_crt(T* c, const T* a, const T* b, size_t n, T q) {
    MonomialParams p = get_params(n);
    
    size_t mark = poly_workspace_get_mark();
    T *c_main = poly_get_workspace(p.n_main);
    T *c_low = poly_get_workspace(p.n_low);
    memset(c_main, 0, p.n_main * sizeof(T));
    memset(c_low, 0, p.n_low * sizeof(T));

    polymul_ring_cyclic_dynamic(c_main, a, b, n, p.n_main, q);

    if (p.n_low > 0) {
        T *la = poly_get_workspace(p.n_low);
        T *lb = poly_get_workspace(p.n_low);
        memset(la, 0, p.n_low * sizeof(T));
        memset(lb, 0, p.n_low * sizeof(T));
        memcpy(la, a, (n < p.n_low ? n : p.n_low) * sizeof(T));
        memcpy(lb, b, (n < p.n_low ? n : p.n_low) * sizeof(T));
        T* c_low_full = poly_get_workspace(2 * p.n_low - 1);
        polymul_karatsuba_recursive(c_low_full, la, lb, p.n_low, q, 32);
        memcpy(c_low, c_low_full, p.n_low * sizeof(T));
    }

    memset(c, 0, (2 * n - 1) * sizeof(T));
    for (size_t i = 0; i < 2 * n - 1; i++) {
        if (i < p.n_low) c[i] = c_low[i];
        else if (i < p.n_main) c[i] = c_main[i];
        else c[i] = zq_mod(c_main[i - p.n_main] + q - c_low[i - p.n_main], q);
    }
    
    poly_workspace_set_mark(mark);
}

#ifndef BENCHMARK
int main(void) {
    size_t n = 8;
    T q = 7681;
    T a[8] ALIGN_MEM, b[8] ALIGN_MEM, c[15] ALIGN_MEM, c_ref[15] ALIGN_MEM;
    printf("--- Monomial Factor CRT (Phase V) ---\n");
    printf("Method: Dynamic Domain Sizing & Pruned NTT.\n\n");
    if (poly_load("A", a, n) != 0) return 1;
    if (poly_load("B", b, n) != 0) return 1;
    poly_print("a", a, n); poly_print("b", b, n);
    poly_reset_workspace();
    polymul_monomial_crt(c, a, b, n, q);
    poly_polymul_ref(c_ref, a, n, b, n, q);
    poly_print("c (monomial)", c, 15);
    poly_print("c (reference)", c_ref, 15);
    if (memcmp(c, c_ref, 15 * sizeof(T)) == 0) printf("RESULT: CORRECT\n");
    else printf("RESULT: INCORRECT\n");
    printf("-------------------------------------\n");
    return 0;
}
#endif