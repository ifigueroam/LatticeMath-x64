Investigating NTT Discrepancies I'm delving into the user's report of unexpected output from test_04ntt. A
  thorough review of Scripts/04ntt.c is the first order of business to pinpoint where the mathematical logic might
  be failing. It appears the implementation of ntt_forward_gs and ntt_inverse_ct requires closer scrutiny.
  [Thought: true]Analyzing the NTT Core I'm now examining the ntt_forward_gs and ntt_inverse_ct functions within
  04ntt.c. I need to verify that the butterfly operations, particularly the Montgomery reductions and the
  application of twiddle factors, are mathematically sound for $q=7681$. There might be an issue with how the
  arrays are being padded or scaled.
  [Thought: true]Isolating the NTT Issue I've been carefully reviewing the 04ntt.c code. The issue seems to stem
  from the bit-reversal indexing logic or perhaps the transition between the Montgomery domain and the normal
  domain. I need to test these specific parts of the algorithm to pinpoint the exact location of the error.
  [Thought: true]I will analyze the mathematical discrepancy in 04ntt.c and explain why the current ntt_forward_gs
  and ntt_inverse_ct implementations are producing an incorrect result. I will then explain the necessary
  mathematical corrections to fix the issue.

  1. Analysis of the Mathematical Error in 04ntt.c

  The current implementation in 04ntt.c attempts to use the Cooley-Tukey (CT) / Gentleman-Sande (GS) Duality to
  avoid the $O(n)$ bit-reversal step. However, it contains two critical mathematical flaws that cause it to
  calculate the wrong product:

  Error A: Twiddle Factor Desynchronization
  In the ntt_forward_gs and ntt_inverse_ct functions, you are using a sequential counter t++ to load twiddle
  factors:

   1 T w = twiddles_mont[t++]; // Sequential access
   * The Problem: The GS and CT butterfly structures expect twiddle factors in a very specific, non-sequential
     order (usually a bit-reversed permutation of the roots of unity). By generating the twiddles sequentially in
     the main loop and accessing them sequentially (t++), the butterfly operations are multiplying by the wrong
     root of unity at almost every stage. This scrambles the frequency domain completely.

  Error B: Domain Inconsistency (Montgomery vs. Standard)
  The code aggressively mixes standard arithmetic and Montgomery arithmetic without proper entry and exit
  conversions.
   * The Problem: You use zq_montgomery_reduce in the butterfly loops:

   1     a[i + len + j] = zq_montgomery_reduce((T2)(u + q - v) * w, q);
      However, the input polynomial a was never explicitly transformed into the Montgomery domain (i.e., multiplied
  by $R \pmod q$). If you perform Montgomery multiplication on numbers that aren't in the Montgomery domain, the
  result is implicitly scaled down by $R^{-1}$, which corrupts the magnitude of every coefficient.

  ---

  2. How to Fix the NTT Implementation

  To fix 04ntt.c so it calculates the correct linear convolution, you must implement a Standard Iterative
  Decimation-in-Time (DIT) NTT. This is mathematically robust and much easier to verify.

  Here is the exact strategy to implement:

  Step 1: Re-introduce Explicit Bit-Reversal
  Instead of trying to implicitly handle bit-reversal via CT/GS duality (which requires complex twiddle table
  generation), you should explicitly bit-reverse the input arrays using the bitreverse() function defined in
  CoreLib/common.c.

   1 // At the start of the NTT core:
   2 bitreverse(a, n);

  Step 2: Implement a Standard DIT Butterfly
  Replace the complex GS and CT functions with a single, standard iterative Cooley-Tukey butterfly. This uses a
  standard nested loop structure where the twiddle factor index is easily calculated based on the current stage.

    1 static void ntt_core(T* a, size_t n, T q, const T* twiddles) {
    2     bitreverse(a, n); // Stage 1: Permute
    3     for (size_t len = 2; len <= n; len <<= 1) {
    4         size_t half = len >> 1;
    5         size_t step = n / len;
    6         for (size_t i = 0; i < n; i += len) {
    7             for (size_t j = 0; j < half; j++) {
    8                 T w = twiddles[j * step]; // Standard twiddle access
    9                 T u = a[i + j];
   10                 T v = (T)zq_mod((T2)a[i + j + half] * w, q);
   11                 a[i + j] = (T)zq_mod((T2)u + v, q);
   12                 a[i + j + half] = (T)zq_mod((T2)u + q - v, q);
   13             }
   14         }
   15     }
   16 }

  Step 3: Remove Montgomery Mixing
  For maximum stability and to guarantee the math works perfectly with your test_05benchmark configuration, remove
  the zq_montgomery_reduce calls from the butterfly and replace them with standard zq_mod. Montgomery is a great
  optimization, but it should only be added after the baseline standard arithmetic NTT is proven to work.

  Step 4: Ensure Linear Convolution Padding
  Ensure the zero-padding logic inside polymul_ntt satisfies the requirement $N \ge 2n - 1$ for linear convolution.

   1 size_t N = 1;
   2 while (N < 2 * n - 1) N <<= 1; // Correct padding

