"""
test_ntt_math2.py
-----------------
Purpose: Calculating Ground Truth for Polynomial A.

This specialized script was used to generate the exact expected result 
for the linear convolution of polynomial A multiplied by itself (A*A).
The output served as the benchmark for verifying the correctness of 
all optimized C implementations.
"""

import sys

q = 7681

def ntt_gs(a, n, w):
    """Gentleman-Sande butterfly implementation."""
    for l in range(n.bit_length()-1, 0, -1):
        length = 1 << (l-1)
        for i in range(0, n, 2*length):
            for j in range(length):
                W = pow(w, (n // (2*length)) * j, q)
                u = a[i+j]
                v = a[i+length+j]
                a[i+j] = (u + v) % q
                a[i+length+j] = ((u - v) * W) % q
    return a

def ntt_ct(a, n, winv):
    """Cooley-Tukey butterfly implementation."""
    for l in range(1, n.bit_length()):
        length = 1 << (l-1)
        for i in range(0, n, 2*length):
            for j in range(length):
                W = pow(winv, (n // (2*length)) * j, q)
                u = a[i+j]
                v = (a[i+length+j] * W) % q
                a[i+j] = (u + v) % q
                a[i+length+j] = (u - v) % q
    return a

# Polynomial A coefficients from input_config
a = [1, 2, 3, 4, 5, 6, 7, 8]

# Linear convolution via padding
N = 16
for i in range(2, q):
    if pow(i, N, q) == 1 and pow(i, N//2, q) != 1:
        w16 = i
        break
w16inv = pow(w16, -1, q)

a_pad = a + [0]*8

# Verification sequence
A = ntt_gs(a_pad.copy(), N, w16)
C = [(A[i]*A[i])%q for i in range(N)]
c_pad = ntt_ct(C.copy(), N, w16inv)
ninv = pow(N, -1, q)
c_pad = [(x * ninv)%q for x in c_pad]

print("Expected linear convolution result for A * A:")
print(c_pad[:15])

# Reference Ground Truth: 1, 4, 10, 20, 35, 56, 84, 120, 147, 164, 170, 164, 145, 112, 64
