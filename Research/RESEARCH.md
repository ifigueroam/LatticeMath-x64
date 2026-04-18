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
- **Problem:** NTT for $n=1024$ is broken by the current modulus $q=7681$ because $q-1$ is not 
  divisible by 2048.
- **Proposed Solution:** The use of $n_{main}=1536$ ($3 \times 512$) and $n_{low}=511$ is 
  proposed.
- **Implementation Path:**
    - A **Multi-Dimensional Good-Thomas NTT** ($3 \times 512$) is to be deployed.
    - The low part is to be computed via **Truncated Karatsuba**.
    - Reconstruction is to be performed using the simplified CRT map: 
      $C(x) = (C_{main}[0:n_{low}] - C_{low})(x^{n_{main}} - 1) + C_{main}$

---

## [2026-04-15] Analysis: NTT Mathematical Discrepancy Fix
**Objective:** Resolve incorrect coefficients in `04ntt.c`.

### 1. Verbose Thinking & Analysis
Investigating NTT Discrepancies: The report of unexpected output from test_04ntt was 
investigated. A thorough review of Scripts/04ntt.c was prioritized to pinpoint where the 
mathematical logic may have failed. The implementation of ntt_forward_gs and 
ntt_inverse_ct was identified as requiring closer scrutiny.

Analyzing the NTT Core: The ntt_forward_gs and ntt_inverse_ct functions within 04ntt.c were 
examined. Butterfly operations, particularly the Montgomery reductions and the 
application of twiddle factors, were verified for mathematical soundness under $q=7681$. 
Potential issues with array padding or scaling were evaluated.

Isolating the NTT Issue: The 04ntt.c code was carefully reviewed. It was suggested that the 
issue stemmed from the bit-reversal indexing logic or the transition between the 
Montgomery domain and the normal domain. Specific segments of the algorithm were tested to 
pinpoint the exact location of the error.

### 2. Mathematical Error Analysis
Two critical mathematical flaws were identified in the previous implementation in 04ntt.c, 
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
To resolve the discrepancy in 04ntt.c, a Standard Iterative Decimation-in-Time (DIT) NTT 
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
