#ifndef RSA_H
#define RSA_H

#include <stdint.h>

typedef struct {
    long long n; 
    long long e; 
    long long d; 
} RSA_KeyPair;


void generate_rsa_keys(RSA_KeyPair *keypair);
long long rsa_encrypt(long long data, long long e, long long n);
long long rsa_decrypt(long long cipher, long long d, long long n);

#endif