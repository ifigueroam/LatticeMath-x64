# Tools & Research Sandbox

This directory contains the Python-based mathematical scaffolding used during the 
development and synchronization of the LatticeMath-x64 library.

## Purpose of these Scripts
These files were created during the **mathematical synchronization phase** to serve as a 
"mathematical sandbox" for verifying complex Number Theoretic Transform (NTT) logic 
before its implementation in high-performance C.

### Included Tools:
1.  **test_ntt_math.py**: Prototyped the Gentleman-Sande (forward) and Cooley-Tukey (inverse) 
    butterfly duality. It verified that pairing these structures correctly eliminates 
    explicit bit-reversal shuffles.
2.  **test_ntt_math2.py**: Calculated the "Ground Truth" for polynomial **A**. It provided 
    the exact expected coefficients used to prove the C implementation's mathematical 
    correctness.
3.  **test_ntt_swap.py**: Developed to debug Twiddle Factor Interleaving logic, 
    verifying indexing when using loop-swapping for SIMD optimization.

## Why they exist
While the core engine is now fully implemented in C, these scripts remain essential for 
**audit and future research**. They allow for rapid prototyping if the project's parameters 
(such as modulus $q$ or ring dimension $n$) are modified in the future.
