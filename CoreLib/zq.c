/**
 * @file zq.c
 * @brief Common code for working in the finite field Z/qZ (Z_q).
 *
 * Most code is only for pre-computation and not very efficient.
 */

#include "zq.h"

/**
 * @brief Computes inverse a^-1 of a, such that a*a^-1 = 1 mod q
 *
 * Hardware-Aware: Naive implementation kept for pre-computation,
 * but uses T2 for internal products to prevent 16-bit overflows.
 *
 * @param a element to be inverted
 * @param q modulus
 * @return T inverse
 */
T zq_inverse(T a, T2 q) {
    if (q == 0) q = 1 << 16;
    for (T2 i = 1; i < q; i++) {
        T2 p = (T2)a * i;
        if (p % q == 1) {
            return i;
        }
    }
    return 0;
}

/**
 * @brief Checks if a is a prime
 *
 * @param a element to be checked
 * @return int 1 is a is prime; 0 otherwise
 */
int zq_isPrime(T a) {
    if (a < 2) return 0;
    if (a == 2) return 1;
    if (a % 2 == 0) return 0;
    for (T i = 3; (T2)i * i <= a; i += 2) {
        if (a % i == 0) return 0;
    }
    return 1;
}

/**
 * @brief Computes exponentation a^e mod q
 *
 * Hardware-Aware: Replaced recursive implementation with an
 * iterative square-and-multiply algorithm to reduce stack overhead
 * and improve branch prediction on x64.
 *
 * @param a base
 * @param e exponent
 * @param q moduluis
 * @return T a^e mod q
 */
T zq_pow(T a, T2 e, T q) {
    T2 res = 1;
    T2 base;

    if (q == 1) return 0;
    if (e == 0) return 1;

    base = a % q;

    while (e > 0) {
        if (e % 2 == 1) res = (res * base) % q;
        base = (base * base) % q;
        e >>= 1;
    }
    return (T)res;
}

/**
 * @brief Finds an primitive n-th root of unity mod q
 *
 * @param n n
 * @param q modulus
 * @return T root, s.t., root^k = 1 mod q with root^l != 1 mod q for all l < k.
 */
T zq_primitiveRootOfUnity(T n, T q) {
    int isPrimitive;
    for (T i = 2; i < q; i++) {
        if (zq_pow(i, n, q) == 1) {
            isPrimitive = 1;
            for (T j = 2; j < n; j++) {
                if (zq_pow(i, j, q) == 1) {
                    isPrimitive = 0;
                    break;
                }
            }

            if (isPrimitive) {
                return i;
            }
        }
    }
    return 0;
}
