#ifndef _AES_H_
#define _AES_H_

#include <stdint.h>
#include <stddef.h>


#define AES_BLOCKLEN 16 


struct AES_ctx {
  uint8_t RoundKey[176];
};


void AES_init_ctx(struct AES_ctx* ctx, const uint8_t* key);
void AES_ECB_encrypt(const struct AES_ctx* ctx, uint8_t* buf);
void AES_ECB_decrypt(const struct AES_ctx* ctx, uint8_t* buf);

#endif