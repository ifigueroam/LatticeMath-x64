# LatticeMath-x64 Framework

LatticeMath-x64 is a high-performance, hardware-exploiting framework designed for implementing and testing 
**polynomial multiplication algorithms** over finite fields ($\mathbb{Z}_q[x]$). Optimized for x86_64 
architectures, it provides a foundation for high-speed lattice-based cryptographic primitives.

---

## Project Hierarchy
```bash
LatticeMath-x64/
├── BaseLib/      # Headers, SIMD, Barrett/Montgomery Reduction, Timing
├── CoreLib/      # Memory Arena, Poly Utilities, Randomness, Config Loader
├── Scripts/      # Algorithms (Karatsuba, Toom-Cook, Optimized NTT, Benchmark)
├── Testing/      # Compiled Performance Binaries
├── input_config  # Text-based input for custom polynomials A and B
└── Makefile      # Native x64 Build System with Auto-Formatting
```

## Architectural Roadmap & Improvements

### 1. Arithmetic Tier (Phase 1 & 6)
Integrated **Montgomery Reduction** alongside Barrett for $q=7681$. Montgomery replaces high-word 
multiplication with low-word arithmetic, significantly reducing register pressure in SIMD loops.

### 2. Algorithmic Tier (Phase 2 & 6)
Implemented **CT/GS Butterfly Duality**. By pairing Cooley-Tukey and Gentleman-Sande butterflies, the 
$O(n)$ bit-reversal permutation step is eliminated entirely.

### 3. SIMD & Hardware Tier (Phase 3 & 6)
Explicit implementation of **AVX2 Vectorization** and **Word-Slicing**. Base cases for Karatsuba ($n=16$) 
are vectorized to process independent multiplications across SIMD lanes.

### 4. Memory & Cache Tier (Phase 4 & 6)
Implemented **Cache Tiling** ($32 \times 32$ blocks) and **Lazy Reduction** in Schoolbook multiplication. 
This ensures L1 cache residency and minimizes modular reduction overhead.

---

## Scientific Research & Optimization
The library is aligned with state-of-the-art PQC implementations (Kyber/Dilithium) through evidence-based 
optimizations from USENIX and IACR:

- **Montgomery Reduction:** Scientifically proven to reduce register pressure in SIMD NTT butterflies 
(Seiler, 2018).
- **Lazy Reduction:** Delaying modular reductions using 64-bit accumulators to maximize instruction 
throughput (Alkim et al., 2016).
- **CT/GS Duality:** Merging butterfly structures to eliminate the $O(n)$ bit-reversal bottleneck (IACR 
2018/1139).

### Scientific References (APA Format)
- Alkim, E., Ducas, L., Pöppelmann, T., & Schwabe, P. (2016). Post-quantum key exchange - a new 
hope. *Proceedings of the 25th USENIX Security Symposium*, 327–343.
- Edamatsu, H. (2023). Accelerating Large Integer Multiplication Using Intel AVX-512IFMA. 
*Journal of Signal Processing Systems*, *95*(1), 123–135.
- Seiler, G. (2018). Faster NTT-based polynomial multiplication for Kyber. *IACR Cryptology 
ePrint Archive*, 2018/1139.

---

## Configuration & Usage

### 1. Defining Input Polynomials
Modify the `input_config` file to set your test polynomials (A and B).

### 2. Building and Formatting
```bash
make format    # Enforce 105-column limit
make all       # Compile optimized benchmarks
```

### 3. Running Performance Benchmarks
```bash
./Testing/test_05benchmark  # Full Performance Comparison Grid
```
