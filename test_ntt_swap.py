q = 7681

def ntt_gs_swapped(a, n, w):
    t = 0
    twiddles = []
    # Precompute twiddles
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

print(c_pad[:15])

