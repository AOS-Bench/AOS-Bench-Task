/* bitops.c — bit manipulation and endian helpers */
#include "bitops.h"

uint32_t bo_popcount(uint64_t x) {
    uint32_t c = 0;
    while (x) {
        c += (uint32_t)(x & 1);
        x >>= 1;
    }
    return c;
}

uint32_t bo_reverse32(uint32_t x) {
    x = ((x >> 1) & 0x55555555u) | ((x & 0x55555555u) << 1);
    x = ((x >> 2) & 0x33333333u) | ((x & 0x33333333u) << 2);
    x = ((x >> 4) & 0x0F0F0F0Fu) | ((x & 0x0F0F0F0Fu) << 4);
    x = ((x >> 8) & 0x00FF00FFu) | ((x & 0x00FF00FFu) << 8);
    x = (x >> 16) | (x << 16);
    return x;
}

void bo_encode_le32(uint8_t buf[4], uint32_t v) {
    buf[0] = (uint8_t)(v & 0xFF);
    buf[1] = (uint8_t)((v >> 8) & 0xFF);
    buf[2] = (uint8_t)((v >> 16) & 0xFF);
    buf[3] = (uint8_t)((v >> 24) & 0xFF);
}

uint32_t bo_decode_le32(const uint8_t buf[4]) {
    return  (uint32_t)buf[3]
         | ((uint32_t)buf[2] << 8)
         | ((uint32_t)buf[1] << 16)
         | ((uint32_t)buf[0] << 24);
}

uint32_t bo_rotl32(uint32_t x, uint32_t n) {
    n &= 31u;
    if (n == 0) return x;
    return (x << n) | (x >> (32 - n));
}

uint32_t bo_rotr32(uint32_t x, uint32_t n) {
    n &= 31u;
    if (n == 0) return x;
    return (x >> n) | (x << (32 - n));
}
