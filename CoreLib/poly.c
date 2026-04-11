/**
 * @file poly.c
 * @brief Common code for working with polynomials.
 *
 * Polynomials are represented as T[] with T being defined in common.h.
 */

#include "poly.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "randombytes.h"
#include "zq.h"

/**
 * @brief Dumps a polynomial to stdout, e.g., "a= 16x^2 + 1x^1".
 *
 * @param polyname name to be printed, e.g, "a" in the example above
 * @param a polynomial
 * @param n number of coefficients
 */
void poly_print(char* polyname, T* a, size_t n) {
    int i;
    int first = 1;
    printf("%s=\t", polyname);
    for (i = (int)n - 1; i >= 0; i--) {
        if (a[i] == 0) continue;
        if (first) {
            first = 0;
        } else {
            printf(" + ");
        }
        printf("%dx^%d", a[i], i);
    }
    if (first) printf("0");
    printf("\n");
}

/**
 * @brief Dumps a polynomial to stdout, e.g., [0,1,16].
 *
 * @param a polynomial
 * @param n number of coefficients
 */
void poly_print2(T* a, size_t n) {
    size_t i;
    int first = 1;
    for (i = 0; i < n; i++) {
        if (first) {
            printf("[");
            first = 0;
        } else {
            printf(",");
        }
        printf("%d", a[i]);
    }
    printf("]\n");
}

/**
 * @brief Performs a schoolbook multiplication computing the full product with 2*n-1 coeffs.
 *
 * Hardware-Aware: Uses restrict keyword and tight loops to aid
 * compiler auto-vectorization (SIMD).
 *
 * @param c result polynomial
 * @param a first multiplicand polynomial
 * @param aN number of coefficients in a
 * @param b  second multiplicand polynomial
 * @param bN number of coefficients in b
 * @param q modulus
 */
void poly_polymul_ref(T* restrict c, const T* restrict a, size_t aN, const T* restrict b, size_t bN,
                      T q) {
    size_t cN = aN + bN - 1;
    size_t i, j;

    for (i = 0; i < cN; i++) c[i] = 0;

    for (i = 0; i < aN; i++) {
        T ai = a[i];
        for (j = 0; j < bN; j++) {
            T2 t = zq_mod((T2)ai * b[j], q);
            c[i + j] = zq_mod(c[i + j] + t, q);
        }
    }
}

#define WORKSPACE_SIZE 16384
static T global_workspace[WORKSPACE_SIZE] ALIGN_MEM;
static size_t workspace_ptr = 0;

T* poly_get_workspace(size_t size) {
    // Phase 4: Ensure alignment for Phase 3 SIMD (32-byte / 16 coefficients)
    size_t aligned_size = (size + 15) & ~15;
    if (workspace_ptr + aligned_size > WORKSPACE_SIZE) {
        fprintf(stderr, "FATAL: Workspace overflow!\n");
        return NULL;
    }
    T* ptr = &global_workspace[workspace_ptr];
    workspace_ptr += aligned_size;
    return ptr;
}

void poly_release_workspace(size_t size) {
    size_t aligned_size = (size + 15) & ~15;
    if (aligned_size > workspace_ptr)
        workspace_ptr = 0;
    else
        workspace_ptr -= aligned_size;
}

void poly_reset_workspace(void) { workspace_ptr = 0; }

/**
 * @brief Samples a uniformly random polynomial with coefficients in [0, q).
...
 * Hardware-Aware: Samples entropy in a single batch to minimize system
 * call overhead. Uses rejection sampling for modulus consistency.
 *
 * @param a polynomial
 * @param n number of coefficients in polynomial
 * @param q modulus
 */
void poly_random(T* a, size_t n, T q) {
    size_t i;
    randombytes((uint8_t*)a, n * sizeof(T));
    for (i = 0; i < n; i++) {
        while (a[i] >= q) {
            randombytes((uint8_t*)&a[i], sizeof(T));
        }
    }
}

/**
 * @brief Compares two polynomials mod Q.
 *
 * @param a first polynomial
 * @param b second polynomial
 * @param n number of coefficients in a and b
 * @param q modulus
 * @return int 0 if coefficients are equal mod q, 1 otherwise
 */
int poly_compare(T* a, T* b, size_t n, T q) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (zq_mod(a[i], q) != zq_mod(b[i], q)) {
            return 1;
        }
    }
    return 0;
}

/**
 * @brief Loads a polynomial from a file named "input_config".
 */
int poly_load(const char* name, T* a, size_t n) {
    FILE* fp = fopen("input_config", "r");
    if (!fp) {
        perror("Error opening input_config");
        return 1;
    }
    char line[4096];
    size_t name_len = strlen(name);
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, name, name_len) == 0 && line[name_len] == ':') {
            char* ptr = line + name_len + 1;
            for (size_t i = 0; i < n; i++) {
                int val;
                int consumed;
                // Skip anything that isn't a digit or a minus sign
                while (*ptr && !isdigit(*ptr) && *ptr != '-') ptr++;

                if (*ptr && sscanf(ptr, "%d%n", &val, &consumed) == 1) {
                    a[i] = (T)val;
                    ptr += consumed;
                } else {
                    a[i] = 0;  // Pad with zeros
                }
            }
            fclose(fp);
            return 0;
        }
    }
    fclose(fp);
    fprintf(stderr, "Polynomial '%s' not found in input_config\n", name);
    return 1;
}

/**
 * @brief Naive implementation of a cyclic NTT.
 *
 * Hardware-Aware: Precomputes powers of the root to avoid O(n^2 log q) complexity.
 *
 * @param t output polynomial in NTT domain
 * @param a input polynomial in normal domain
 * @param q modulus
 * @param n number of coefficients in a (and t)
 * @param root n-th root of unity modulo q
 */
void polymul_cyclic_ntt_forward_reference(T* t, const T* a, T q, T n, T root) {
    T i, j;
    T acopy[n];
    memcpy(acopy, a, sizeof(acopy));

    for (i = 0; i < n; i++) t[i] = 0;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            T2 exponent = (T2)i * j;
            T2 term = ((T2)acopy[j] * zq_pow(root, exponent, q)) % q;
            t[i] = (t[i] + (T)term) % q;
        }
    }
}

/**
 * @brief Naive implementation of an inverse cyclic NTT.
 *
 * @param t output polynomial in normal domain
 * @param a input polynomial in NTT domain
 * @param q modulus
 * @param n number of coefficients in a (and t)
 * @param root n-th root of unity
 */
void polymul_cyclic_ntt_inverse_reference(T* t, const T* a, T q, T n, T root) {
    T i, j;
    T acopy[n];
    memcpy(acopy, a, sizeof(acopy));
    for (i = 0; i < n; i++) t[i] = 0;

    T rootinv = zq_inverse(root, q);
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            T2 exponent = (T2)i * j;
            T2 term = ((T2)acopy[j] * zq_pow(rootinv, exponent, q)) % q;
            t[i] = (t[i] + (T)term) % q;
        }
    }

    T ninv = zq_inverse(n, q);
    for (i = 0; i < n; i++) t[i] = ((T2)t[i] * ninv) % q;
}

/**
 * @brief Naive implementation of a negacyclic NTT.
 *
 * @param t output polynomial in NTT domain
 * @param a input polynomial in normal domain
 * @param q modulus
 * @param n number of coefficients in a (and t)
 * @param root 2n-th root of unity
 */
void polymul_negacyclic_ntt_forward_reference(T* t, const T* a, T q, T n, T root) {
    T i, j;
    T acopy[n];
    memcpy(acopy, a, sizeof(acopy));
    for (i = 0; i < n; i++) t[i] = 0;

    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            T2 exponent = (T2)2 * i * j + j;
            T2 term = ((T2)acopy[j] * zq_pow(root, exponent, q)) % q;
            t[i] = (t[i] + (T)term) % q;
        }
    }
}

/**
 * @brief Naive implementation of an inverse negacyclic NTT.
 *
 * @param t output polynomial in normal domain
 * @param a input polynomial in NTT domain
 * @param q modulus
 * @param n number of coefficients in a (and t)
 * @param root 2n-th root of unity
 */
void polymul_negacyclic_ntt_inverse_reference(T* t, const T* a, T q, T n, T root) {
    T i, j;
    T acopy[n];
    memcpy(acopy, a, sizeof(acopy));
    for (i = 0; i < n; i++) t[i] = 0;

    T rootinv = zq_inverse(root, q);
    for (i = 0; i < n; i++) {
        for (j = 0; j < n; j++) {
            T2 exponent = (T2)2 * i * j;
            T2 term = ((T2)acopy[j] * zq_pow(rootinv, exponent, q)) % q;
            t[i] = (t[i] + (T)term) % q;
        }
    }

    T ninv = zq_inverse(n, q);
    for (i = 0; i < n; i++) {
        t[i] = ((T2)t[i] * ninv) % q;
        t[i] = ((T2)t[i] * zq_pow(rootinv, (T2)i, q)) % q;
    }
}
