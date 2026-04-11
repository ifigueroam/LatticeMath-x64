# LatticeMath-x64 Framework

LatticeMath-x64 is a high-performance, hardware-exploiting framework designed for implementing and testing 
**polynomial multiplication algorithms** over finite fields ($\mathbb{Z}_q[x]$). Optimized for x86_64 
architectures, it provides a foundation for high-speed lattice-based cryptographic primitives.

---

## Project Hierarchy
```bash
LatticeMath-x64/
├── BaseLib/      # Headers, SIMD Intrinsics, Barrett Reduction, Timing
├── CoreLib/      # Memory Arena, Poly Utilities, Randomness, Config Loader
├── Scripts/      # Algorithms (Karatsuba, Toom-Cook, NTT, Benchmark)
├── Testing/      # Compiled Performance Binaries
├── input_config  # Text-based input for custom polynomials A and B
└── Makefile      # Native x64 Build System with Auto-Formatting
```

## Architectural Roadmap & Improvements

### 1. Arithmetic Tier (Phase 1)
Implemented **Barrett Reduction** for $q=7681$. This replaces high-latency `DIV` instructions with 
high-speed multiplication and bit-shifts, reducing reduction time by ~15x.

### 2. Algorithmic Tier (Phase 2)
Migrated from naive $O(n^2)$ NTT to a **Cooley-Tukey Fast NTT** ($O(n \log n)$). For $n=256$, operations 
were reduced from ~65,000 to ~2,000.

### 3. SIMD Tier (Phase 3)
Explicit implementation of **AVX2 Vectorization** in `BaseLib/simd.h`. Processes 16 coefficients 
simultaneously using 256-bit registers.

### 4. Memory Arena Tier (Phase 4)
Implemented a **Global Scratchpad Arena** in `CoreLib/poly.c`. This ensures temporary data stays 
resident in **L1/L2 CPU caches**, avoiding stack/heap allocation overhead.

### 5. Multi-Core Benchmark Tier (Current)
Integrated **OpenMP** and high-resolution timing to evaluate algorithms on 1 vs. 4+ cores.

---

## Configuration & Usage

### 1. Defining Input Polynomials
Modify the `input_config` file to set your test polynomials. The loader supports comma-separated or 
space-separated values:
```text
A: 1, 2, 3, 4, 5, 6, 7, 8
B: 8, 7, 6, 5, 4, 3, 2, 1
```

### 2. Building and Formatting
The build system automatically detects CPU features. It also includes a formatting tool to ensure code 
readability in side-by-side views (105-column limit):
```bash
make format    # Wraps all code and text to 105 columns
make all       # Compiles all optimized benchmarks
```

### 3. Running Performance Benchmarks
Execute the multi-threaded benchmark suite to see performance scaling across different $n$ sizes:
```bash
./Testing/test_05benchmark  # Comprehensive Performance Grid
```

## Usability: Object-Oriented C
The framework supports an initial `Poly` structure (see `BaseLib/api.h`) to encapsulate alignment, 
degree, and modulus, facilitating easier integration into larger projects.
