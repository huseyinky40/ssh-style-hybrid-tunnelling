#ifndef PROTOCOL_H
#define PROTOCOL_H    

#include <stdint.h>
#include <string.h>   
#include "aes.h"
#include "rsa.h"

#define SERVER_PORT 12345
#define MAX_PAYLOAD 1024

#define TYPE_HANDSHAKE_REQ  10
#define TYPE_HANDSHAKE_RES  11
#define TYPE_HANDSHAKE_KEY  12

#define TYPE_MSG        1
#define TYPE_FILE_START 2
#define TYPE_FILE_DATA  3
#define TYPE_FILE_END   4

#define TYPE_ACK  20
#define TYPE_NACK 21

typedef struct {
    uint32_t frame_id;
    uint8_t  msg_type;
    uint32_t payload_size;
    uint32_t crc;
    uint8_t  data[MAX_PAYLOAD];
} Frame;

static uint32_t calculate_crc32(const uint8_t *data, size_t length) {
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < length; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            if (crc & 1) crc = (crc >> 1) ^ 0xEDB88320;
            else         crc >>= 1;
        }
    }
    return ~crc;
}

static void perform_aes_encrypt(uint8_t *buffer, size_t len, uint8_t *key) {
    struct AES_ctx ctx;             
    AES_init_ctx(&ctx, key);
    for (size_t i = 0; i + 16 <= MAX_PAYLOAD; i += 16) {
        AES_ECB_encrypt(&ctx, buffer + i);
    }
}

static void perform_aes_decrypt(uint8_t *buffer, size_t len, uint8_t *key) {
    struct AES_ctx ctx;             
    AES_init_ctx(&ctx, key);
    for (size_t i = 0; i + 16 <= MAX_PAYLOAD; i += 16) {
        AES_ECB_decrypt(&ctx, buffer + i);
    }
}

#endif