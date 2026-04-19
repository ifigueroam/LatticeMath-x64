# RESEARCH: Scientific Analysis & Technical Scaffolding

This document serves as a persistent record of all scientific research, mathematical 
analysis, and implementation guidance performed for the LatticeMath-x64 project.

---

## [2026-04-15] Study: The Monomial Factor CRT Trick
**Source:** TCHES 2025, Issue 4, "A New Trick for Polynomial Multiplication"
**Location:** `Research/LocalPaper/TCHES2025_4_26.pdf`

### 1. Core Mathematical Innovation
A method to perform polynomial multiplication using a modulus of the form 
$Q(x) = (x^{n_{main}} - 1)x^{n_{low}}$ is introduced in the paper. This allows a large 
multiplication $C=AB$ to be split into a main NTT-friendly part and a low-degree part.

### 2. Tailoring to LatticeMath-x64 ($q=7681$)
- **Problem:** The NTT for $n=1024$ is rendered dysfunctional by the current modulus $q=7681$, as 
  $q-1$ is not divisible by 2048.
- **Proposed Solution:** The use of $n_{main}=1536$ ($3 \times 512$) and $n_{low}=511$ is 
  proposed.
- **Implementation Path:**
    - A **Multi-Dimensional Good-Thomas NTT** ($3 \times 512$) is to be deployed.
    - The low part is to be computed via **Truncated Karatsuba**.
    - Reconstruction is to be performed using the simplified CRT map: 
      $C(x) = (C_{main}[0:n_{low}] - C_{low})(x^{n_{main}} - 1) + C_{main}$

---

## [2026-04-15] Investigation: Project Lineage & Scientific Mapping
**Objective:** Identify and connect the development of LatticeMath-x64 with the research 
papers in `Research/LocalPaper/`.

### 1. Verbose Thinking & Analysis
Lineage Assessment: The origins of the LatticeMath-x64 codebase were traced to the 
`mkannwischer/polymul` repository. It was recognized that the core algorithms (Schoolbook, 
Karatsuba, Toom-Cook, and NTT) are directly derived from the technical descriptions in 
Matthias J. Kannwischer's doctoral thesis. The choice of 16-bit signed arithmetic and 
specific modular reduction kernels (Barrett/Montgomery) was identified as a direct 
implementation of the "hardware-aware" principles established in the thesis.

Foundation Mapping: The theoretical necessity of these algorithms was connected to the 
works of Peikert (2016) and Micciancio & Regev (2008). The high-performance requirement of 
the library was justified by the cryptographic contexts of Ring-LWE and Module-LWE, which 
form the basis of NIST post-quantum standards like Kyber and Dilithium.

Accelerator Comparison: The benchmarking strategy implemented in Phase 5 was compared to 
the hardware-specific optimizations explored by Wang et al. (2025). It was concluded that 
the project's move toward multi-core OpenMP evaluation aligns with modern comparative 
performance standards for lattice-based primitives.

Bottleneck Resolution: The $n=1024$ NTT failure was analyzed through the lens of the TCHES 
2025 paper. It was determined that the "Monomial Factor" trick provides the specific 
mathematical bridge needed to maintain $O(n \log n)$ performance using a composite modulus 
compatible with $q=7681$.

### 2. Point-by-Point Scientific Connections
- **Algorithm Origin:** `Scripts/01-schoolbook.c` through `04-ntt.c` $\leftarrow$ 
  Kannwischer PhD Thesis (Section 2.2).
- **Arithmetic Logic:** Barrett and Montgomery implementations in `zq.h` $\leftarrow$ 
  Kannwischer PhD Thesis (Section 2.3).
- **Cryptographic Rings:** Rings $\mathbb{Z}_q[x]/(x^n+1)$ defined in `poly.h` $\leftarrow$ 
  Peikert (2016).
- **Optimization Motivation:** Multi-core benchmarking and cache tiling $\leftarrow$ 
  Wang et al. (2025).
- **Performance Scaling:** Monomial CRT strategy for $n=1024$ $\leftarrow$ Chiu et al. 
  (TCHES 2025).

---

## [2026-04-15] Analysis: NTT Mathematical Discrepancy Fix
**Objective:** Resolve incorrect coefficients in `04-ntt.c`.

### 1. Verbose Thinking & Analysis
Investigating NTT Discrepancies: The report of unexpected output from test_04-ntt was 
investigated. A thorough review of Scripts/04-ntt.c was prioritized to pinpoint where the 
mathematical logic may have failed. The implementation of ntt_forward_gs and 
ntt_inverse_ct was identified as requiring closer scrutiny.

Analyzing the NTT Core: The ntt_forward_gs and ntt_inverse_ct functions within 04-ntt.c were 
examined. Butterfly operations, particularly the Montgomery reductions and the 
application of twiddle factors, were verified for mathematical soundness under $q=7681$. 
Potential issues with array padding or scaling were evaluated.

Isolating the NTT Issue: The 04-ntt.c code was carefully reviewed. It was suggested that the 
issue stemmed from the bit-reversal indexing logic or the transition between the 
Montgomery domain and the normal domain. Specific segments of the algorithm were tested to 
pinpoint the exact location of the error.

### 2. Mathematical Error Analysis
Two critical mathematical flaws were identified in the previous implementation in 04-ntt.c, 
which attempted to use the Cooley-Tukey (CT) / Gentleman-Sande (GS) Duality to avoid the 
$O(n)$ bit-reversal step:

**Error A: Twiddle Factor Desynchronization**
Twiddle factors in the GS and CT butterfly structures were found to be accessed 
sequentially, whereas a specific, non-sequential order is expected. Consequently, the wrong 
roots of unity were used in the butterfly operations, effectively scrambling the frequency 
domain.

**Error B: Domain Inconsistency**
Standard and Montgomery arithmetic were found to be mixed without proper entry/exit 
conversions. The magnitude of the coefficients was corrupted due to Montgomery 
multiplication being performed on non-Montgomery inputs.

### 3. Guided Implementation Steps
To resolve the discrepancy in 04-ntt.c, a Standard Iterative Decimation-in-Time (DIT) NTT 
was implemented:

**Step 1: Re-introduce Explicit Bit-Reversal**
Input arrays are explicitly bit-reversed using the bitreverse() function from 
CoreLib/common.c.
```c
bitreverse(a, n);
```

**Step 2: Implement a Standard DIT Butterfly**
```c
static void ntt_core(T* a, size_t n, T q, const T* twiddles) {
    bitreverse(a, n); // Stage 1: Permute
    for (size_t len = 2; len <= n; len <<= 1) {
        size_t half = len >> 1;
        size_t step = n / len;
        for (size_t i = 0; i < n; i += len) {
            for (size_t j = 0; j < half; j++) {
                T w = twiddles[j * step];
                T u = a[i + j];
                T v = (T)zq_mod((T2)a[i + j + half] * w, q);
                a[i + j] = (T)zq_mod((T2)u + v, q);
                a[i + j + half] = (T)zq_mod((T2)u + q - v, q);
            }
        }
    }
}
```

**Step 3: Remove Montgomery Mixing**
Verification stability was ensured by replacing zq_montgomery_reduce calls with standard 
zq_mod.

**Step 4: Ensure Linear Convolution Padding**
Correct linear product results were ensured by enforcing the transform size $N \ge 2n - 1$.
```c
while (N < 2 * n - 1) N <<= 1;
```

---

## [2026-04-18] Research: 2-D Winograd-Based Divide-and-Conquer Method
**Source:** IEEE Transactions on VLSI Systems (2025), "An Efficient Polynomial Multiplication 
Accelerator for Lattice-Based Cryptography"
**Location:** `Research/LocalPaper/An_Efficient_Polynomial_Multiplication_Accelerator...`

### 1. Architectural Analysis & Verbose Thinking
Investigating the 2-D Winograd Algorithm: The "2-D Winograd-based polynomial multiplication" (2-D 
WPM) algorithm was analyzed as a superior alternative to traditional 1-D Winograd and 
Karatsuba/Toom-Cook methods for intermediate polynomial degrees. It was observed that the 
primary challenge in scaling 1-D Winograd lies in the rapid growth of denominator values and 
matrix element magnitudes, which necessitates expensive modular divisions.

Decomposing the 2-D Transformation: The core innovation of the 2-D WPM algorithm was recognized 
as the conversion of 1-D polynomial data into 2-D matrices to enable the use of $F(m \times m, 
r \times r)$ Winograd kernels. For the specific case of $m=3, r=3$, it was determined that a 
1-D tile of length 17 ($m^2 + r^2 - 1$) and a filter of length 9 ($r^2$) are required. The 
reshaping logic, which involves sliding a window of length 5 with a step of 3, was identified as 
the critical bridge between 1-D memory layouts and 2-D matrix arithmetic.

Eliminating Modular Divisions: A "division elimination" strategy was evaluated for integration 
into the LatticeMath-x64 framework. It was found that by scaling the filter transformation 
matrix $M_k$ by the least common multiple of its denominators ($L=6$), all fractional elements 
are converted to integers. The mathematical integrity of the convolution is maintained by 
performing a final scaling of the accumulated output by $(L^2)^{-1} \pmod q$. For $q=7681$, the 
modular inverse of 36 was calculated to be 2347, which serves as the final normalization 
constant.

Mapping to LatticeMath-x64: The implementation of 2-D WPM was recognized as a transitional 
optimization between the $O(n^2)$ Schoolbook and $O(n \log n)$ NTT. It was noted that while NTT 
remains theoretically faster for $n=256$, the 2-D Winograd method provides a robust, 
parameter-flexible alternative that avoids the root-of-unity restrictions of the NTT. The reuse 
of transformation results, as proposed by Wang et al., was identified as a key strategy for 
minimizing redundant computations during the sliding-window traversal of the polynomial tiles.

### 2. Mathematical Framework ($F(3 \times 3, 3 \times 3)$)
The implementation utilizes the following constant matrices derived from the 2-D convolution 
theory:

**Input Transformation ($M_{in}$, $5 \times 5$):**
Used to project a 1-D tile (reshaped to $5 \times 5$) into the Winograd domain.
```
[ 2 -1 -2  1  0]
[ 0 -2 -1  1  0]
[ 0  2 -3  1  0]
[ 0 -1  0  1  0]
[ 0  2 -1 -2  1]
```

**Filter Transformation ($M_k$, $5 \times 3$ - Scaled by $L=6$):**
Used to project a 1-D filter (reshaped to $3 \times 3$) into the Winograd domain without 
fractions.
```
[ 3  0  0]
[-3 -3 -3]
[-1  1 -1]
[ 1  2  4]
[ 0  0  6]
```

**Output Transformation ($M_{out}$, $3 \times 5$):**
Used to interpolate the Hadamard product result back to the 2-D spatial domain.
```
[ 1  1  1  1  0]
[ 0  1 -1  2  0]
[ 0  1  1  4  1]
```

### 3. Implementation Roadmap
1. **Reshaping:** $17$-point tiles of $A$ and $9$-point filters of $B$ are extracted and 
   arranged into $5 \times 5$ and $3 \times 3$ matrices respectively.
2. **Winograd Domain Transfer:** $D'_{in} = M_{in} D_{in} M_{in}^T$ and 
   $K'_r = M_k K_r M_k^T$ are computed using modular arithmetic.
3. **Hadamard Product:** $D'_{out} = D'_{in} \odot K'_r$ is performed element-wise.
4. **Inverse Mapping:** $D_{out} = M_{out} D'_{out} M_{out}^T$ recovers the $3 \times 3$ 
   result.
5. **Normalization:** Every element of $D_{out}$ is multiplied by $2347 \pmod{7681}$ to 
   account for the $L^2=36$ scaling.
6. **Overlap-Add:** The $3 \times 3$ results are reshaped back to 1-D and accumulated into the 
   final result polynomial $C$.

