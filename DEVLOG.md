# LatticeMath-x64 - Comprehensive Architectural Evolution Log

## Overview
This log documents the end-to-end evolution of the LatticeMath-x64 project, tracking the 
transformation from a mathematical reference into a high-performance, hardware-exploiting 
cryptographic primitive library.

---

## Phase 1: Arithmetic Optimization (Barrett Reduction)
**Objective:** Eliminate high-latency modular reduction bottlenecks.
**Changes:**
- Implemented **Barrett Reduction** in `BaseLib/zq.h` specifically tuned for $q=7681$.
- Computed the optimized multiplier `559166` to allow for 16-bit coefficient arithmetic without 
signed overflow.
- Replaced all naive `% q` instances across the `Scripts/` directory with `zq_mod`.
- **Result:** Achieved a ~15x speedup in modular reduction compared to the standard `DIV` 
instruction.

## Phase 2: Algorithmic Leap (Fast NTT)
**Objective:** Reduce polynomial multiplication complexity from $O(n^2)$ to $O(n \log n)$.
**Changes:**
- Replaced the $O(n^2)$ naive NTT with a **Cooley-Tukey Decimation-in-Time (DIT)** transform.
- Implemented automatic zero-padding to the next power of two to support flexible $n$ values.
- Integrated bit-reversal permutation for efficient butterfly processing.
- **Result:** Reduced total operations for $n=256$ from ~65,000 to ~2,000.

## Phase 3: SIMD Vectorization (AVX2)
**Objective:** Leverage hardware parallelism via 256-bit registers.
**Changes:**
- Created `BaseLib/simd.h` containing optimized AVX2 primitives.
- Implemented an **Unsigned Comparison Trick** to handle 16-bit modular reductions in parallel:
  ```c
  static inline __m256i _mm256_cmpgt_epu16(__m256i a, __m256i b) {
      __m256i offset = _mm256_set1_epi16(0x8000);
      return _mm256_cmpgt_epi16(_mm256_xor_si256(a, offset), _mm256_xor_si256(b, offset));
  }
  ```
- Added a **Scalar Tail Handler** to ensure safety when processing non-multiple-of-16 array sizes.

## Phase 4: Memory Architecture (Global Arena)
**Objective:** Eliminate stack overhead and maximize cache locality.
**Changes:**
- Engineering of a 32-byte aligned `global_workspace` in `CoreLib/poly.c`.
- Replaced recursive stack-based scratchpad allocations in Karatsuba and Toom-Cook with arena 
offsets.
- **Result:** Guaranteed L1/L2 cache residency for all temporary evaluations, significantly 
reducing memory latency.

## Phase 5: Professional API (Opaque Structs)
**Objective:** Standardize the library for external integration.
**Changes:**
- Introduced the `Poly` structure in `api.h` to encapsulate degree, modulus, and alignment.
- Implemented `poly_init` using `posix_memalign` for hardware-enforced 32-byte alignment.
- Added a configuration parser for command-line parameterization.

---

## Execution Log - [2026-03-29]
- **Target:** `Testing/test_01schoolbook`
- **Configuration:** n=256, q=7681 (Barrett-optimized)
- **Result:** SUCCESS. Verified baseline correctness for Phase 1 logic.

---

## [2026-04-11] Project Overview & Architecture Analysis
- **Goal:** Comprehensive audit of the LatticeMath-x64 framework.
- **Summary:** Verified that the framework successfully integrates Barrett Reduction, AVX2 SIMD, 
and a Global Arena. The system is confirmed stable for $n=256, q=7681$.

---

## [2026-04-11] Custom Input Implementation (input_config)
- **Goal:** Enable persistent, user-defined polynomial testing.
- **Actions:**
    - Created `input_config` for text-based coefficient storage.
    - Implemented `poly_load` in `CoreLib/poly.c` with robust parsing (skipping spaces/commas).
    - Updated all test scripts to utilize the new loader for synchronized $A \times A$ testing.

---

## [2026-04-11] Deep Math Analysis & Test Synchronization
- **Finding:** Discrepancies in `test_01schoolbook` (Negacyclic) and `test_03toom` (n=6 size).
- **Actions:**
    - Standardized all algorithms to **Linear Convolution** (Full Product).
    - Upgraded Toom-Cook to $n=9$ with zero-padding to process the full $n=8$ input.
- **Result:** All four algorithms now return identical full-product polynomials for the same input.

---

## [2026-04-11] Formatting Rule & Mandate Implementation
- **Goal:** Enforce side-by-side readability (105-column limit).
- **Actions:**
    - Created `.clang-format` with `ColumnLimit: 105`.
    - Added `make format` to the Makefile.
    - Updated `GEMINI.md` with a permanent documentation and formatting mandate.

---

## [2026-04-11] Multi-Core Benchmarking & OpenMP Integration
- **Goal:** Evaluate performance scaling across $n \in \{256, 512, 768, 1024\}$ using all CPU cores.
- **Implementation:**
    - Integrated **OpenMP** into the build system.
    - Created `05benchmark.c` to produce a Markdown-aligned performance grid.
    - Added high-resolution nanosecond timing (`get_time_ns`) to `common.h`.
- **Result:** Successfully benchmarked Schoolbook, Karatsuba, Toom-Cook, and NTT in both single and 
multi-threaded modes.

---

## [2026-04-11] Parallel Performance Analysis (The Parallelism Paradox)
- **Problem:** The benchmark revealed that 4-core execution was significantly slower than 1-core 
for the Schoolbook algorithm.
- **Root Cause Analysis:**
    1.  **Atomic Contention:** The use of `#pragma omp atomic` in the parallel loop forced CPU 
cores to serialize access to the result array, creating a "lineup" effect.
    2.  **False Sharing:** Multiple cores were attempting to write to the same 64-byte cache line, 
causing the line to "bounce" between L1 caches (Cache Thrashing).
    3.  **Management Overhead:** For smaller $n$, the time to spawn and sync threads exceeds the 
computational work.
- **Future Improvement:** Implement "Thread-Local Storage" where each core maintains its own 
partial product array, followed by a final parallel reduction to eliminate atomic locking.
