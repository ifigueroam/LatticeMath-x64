# LatticeMath-x64 Framework

LatticeMath-x64 is a high-performance, hardware-exploiting framework designed for implementing and
testing **polynomial multiplication algorithms** over finite fields ($\mathbb{Z}_q[x]$). Optimized for
x86_64 architectures and Linux environments, it provides a foundational library for high-speed
lattice-based cryptographic primitives such as Kyber and Dilithium.

The framework is built on the principle of **direct silicon control**, moving from high-level
mathematical abstractions to instruction-level optimizations including AVX2 SIMD vectorization,
custom Montgomery reduction kernels, and static cache-hot memory management.

---

## Table of Contents
1. [Project Hierarchy](#project-hierarchy--scientific-audit-trail)
2. [Surviving Architectural Pillars](#surviving-architectural-pillars)
3. [Detailed Milestone Timeline](#detailed-milestone-timeline)
4. [Mathematical Core & Foundations](#mathematical-core--foundations)
5. [Algorithm Deep-Dive](#algorithm-deep-dive)
6. [Performance Analysis](#performance-analysis--hardware-telemetry-n1024)
7. [Configuration & Usage](#configuration--usage)
8. [Scientific Audit & Lineage](#scientific-audit--lineage)

---

## Project Hierarchy & Scientific Audit Trail
```bash
LatticeMath-x64/
├── BaseLib/      # Arithmetic Foundations (Arithmetic Tier)
│   ├── zq.h      # Barrett & Montgomery reduction (Division-free logic)
│   ├── simd.h    # AVX2 manual intrinsics (16-way sub-cycle parallelism)
│   └── api.h     # Opaque Poly structure and developer interface
├── CoreLib/      # System Infrastructure (Cache & Memory Tier)
│   ├── poly.c    # Global Scratchpad Arena (Eliminates stack/heap latency)
│   └── poly_api.c # posix_memalign management (HW alignment enforcement)
├── Scripts/      # Standardized Algorithm Suite (Algorithmic Tier)
│   ├── 00-benchmark.c   # Shielded statistical grid with RDTSCP cycle counting
│   ├── 01-schoolbook.c  # O(n^2) Cache-tiled baseline with Lazy Reduction
│   ├── 02-karatsuba.c   # O(n^1.58) Recursive with SIMD base-cases
│   ├── 03-toom-cook.c   # O(n^1.40) Toom-4 Hybrid with SIMD Lazy Interpolation
│   ├── 04-ntt.c         # O(n log n) High-Performance Complex Domain FFT
│   ├── 05-crt-polymul.c # O(n log n) TCHES 2025 Monomial Factor CRT
│   └── 06-winograd.c    # O(n^1.58) 2-D Divide-and-Conquer accelerator
├── Tools/        # Engineering & Stabilization Utilities
│   ├── bench_shield.sh # Laboratory-grade CPU shielding script
│   └── format_docs.py  # Automated 105-column documentation formatter
├── Docs/         # Evolutionary & Historical Documentation
│   ├── DEVLOG.md    # Technical development milestones and discovery
│   └── TRACKLOG.md  # Detailed historical timeline and visual evolution
└── input_config  # Custom polynomial test vector storage
```

---

## Surviving Architectural Pillars

The library follows a rigorous, scientifically-guided maturation cycle. The following pillars
represent the most advanced implementations that successfully scaled to production-grade
performance.

### I. Arithmetic Tier (Montgomery & Barrett Bedrock)
- **Objective:** Eliminate high-latency integer division (`div`) instructions.
- **Mechanism:** Implemented Barrett reduction using precomputed 64-bit reciprocals and high-speed
  Montgomery reduction for low-word multiplications.
- **Impact:** Achieved a ~15x reduction in modular reduction latency compared to standard operators.

### II. Hardware Tier (AVX2 SIMD Vectorization)
- **Objective:** Exploit sub-cycle parallelism via 256-bit YMM registers.
- **Mechanism:** Engineered the **Unsigned Comparison XOR-trick** for 16-bit unsigned arithmetic.
- **Impact:** Processes 16 coefficients simultaneously per cycle; total $16 \times$ spatial scaling.

### III. Cache Tier (Static Scratchpad Arena)
- **Objective:** Prevent cache thrashing by maintaining data locality in L1/L2 caches.
- **Mechanism:** Utilizes a 32-byte aligned static memory pool with mark-based allocation logic.
- **Impact:** Verified 0% TLB misses during deep recursion; eliminates heap allocation latency.

### IV. Algorithmic Peak (Monomial Factor CRT & Complex FFT)
- **Objective:** Bypass field constraints and reach the information-theoretic limit ($O(n \log n)$).
- **Mechanism:** Implemented Stage 4 Matrix Supremacy with 2D Incomplete Transforms and Complex
  Domain FFT utilizing FMA3 units.
- **Impact:** CRT-Polymul established as the supreme multiplier (~250 kCyc for $n=1024$).

### V. Security Tier (Constant-Time Stability)
- **Objective:** Ensure resistance to timing-based side-channel attacks.
- **Mechanism:** Strictly branchless data movement and bitwise modular masking.
- **Impact:** Execution time is independent of polynomial coefficient values.

---

## Detailed Milestone Timeline

### Phase 16: High-Fidelity Shielding & Convergence (April 2026)
- **Objective:** Eliminate OS noise and establish the project's production baseline.
- **Architectural Transition:** Shifted from "Statistical Median" to "Shielded Laboratory" metrics.
- **Mechanism:** Serialized timing fences (`rdtscp` + `cpuid`), core affinity pinning, and 
  system-level governor lockdown.
- **Rationale:** Standard RDTSC is susceptible to instruction reordering and frequency scaling; 
  the shield stabilizes the environment for sub-cycle auditing.
- **Impact:** Achieved laboratory-grade cycle-accurate telemetry with < 0.5% jitter.

### Phase 14: Global Framework Stabilization (April 2026)
- **Objective:** Finalize the performance hierarchy and technical rationale for multiplier 
  supremacy.
- **Architectural Transition:** Standardized the framework around the $O(n \log n)$ scaling 
  dominance of CRT-Polymul.
- **Mechanism:** Refactored algorithmic nomenclature and conducted a cross-analysis of Big-O 
  complexity versus software instruction pressure.
- **Rationale:** Butterfly-based transforms minimize instruction pressure and data expansion 
  compared to recursive Winograd trees.
- **Impact:** Established a high-fidelity benchmark reference for lattice-based primitives.

### Phase 13: Matrix-Reshaped Matrix Supremacy (April 2026)
- **Objective:** Maximize data-movement efficiency and reach the absolute theoretical latency floor.
- **Architectural Transition:** Shifted from "Generalized 1D NTT" to "Specialized 2D Incomplete Matrix 
  Transform."
- **Mechanism:** Deployed Good-Thomas 2D decomposition with early-stop size-16 blocks and 
  Zero-Copy CRT reconstruction.
- **Rationale:** 2D decomposition minimizes permutation frequency and arithmetic depth.
- **Impact:** Achieved record-breaking **254.4 kCyc** for $n=1024$.

---

## Mathematical Core & Foundations

The framework operates over the ring $R_q = \mathbb{Z}_q[x] / (x^n + 1)$, where $q = 7681$ is a
prime chosen for its high density of primitive roots ($7681 = 15 \times 2^9 + 1$). This enables
efficient Number Theoretic Transforms (NTTs) for degrees up to $n=512$. For higher degrees like
$n=1024$, the framework utilizes the **Monomial Factor CRT** and **Complex Domain FFT** to bypass
the primitive root ceiling.

---

## Algorithm Deep-Dive

### 05-CRT-Polymul (Framework Supreme)
The current performance leader. Based on **TCHES 2025 (Chiu et al.)**, it utilizes a 2D Incomplete
Matrix Transform to achieve quasi-linear scaling with minimal data movement. It is the most
architecturally advanced multiplier in the suite.

### 06-Winograd (Iterative Radix-4)
Implements a 2-D Winograd-based multiplier ($F(3 \times 3, 3 \times 3)$) inspired by **Wang et al.
(2025)**. Stage 13 utilizes an iterative radix-4 network and vectorized reconstruction to achieve a
7.3% speedup over recursive variants.

---

## Performance Analysis & Hardware Telemetry (n=1024)

Based on the latest shielded benchmark run (q=7681, 1 Core baseline, **Median of 1000 iterations**):

| Algorithm    | Median (kCyc) | Computational Model           | Architectural Tier      |
|--------------|---------------|-------------------------------|-------------------------|
| Schoolbook   | 7248.0        | O(n^2) Cache-Tiled            | Baseline                |
| Karatsuba    | 1889.0        | O(n^1.58) SIMD Word-Sliced    | Recursive Acceleration  |
| Toom-Cook    | 1726.7        | O(n^1.40) Hybrid AVX2 Lazy    | Algorithmic Leap        |
| NTT (FFT)    | 673.1         | O(n log n) Complex Domain     | Algorithmic Peak        |
| CRT-Polymul  | 254.4         | O(n log n) 2D Matrix Stage 4  | Multi-Domain Supremacy  |
| Winograd (k) | 1028.3        | O(n^1.58) Iterative Radix-4    | Functional Pillar       |

---

## Configuration & Usage

### 1. Build System
The project uses a native Makefile enforcing C99 standards and 105-column formatting.
- **`make all`**: Compiles all optimized multiplication scripts into `Testing/`.
- **`make clean`**: Removes all binaries and intermediate object files.
- **`make format`**: Enforces the readability standards via `clang-format`.

### 2. Performance Benchmarking (Shielded)
The primary entry point for high-fidelity performance evaluation is the **Shielded Benchmark**.
- **Laboratory Grade (Recommended):** `sudo ./Tools/bench_shield.sh`
- **Output:** Reports metrics in `Min / Med / Jitter` format. Cycle counting is RDTSCP-serialized.

### 3. Standalone Verification
Each algorithm provides a standalone executable for bit-level verification and step-by-step logging.
- **Example:** `./Testing/test_05-crt-polymul` or `./Testing/test_04-ntt`.

---

## Scientific Audit & Lineage
The LatticeMath-x64 implementation is directly mapped to the following research corpus:
- **TCHES 2025:** Chiu, C.-M., et al. "A New Trick for Polynomial Multiplication."
- **IEEE TVLSI 2025:** Wang, Z., et al. "2-D Winograd-Based Divide-and-Conquer Multiplier."
- **ISSAC 2007:** Bodrato, M., & Zanoni, A. "Optimal Toom-Cook Interpolation."
- **SUPERCOP:** Bernstein & Lange (2020). Performance Benchmarking Standards.

Full technical details, thought blocks, and hardware-alignment thinking are available in
`Research/RESEARCH.md`.
