import sys

q = 7681

def bitreverse(a, n):
    bits = n.bit_length() - 1
    res = [0]*n
    for i in range(n):
        rev = int(f"{i:0{bits}b}"[::-1], 2)
        res[rev] = a[i]
    return res

def ntt_gs(a, n, w):
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

n = 8
w = 3383 # Primitive 8th root of unity mod 7681
winv = pow(w, -1, q)

a = [1, 2, 3, 4, 5, 6, 7, 8]
b = [8, 7, 6, 5, 4, 3, 2, 1]

# Pad to 16
N = 16
w16 = 5483 # primitive 16th root
w16inv = pow(w16, -1, q)

a_pad = a + [0]*8
b_pad = b + [0]*8

A = ntt_gs(a_pad.copy(), N, w16)
B = ntt_gs(b_pad.copy(), N, w16)

C = [(A[i]*B[i])%q for i in range(N)]

c_pad = ntt_ct(C.copy(), N, w16inv)
ninv = pow(N, -1, q)
c_pad = [(x * ninv)%q for x in c_pad]

print(c_pad[:15])

