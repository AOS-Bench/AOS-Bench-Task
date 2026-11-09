#ifndef LIBCOMPUTE_BITOPS_H
#define LIBCOMPUTE_BITOPS_H

#include <stdint.h>
#include <stddef.h>

/* Population count (number of set bits) in x. */
uint32_t bo_popcount(uint64_t x);

/* Reverse the bit order of a 32-bit word. */
uint32_t bo_reverse32(uint32_t x);

/* Encode value v as little-endian 4 bytes into buf. */
void bo_encode_le32(uint8_t buf[4], uint32_t v);

/* Decode 4 little-endian bytes from buf into a uint32_t. */
uint32_t bo_decode_le32(const uint8_t buf[4]);

/* Rotate x left by n (n in 0..31). */
uint32_t bo_rotl32(uint32_t x, uint32_t n);

/* Rotate x right by n (n in 0..31). */
uint32_t bo_rotr32(uint32_t x, uint32_t n);

#endif
