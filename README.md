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
│   ├── zq.h      # Barrett & Montgomery reduction (Division-free logic)
│   ├── simd.h    # AVX2 manual intrinsics (16-way sub-cycle parallelism)
│   └── api.h     # Opaque Poly structure and developer interface
├── CoreLib/      # System Infrastructure (Cache & Memory Tier)
│   ├── poly.c    # Global Scratchpad Arena (Eliminates stack/heap latency)
│   └── poly_api.c # posix_memalign management (HW alignment enforcement)
├── Scripts/      # Standardized Algorithm Suite (Algorithmic Tier)
│   ├── 00-benchmark.c   # Iterative statistical grid with RDTSC cycle counting
│   ├── 01-schoolbook.c  # O(n^2) Cache-tiled baseline with Lazy Reduction
│   ├── 02-karatsuba.c   # O(n^1.58) Recursive with SIMD base-cases
│   ├── 03-toom.c        # O(n^1.40) Toom-4 Hybrid with SIMD Lazy Interpolation
│   ├── 04-ntt.c         # O(n log n) High-Performance Complex Domain FFT
│   └── 05-winograd.c    # 2-D Divide-and-Conquer accelerator (F(3x3, 3x3))
├── Docs/         # Evolutionary & Historical Documentation
│   ├── DEVLOG.md    # Technical development milestones and discovery
│   └── TRACKLOG.md  # Detailed historical timeline and visual evolution
├── Testing/      # Compiled Standalone Performance Binaries
└── input_config  # Custom polynomial test vector storage
```

---

## Detailed Architectural Roadmap (Surviving Pillars)

The library's development follows a rigorous, scientifically-guided maturation cycle. The following 
phases represent the "Surviving Pillars" of the architecture—the most advanced, hardware-optimized 
implementations that successfully scaled to production-grade performance.

### Phase 1: Arithmetic Tier (Barrett Reduction Bedrock)
- **Objective:** Eliminate high-latency `div` instructions (80-100 cycles) from modular arithmetic.
- **Architectural Transition:** Moved from compiler-dependent modulo (`%`) to a multiplication-shift model.
- **Mechanism:** Implemented Barrett reduction using precomputed 64-bit reciprocals and constant shifts.
- **Rationale:** Integer division is the single greatest bottleneck in modular algebra; shifts are near-instant.
- **Impact:** Achieved a ~15x reduction in modular reduction latency.

### Phase 2: Hardware Tier (AVX2 SIMD Vectorization)
- **Objective:** Exploit sub-cycle parallelism via 256-bit YMM registers.
- **Architectural Transition:** Shifted from "Scalar Sequential" to "Vector Parallel" execution.
- **Mechanism:** Developed the **Unsigned Comparison XOR-trick** using `_mm256` intrinsics.
- **Rationale:** AVX2 lacks native 16-bit unsigned comparisons; XOR-offsets enable standard signed logic.
- **Impact:** Processes 16 coefficients simultaneously per cycle; total $16 \times$ scaling.

### Phase 3: Cache Tier (Static Scratchpad Arena)
- **Objective:** Address the "Memory Wall" by keeping data resident in L1/L2 CPU caches.
- **Architectural Transition:** Shifted from "Dynamic" memory to a unified "Mark-based Restoration" arena.
- **Mechanism:** Engineered a 32-byte aligned static memory pool with LIFO allocation logic.
- **Rationale:** Recursive `malloc`/`free` calls cause cache thrashing and non-deterministic latency.
- **Impact:** Verified 0% TLB misses during deep recursion; expanded capacity to 64KB for multi-threading.

### Phase 4: API Tier & Multi-Core Engineering (OpenMP)
- **Objective:** Standardize the framework for professional integration and thread-level scaling.
- **Architectural Transition:** Shifted from "Research Scripts" to "Production-Grade API."
- **Mechanism:** Implemented opaque `Poly` objects and OpenMP fork-join loop optimization.
- **Rationale:** Encapsulation and multi-core utilization are mandatory for modern cryptographic libraries.
- **Impact:** Enabled linear scaling across 32+ logical cores.

### Phase 5: PQC Scientific Alignment (Montgomery Arithmetic)
- **Objective:** Align with state-of-the-art primitives (Kyber/Dilithium) and reduce register pressure.
- **Architectural Transition:** Transitioned from Barrett reduction to Montgomery reduction.
- **Mechanism:** Replaced multi-word Barrett logic with low-word Montgomery reduction kernels.
- **Rationale:** Montgomery reduction avoids high-word multiplications, freeing up YMM registers.
- **Impact:** Achieved an additional 51% speedup in Schoolbook throughput.

### Phase 6: Winograd Accelerator Tier (2-D Domain Transform)
- **Objective:** Alleviate matrix complexity of 1-D Winograd via 2-D matrix domain mapping.
- **Architectural Transition:** Shifted from "Linear Convolution" to "2-D Matrix Domain Acceleration."
- **Mechanism:** Implemented the $F(3 \times 3, 3 \times 3)$ kernel with a Division Elimination scaling trick.
- **Rationale:** 2-D transforms reduce the number of multiplications faster than 1-D splits for small $N$.
- **Impact:** Achieved ultra-low block latency (~2.5 us), providing a high-speed transitional engine.

### Phase 7: High-Performance Non-NTT Kernels (Bodrato Strategy)
- **Objective:** Resolve the constant-factor bottleneck of Toom-Cook linear transformations.
- **Architectural Transition:** Transitioned from "Naive Matrix" to "Optimal Addition Sequences."
- **Mechanism:** Applied **Bodrato’s Optimal Addition Sequences** combined with **Lazy Interpolation**.
- **Rationale:** Reducing the number of additions in the interpolation phase is critical for Toom-Cook.
- **Impact:** Reclaimed theoretical superiority of Toom-Cook over Karatsuba for high-degree rings.

### Phase 8: True Definitive Roadmap (Hybrid SIMD Lazy Toom-4)
- **Objective:** Maximize real-world throughput by uniting Genuine AVX2 processing and Hybrid Execution.
- **Architectural Transition:** Transitioned from "Strict Isolation" to "Hybrid Production Model."
- **Mechanism:** Rewrote interpolation using 32-bit accumulators over contiguous $n/4$ chunks.
- **Rationale:** 32-bit intermediate registers allow "Lazy Reduction," delaying modulo $q$ until the end.
- **Impact:** Slashed latency for $n=1024$ to ~2080 kCyc, definitively outperforming Karatsuba.

### Phase 9: High-Performance FFT (Complex Domain Integration)
- **Objective:** Overcome field constraints ($q=7681$) that cripple finite-field transforms at $n=1024$.
- **Architectural Transition:** Shifted from "Discrete Modulo Logic" to "Continuous Domain Math."
- **Mechanism:** Abandoned the field $\mathbb{Z}_q$ for the complex field $\mathbb{C}$ using AVX2 FMA3.
- **Rationale:** Complex numbers provide unconditional roots existence; FMA3 units offer superior throughput.
- **Impact:** Achieved a $30\times$ speedup over previous NTTs, reaching ~1000 kCyc for $n=1024$.

### Phase 10: Robust Benchmarking Suite (Statistical Professionalization)
- **Objective:** Establish a standardized, noise-resistant framework for performance evaluation.
- **Architectural Transition:** Transitioned from "Diagnostic Timing" to "High-Fidelity Profiling."
- **Mechanism:** Implemented **Iterative Benchmarking** (Median of 1000 runs) and **RDTSC Cycle Counting**.
- **Rationale:** Cycle counting is frequency-independent; medians reject OS context-switch noise.
- **Impact:** Provides high-fidelity telemetry, enabling scientific precision in optimization tracking.

### Phase 11: Monomial Factor CRT (Hybrid Multi-Domain)
- **Objective:** Bypass the $q=7681$ primitive-root constraint for high-security rings ($n=1024$).
- **Architectural Transition:** Shifted from "Single-Domain" to "Multi-Domain" multiplication routing.
- **Mechanism:** Utilizes a lifting modulus $Q(x) = (x^{n_{main}} - 1)x^{n_{low}}$ and Ruritanian permutations.
- **Rationale:** Coprime moduli allow decoupling the product into domains where NTT roots exist.
- **Impact:** Reclaims prime-field NTT speed for $n=1024$ without floating-point precision risks.

### Phase 12: Stage 3 Hardware Alignment (Instruction-Level Pruning)
- **Objective:** Exploit 256-bit wide ALU pipelines and relieve the modular multiplier bottleneck.
- **Architectural Transition:** Shifted from "Element-Wise Pruning" to "Instruction-Level Hardware Alignment."
- **Mechanism:** Implemented block-wise SIMD pruning (`_mm256_testz_si256`) and Crude Barrett arithmetic.
- **Rationale:** Vectorized zero-skipping reduces branch pressure; shifts replace high-latency multiplications.
- **Impact:** Reduced constant-factor overhead, achieving sub-700 kCyc latency for $n=1024$.

### Phase 13: Stage 4 Matrix Supremacy (2D Incomplete Transforms)
- **Objective:** Maximize data-movement efficiency and algorithmic depth reduction.
- **Architectural Transition:** Shifted from "Generalized 1D NTT" to "Specialized 2D Incomplete Matrix Transform."
- **Mechanism:** Transitioned to 2D Good-Thomas Matrix transforms with "Early-Stop" incomplete blocks.
- **Rationale:** 2D decomposition minimizes permutation frequency; incomplete transforms reduce arithmetic depth.
- **Impact:** Achieved record-breaking **314.1 kCyc** for $n=1024$, establishing the Monomial CRT as the supreme standard.

---

## Performance Analysis & Hardware Telemetry (n=1024)

Based on the latest benchmark run (q=7681, 1 Core baseline, **Median of 1000 iterations**):

| Algorithm    | Median (kCyc) | Computational Model           | Architectural Tier      |
|--------------|---------------|-------------------------------|-------------------------|
| Schoolbook   | 8382.3        | O(n^2) Cache-Tiled            | Baseline                |
| Karatsuba    | 2413.2        | O(n^1.58) SIMD Word-Sliced    | Recursive Acceleration  |
| Toom-Cook-4  | 2080.0        | O(n^1.40) Hybrid AVX2 Lazy    | Algorithmic Leap        |
| NTT (FFT)    | 790.5         | O(n log n) Complex Domain     | Algorithmic Peak        |
| Monomial CRT | 314.1         | O(n log n) 2D Matrix Stage 4  | Multi-Domain Supremacy  |
| Winograd (k) | 4.7           | O(n^1.58) Matrix Domain       | HW-Aware Accelerator   |

---

## Configuration & Usage

The LatticeMath-x64 framework is designed for both rapid benchmarking and deep architectural research. 
Follow these steps to build, test, and customize the multiplier suite.

### 1. The Build System
The project uses a native Makefile that enforces strict C99 standards and 105-column formatting.
- **Standard Build:** `make all` compiles all optimized multiplication scripts into the `Testing/` directory.
- **Cleanup:** `make clean` removes all binaries and intermediate object files. Always run this before 
  switching between 16-bit and 32-bit coefficient modes.
- **Formatting Mandate:** `make format` runs `clang-format` on the entire codebase. This is mandatory 
  to adhere to the project's readability standards.

### 2. Executing Performance Benchmarks
The primary entry point for performance analysis is the **Robust Benchmark Suite**.
- **Run Grid:** `./Testing/test_00-benchmark`
- **Output Metrics:** The suite reports **CPU Kilocycles (kCyc)**. This is a frequency-independent 
  metric calculated via the `RDTSC` instruction. To convert to microseconds ($\mu s$) on a 3.0GHz 
  CPU, divide the kilocycles by 3.
- **Statistical Rigor:** The suite performs 1000 iterations and selects the **Median**. This 
  ensures that OS context switches and background interrupts do not skew your results.
- **Parallelism Control:** Use the environment variable `OMP_NUM_THREADS` to control the number of 
  cores used during the multi-threaded grid tests.

### 3. Standalone Correctness Testing
Each algorithm is provided as a standalone executable in the `Testing/` folder for bit-level 
verification.
- **Run FFT Test:** `./Testing/test_04-ntt`
- **Run Toom-4 Test:** `./Testing/test_03-toom`
- **Execution Log:** Each standalone test prints a verbose step-by-step log explaining how the 
  polynomials are partitioned, evaluated, and reconstructed.

### 4. Customizing Test Vectors
To test the multipliers with your own specific polynomials:
- Open the `input_config` file in the root directory.
- Define coefficients for polynomial A and B (comma-separated).
- The framework automatically synchronizes these values across all standalone scripts upon execution.

---

## Scientific Audit & Lineage
The LatticeMath-x64 implementation is directly mapped to the following research corpus:
- **High-Performance FFT:** Harvey, D., et al. (2016). *Journal of Complexity*.
- **Toom-Cook Optimizations:** Bodrato, M., & Zanoni, A. (2007). *ISSAC*.
- **2-D Winograd Accelerator:** Wang, Z., et al. (2025). *IEEE TVLSI*.
- **Benchmark Standard:** SUPERCOP (Bernstein & Lange, 2020).

Full technical details and thinking blocks for these mappings are available in `Research/RESEARCH.md`.
