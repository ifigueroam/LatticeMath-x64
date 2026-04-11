/**
 * @file api.h
 * @brief API for polynomial operations.
 *
 * Declarations for the main polynomial operations and utility functions.
 * Helps to abstract away the implementation details and provide a clean interface for users.
 */
#ifndef API_H
#define API_H

#include "common.h"

typedef struct {
    T* coeffs;  // Pointer to ALIGN_MEM coefficients
    size_t n;   // Degree
    T q;        // Modulus
} Poly;

/**
 * @brief Initialize a polynomial with zero coefficients(aligned) given degree n and modulus q.
 * Helper function to set up the polynomial structure and allocate aligned memory for coefficients.
 *
 * Hardware-Aware Optimization: Uses posix_memalign for 32-byte alignment, compatible with C99.
 * Exits on allocation failure.
 *
 * @param p Pointer to the Poly structure to initialize
 * @param n Degree of the polynomial
 * @param q Modulus for the polynomial coefficients
 */
void poly_init(Poly* p, size_t n, T q);

/**
 * @brief Free the memory allocated for a polynomial.
 * Helper function to deallocate the memory for the polynomial coefficients.
 *
 * Hardware-Aware Optimization: Uses free for deallocation, compatible with C99.
 * @param p Pointer to the Poly structure to free
 */
void poly_free(Poly* p);

/**
 * @brief Parse command-line arguments for polynomial configuration.
 * Helper function to read polynomial parameters from command-line arguments.
 *
 * Hardware-Aware Optimization: Uses standard C library functions for parsing, compatible with C99.
 * @param argc Argument count
 * @param argv Argument values
 * @param n Pointer to store the polynomial degree
 * @param q Pointer to store the modulus
 */
void parse_config(int argc, char** argv, size_t* n, T* q);

#endif
