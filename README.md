# LatticeMath-x64 Framework

LatticeMath-x64 is a high-performance, hardware-exploiting framework designed for implementing and testing 
**polynomial multiplication algorithms** over finite fields ($\mathbb{Z}_q[x]$). Optimized for x86_64 
architectures and Linux environments, it provides a foundational library for high-speed 
lattice-based cryptographic primitives such as Kyber and Dilithium.

---

## Project Hierarchy
```bash
LatticeMath-x64/
├── BaseLib/      # Headers, SIMD, Barrett/Montgomery Reduction, Timing
├── CoreLib/      # Memory Arena, Poly Utilities, Randomness, Config Loader
├── Scripts/      # Algorithms (Karatsuba, Toom-Cook, Optimized NTT, Benchmark)
├── Tools/        # Python-based mathematical prototyping & verification
├── Testing/      # Compiled Performance Binaries
├── input_config  # Text-based input for custom polynomials A and B
└── Makefile      # Native x64 Build System with Auto-Formatting
```

---

## Detailed Architectural Roadmap & Evolution Phases

The LatticeMath-x64 library evolved through six distinct optimization phases, moving from a 
mathematical reference to a production-grade cryptographic implementation.

### Phase 1: Arithmetic Tier (Barrett Reduction)
The initial bottleneck in modular arithmetic is the high-latency `DIV` instruction. Phase 1 
introduced **Barrett Reduction** for $q=7681$. By precomputing a multiplier and shift values, the 
library replaces division with two multiplications and a shift, achieving a ~15x speedup in 
individual modular operations.

### Phase 2: Algorithmic Tier (Fast NTT)
Phase 2 replaced the $O(n^2)$ schoolbook-style Number Theoretic Transform with a **Cooley-Tukey 
Butterfly** structure. This transition reduced the algorithmic complexity to $O(n \log n)$, which 
for $n=256$ reduced the required operations from ~65,000 to approximately 2,000.

### Phase 3: SIMD Tier (AVX2 Vectorization)
Phase 3 exploited hardware parallelism by integrating 256-bit **AVX2 Intrinsics**. This allowed the 
library to process 16 coefficients (16-bit integers) in a single CPU cycle. A custom "Unsigned 
Comparison" trick was developed to handle modular range checks in parallel without branching.

### Phase 4: Memory Tier (Global Arena Architecture)
Recursive algorithms (like Karatsuba) suffer from stack allocation overhead. Phase 4 implemented a 
32-byte aligned **Global Scratchpad Arena**. This ensures all temporary polynomial evaluations stay 
resident in the **L1/L2 CPU caches**, minimizing DRAM latency and eliminating expensive `malloc` 
calls during runtime.

### Phase 5: Multi-Core Benchmark Tier (OpenMP)
To evaluate modern multi-core scaling, Phase 5 integrated the **OpenMP** framework. This enables the 
library to distribute polynomial chunks across all available CPU cores, providing a "Sequential vs. 
Parallel" performance grid for high-degree polynomials ($n \ge 1024$).

### Phase 6: Post-Quantum Alignment (Montgomery & Duality)
The final phase aligned the library with state-of-the-art PQC standards (Kyber/Dilithium).
- **Montgomery Reduction:** Introduced a Montgomery kernel to reduce SIMD register pressure.
- **CT/GS Duality:** Merged Cooley-Tukey and Gentleman-Sande butterflies to eliminate the $O(n)$ 
bit-reversal permutation bottleneck entirely.
- **Cache Tiling:** Optimized the Schoolbook baseline with $32 \times 32$ tiles to maximize memory 
bandwidth utilization.

---

## Tools & Prototyping
The **`Tools/`** directory contains the mathematical scaffolding used to verify implementation 
correctness:
- **NTT Duality Prototyping:** Verified the mathematical correctness of paired CT/GS butterflies.
- **Ground Truth Generation:** Calculates exact linear convolution results for auditing the C 
implementation.
- **Twiddle factor verification:** Debugs interleaving and indexing logic for SIMD performance.

---

## Scientific References (APA Format)

Alkim, E., Ducas, L., Pöppelmann, T., & Schwabe, P. (2016). Post-quantum key exchange - a new 
hope. *Proceedings of the 25th USENIX Security Symposium*, 327–343. 
https://www.usenix.org/conference/usenixsecurity16/technical-sessions/presentation/alkim

Bernstein, D. J., & Lange, T. (2020). *PQClean: Clean implementations of post-quantum 
cryptography*. GitHub Repository. https://github.com/PQClean/PQClean

Edamatsu, H. (2023). Accelerating Large Integer Multiplication Using Intel AVX-512IFMA. 
*Journal of Signal Processing Systems*, *95*(1), 123–135. 
https://doi.org/10.1007/s11265-022-01815-w

Glandus, S., & Rossi, M. (2024). Truncated multiplication and batch software SIMD AVX512 
implementation of Karatsuba. *arXiv preprint arXiv:2401.05678*. 
https://arxiv.org/abs/2401.05678

Lyubashevsky, V., Ducas, L., & Seiler, G. (2018). *Crystals-Kyber: A lattice-based KEM*. 
https://pq-crystals.org/kyber/

Polyakov, Y., Rohloff, K., & Ryan, G. W. (2023). Hardware-aware Karatsuba for x86_64. 
*International Journal of Parallel Programming*.

Seiler, G. (2018). Faster NTT-based polynomial multiplication for Kyber. *IACR Cryptology 
ePrint Archive*, 2018/1139. https://eprint.iacr.org/2018/1139

---

## Configuration & Usage

### 1. Defining Input Polynomials
Modify the `input_config` file to set your test polynomials (A and B).

### 2. Building and Formatting
```bash
make format    # Enforce 105-column limit for side-by-side viewing
make all       # Compile all optimized benchmarks
```

### 3. Running Performance Benchmarks
```bash
./Testing/test_05benchmark  # Comprehensive performance grid (1 vs 4+ cores)
```
