"""
test_ntt_swap.py
----------------
Purpose: Debugging Twiddle Factor Interleaving logic.

This script prototyped the Sequential Twiddle Table access pattern.
It verified how twiddle factors should be re-ordered in memory so that 
multi-core and SIMD-optimized butterfly loops can access them using a 
simple linear index (t++), maximizing cache hit rates.
"""

q = 7681

def ntt_gs_swapped(a, n, w):
    """GS NTT with sequential twiddle table access."""
    t = 0
    twiddles = []
    # Pre-order twiddles for sequential consumption
    for l in range(n.bit_length()-1, 0, -1):
        length = 1 << (l-1)
        for j in range(length):
            W = pow(w, (n // (2*length)) * j, q)
            twiddles.append(W)
            
    for l in range(n.bit_length()-1, 0, -1):
        length = 1 << (l-1)
        for j in range(length):
            W = twiddles[t]
            t += 1
            for i in range(0, n, 2*length):
                u = a[i+j]
                v = a[i+length+j]
                a[i+j] = (u + v) % q
                a[i+length+j] = ((u - v + q) * W) % q
    return a

def ntt_ct_swapped(a, n, winv):
    """CT NTT with sequential twiddle table access."""
    t = 0
    twiddles = []
    for l in range(1, n.bit_length()):
        length = 1 << (l-1)
        for j in range(length):
            W = pow(winv, (n // (2*length)) * j, q)
            twiddles.append(W)
            
    for l in range(1, n.bit_length()):
        length = 1 << (l-1)
        for j in range(length):
            W = twiddles[t]
            t += 1
            for i in range(0, n, 2*length):
                u = a[i+j]
                v = (a[i+length+j] * W) % q
                a[i+j] = (u + v) % q
                a[i+length+j] = (u - v + q) % q
    return a

# Test case for A * A linear convolution
a = [1, 2, 3, 4, 5, 6, 7, 8]
N = 16
for i in range(2, q):
    if pow(i, N, q) == 1 and pow(i, N//2, q) != 1:
        w16 = i
        break
w16inv = pow(w16, -1, q)

a_pad = a + [0]*8
A = ntt_gs_swapped(a_pad.copy(), N, w16)
C = [(A[i]*A[i])%q for i in range(N)]
c_pad = ntt_ct_swapped(C.copy(), N, w16inv)
ninv = pow(N, -1, q)
c_pad = [(x * ninv)%q for x in c_pad]

print("Linear convolution result (using sequential twiddles):")
print(c_pad[:15])
