#include "rsa.h"
#include <stdlib.h>


long long power_mod(long long base, long long exp, long long mod) {
    long long res = 1;
    base = base % mod;
    while (exp > 0) {
        if (exp % 2 == 1) res = (res * base) % mod;
        exp = exp >> 1;
        base = (base * base) % mod;
    }
    return res;
}


void generate_rsa_keys(RSA_KeyPair *keypair) {
    long long p = 61;
    long long q = 53;
    
    keypair->n = p * q; 
    long long phi = (p - 1) * (q - 1); 
    
    keypair->e = 17; 
    keypair->d = 2753; 
}

long long rsa_encrypt(long long data, long long e, long long n) {
    return power_mod(data, e, n);
}

long long rsa_decrypt(long long cipher, long long d, long long n) {
    return power_mod(cipher, d, n);
}