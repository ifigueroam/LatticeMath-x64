/**
 * @file zq.h
 * @brief Common code for working in the finite field Z/qZ (Z_q).
 * This header defines functions for modular reduction, modular inverse, primality testing,
 * modular exponentiation, and finding primitive roots of unity, all of which are essential
 * for polynomial operations in the context of cryptographic applications. The functions are
 * designed to be efficient and hardware-aware, ensuring that they can handle larger values
 * of a and q efficiently, which is important for applications that may require working with
 * large moduli in cryptographic contexts.
 */
#ifndef ZQ_H
#define ZQ_H

#include "common.h"

/**
 * @brief reduces a to [0, q) if q != 0; no-op if q=0
 *
 * Helper function for zq_mod that operates on T type, which is the primary coefficient type
 * used in polynomial operations.
 *
 * Hardware-Aware: The function should be implemented using an efficient reduction algorithm,
 * such as Barrett reduction or Montgomery reduction, to minimize the number of modulus operations
 * and ensure that it can handle larger values of a and q efficiently, which is important for
 * applications that may require working with large moduli in cryptographic contexts.
 *
 * @param a element to be reduced
 * @param q modulus
 * @return T a mod q in [0, q)
 */
static inline T zq_mod(T2 a, T q) {
    if (q == 0) return (T)a;
    if (q == 7681) {
        uint64_t q_hat = ((uint64_t)a * 559166) >> 32;
        T res = (T)(a - q_hat * 7681);
        if (res >= 7681) res -= 7681;
        if (res >= 7681) res -= 7681;
        return res;
    }
    return (T)(a % q);
}

/**
 * @brief reduces a to [0, q) if q != 0; no-op if q=0
 * Helper function for zq_mod that operates on T2 type, which is used for intermediate results
 * that may exceed the range of T.
 *
 * Hardware-Aware: The function should be implemented using an efficient reduction algorithm,
 * such as Barrett reduction or Montgomery reduction, to minimize the number of modulus operations
 * and ensure that it can handle larger values of a and q efficiently, which is important for
 * applications that may require working with large moduli in cryptographic contexts.
 *
 * @param a element to be reduced
 * @param q modulus
 * @return T2 a mod q in [0, q)
 */
static inline T2 zq_mod2(T2 a, T2 q) {
    if (q == 0) return a;
    if (q == 7681) {
        uint64_t q_hat = ((uint64_t)a * 559166) >> 32;
        T2 res = a - q_hat * 7681;
        if (res >= 7681) res -= 7681;
        if (res >= 7681) res -= 7681;
        return res;
    }
    return a % q;
}

/**
 * @brief Computes inverse a^-1 of a, such that a*a^-1 = 1 mod q
 * Helper function for zq_inverse that operates on T type, which is the primary coefficient type
 * used in polynomial operations.
 *
 * Hardware-Aware: The function should be implemented using an efficient algorithm for computing
 * modular inverses, such as the Extended Euclidean Algorithm, to ensure that it can handle larger
 * values of a and q efficiently, which is important for applications that may require computing
 * inverses in large finite fields, especially in cryptographic contexts where performance is critical.
 *
 * @param a element to invert
 * @param q modulus
 * @return T a^-1 mod q in [0, q) if it exists, otherwise 0
 */
T zq_inverse(T a, T2 q);

/**
 * @brief Checks if a is a prime
 * Helper function for zq_isPrime that operates on T type, which is
 * the primary coefficient type used in polynomial operations.
 *
 * Hardware-Aware: The function should be implemented using an efficient primality test, such as
 * Miller-Rabin, to ensure that it can handle larger values of a efficiently, which is important for
 * applications that may require checking the primality of large moduli in cryptographic contexts.
 *
 * @param a number to check for primality
 * @return int 1 if a is prime, 0 otherwise
 *
 */
int zq_isPrime(T a);

/**
 * @brief Computes exponentation a^e mod q
 * Helper function for zq_pow that operates on T type, which is the
 * primary coefficient type used in polynomial operations.
 *
 * Hardware-Aware: The function should be implemented using an efficient modular exponentiation
 * algorithm, such as exponentiation by squaring, to minimize the number of multiplications and modulus
 * operations, which is critical for performance in cryptographic applications where large exponents may
 * be common.
 * @param a base
 * @param e exponent
 * @param q modulus
 * @return T a^e mod q in [0, q)
 */
T zq_pow(T a, T2 e, T q);

/**
 * @brief Finds an primitive n-th root of unity mod q
 * Helper function for zq_primitiveRootOfUnity that operates on T type,
 * which is the primary coefficient type used in polynomial operations.
 *
 * Hardware-Aware: The function should be implemented using an efficient
 * algorithm for finding primitive roots of unity,
 *
 * @param n order of the root of unity
 * @param q modulus
 * @return T primitive n-th root of unity mod q if it exists, otherwise 0
 */
T zq_primitiveRootOfUnity(T n, T q);
#endif
