# LatticeMath-x64 Framework

LatticeMath-x64 is a high-performance, hardware-exploiting framework designed for implementing and testing 
**polynomial multiplication algorithms** over finite fields ($\mathbb{Z}_q[x]$). Optimized for x86_64 
architectures and Linux environments, it provides a foundational library for high-speed 
lattice-based cryptographic primitives such as Kyber and Dilithium.

The framework is built on the principle of **direct silicon control**, moving from high-level 
mathematical abstractions to instruction-level optimizations including AVX2 SIMD vectorization, 
custom Montgomery reduction kernels, and static cache-hot memory management.

---

## Project Hierarchy & Scientific Audit Trail
```bash
LatticeMath-x64/
├── BaseLib/      # Arithmetic Foundations (Arithmetic Tier)
│   ├── zq.h      # Barrett & Montgomery reduction kernels (Division-free logic)
│   ├── simd.h    # AVX2 manual intrinsics (16-way sub-cycle parallelism)
│   └── api.h     # Opaque Poly structure and developer interface (Encapsulation)
├── CoreLib/      # System Infrastructure (Cache & Memory Tier)
│   ├── poly.c    # Global Scratchpad Arena (Eliminates stack/heap latency)
│   └── poly_api.c# posix_memalign lifecycle management (HW alignment enforcement)
├── Scripts/      # Standardized Algorithm Suite (Algorithmic Tier)
│   ├── 00-benchmark.c   # Multi-core OpenMP grid with nanosecond telemetry
│   ├── 01-schoolbook.c  # O(n^2) Cache-tiled baseline with Lazy Reduction
│   ├── 02-karatsuba.c   # O(n^1.58) Recursive with SIMD word-slicing base-cases
│   ├── 03-toom.c        # O(n^1.46) Evaluation-interpolation strategy (n=9 padding)
│   ├── 04-ntt.c         # O(n log n) Iterative DIT transform (Butterfly Duality)
│   └── 05-winograd.c    # 2-D Divide-and-Conquer accelerator (F(3x3, 3x3))
├── Tools/        # Mathematical Scaffolding (Python Prototyping & Ground Truth)
├── Testing/      # Compiled Standalone Performance Binaries
├── Research/     # Scientific Audit Corpus (Internal)
│   ├── LocalPaper/ # PDF repository of foundational cryptographic research
│   └── RESEARCH.md # Verbose mathematical analysis and design thinking blocks
└── input_config  # Custom polynomial test vector storage (Synchronized Testing)
```

---

## Detailed Architectural Roadmap & Evolution Phases

The library's development is structured across seven phases, as detailed in the **TRACKLOG.md**, 
transitioning the system from a "Mathematical Reference" to an "Instruction-Aware Engine."

### Phase 1: Arithmetic Tier (Barrett Reduction Bedrock)
- **Objective:** Eliminate high-latency `div` instructions (80-100 cycles) from modular arithmetic.
- **Architectural Transition:** Moved from compiler-dependent modulo (`%`) to a proactive 
  multiplication-shift model.
- **Scientific Design Rationale:** Targeted $q=7681$ to allow 16-bit coefficients to fit within 
  32-bit products, maximizing register usage without overflow.
- **Impact:** Achieved a ~15x reduction in modular reduction latency.

### Phase 2: Algorithmic Tier (Iterative DIT NTT)
- **Objective:** Reduce algorithmic complexity from quadratic $O(n^2)$ to log-linear $O(n \log n)$.
- **Architectural Transition:** Replaced recursive summation with a symmetric butterfly-tree structure.
- **Mechanism:** Cooley-Tukey Decimation-in-Time (DIT) with precomputed twiddle factors.
- **Impact:** Operations for $n=256$ reduced by 97%, enabling high-security parameters ($n=1024$).

### Phase 3: Hardware Tier (AVX2 SIMD Vectorization)
- **Objective:** Exploit sub-cycle parallelism via 256-bit YMM registers.
- **Innovation:** Developed the **Unsigned Comparison XOR-trick** to circumvent AVX2's lack of 
  native 16-bit unsigned range checks.
- **Mechanism:** Range-shifting coefficients to perform modular range checks in parallel.
- **Impact:** Processes 16 coefficients simultaneously per cycle; total $16 \times$ scaling.

### Phase 4: Cache Tier (Global Scratchpad Arena)
- **Objective:** Address the "Memory Wall" by keeping data resident in L1/L2 CPU caches.
- **Architectural Transition:** Shifted from "Dynamic/Decentralized" memory to "Static/Arena-Based."
- **Rationale:** Recursive Karatsuba/Toom-Cook calls previously triggered excessive TLB misses 
  and stack frame overhead.
- **Impact:** Verified 0% TLB misses during deep recursion; significant reduction in memory latency.

### Phase 5: API Tier & Multi-Core Engineering (OpenMP)
- **Objective:** Standardize the framework for professional integration and thread-level scaling.
- **API logic:** Implemented opaque `Poly` objects to enforce 32-byte alignment at the type-system level.
- **Discovery:** Identified the **Parallelism Paradox** via nanosecond telemetry, where atomic 
  contention in parallel loops degraded performance for small $n$.

### Phase 6: PQC Scientific Alignment (Montgomery Arithmetic)
- **Objective:** Align with state-of-the-art primitives (Kyber/Dilithium) and reduce register pressure.
- **Implementation:** Replaced high-word Barrett multipliers with low-word Montgomery reduction kernels.
- **Rationale:** Montgomery uses fewer high-word instructions on x64, freeing registers for NTT loops.
- **Impact:** Achieved an additional 51% speedup in Schoolbook and 38% in NTT throughput.

### Phase 7: Winograd Accelerator Tier (2-D Domain Transform)
- **Objective:** Alleviate matrix growth and denominator complexity of 1-D Winograd.
- **Mechanism:** Implemented the $F(3 \times 3, 3 \times 3)$ kernel with a **Division Elimination** 
  scaling trick (L=6).
- **Mathematical Synchronization:** Resolved cross-correlation vs. convolution discrepancies via 
  **Filter Reversal**, ensuring bit-identical results with Schoolbook.

---

## Performance Analysis & Hardware Telemetry (n=1024)

Based on the latest benchmark run (q=7681, 1 Core baseline):

| Algorithm    | Latency (us) | Computational Model           | Architectural Tier      |
|--------------|--------------|-------------------------------|-------------------------|
| Schoolbook   | 3169.62      | O(n^2) Cache-Tiled            | Baseline                |
| Karatsuba    | 936.62       | O(n^1.58) SIMD Word-Sliced    | Recursive Acceleration  |
| Toom-Cook-3  | 2007.28      | O(n^1.46) Evaluation-Matrix   | Algorithmic Leap        |
| Winograd (k) | 3.23         | O(n^1.58) Matrix Domain       | HW-Aware Accelerator   |
| NTT          | 12449.18     | O(n log n) Frequency Domain   | Algorithmic Peak        |

*Note: NTT performance reflects linear convolution padding requirements ($N \ge 2n-1$).*

---

## Scientific Audit & Lineage
The LatticeMath-x64 implementation is directly mapped to the following research corpus:
- **Arithmetic Foundations:** Derived from Matthias J. Kannwischer's PhD Thesis (Section 2.2).
- **Montgomery Reduction:** Based on Seiler (2018) for faster Kyber butterflies.
- **2-D Winograd Accelerator:** Based on Wang et al. (2025), specifically the $L=6$ division-elimination.
- **Monomial CRT Strategy:** Analyzed from TCHES 2025 "New Trick" to resolve $n=1024$ field constraints.

Full technical details and thinking blocks for these mappings are available in `Research/RESEARCH.md`.

---

## Configuration & Usage

### 1. Defining Input Polynomials
Modify the `input_config` file to set your test polynomials (A and B).
```text
A: 1, 2, 3, 4, 5, 6, 7, 8
B: 8, 7, 6, 5, 4, 3, 2, 1
```

### 2. Building and Formatting
The project includes a native build system that enforces architectural standards automatically.
```bash
make format    # Enforce 105-column limit and clang-format
make all       # Compile all optimized benchmark binaries
```

### 3. Running Performance Benchmarks
```bash
./Testing/test_00-benchmark  # Comprehensive performance grid (1 vs 4+ cores)
./Testing/test_05-winograd   # Standalone standardized Winograd test
```
