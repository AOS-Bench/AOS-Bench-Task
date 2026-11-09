#ifndef LIBCOMPUTE_HASH_H
#define LIBCOMPUTE_HASH_H

#include <stdint.h>
#include <stddef.h>

/* djb2 hash (32-bit). Well-known constants:
 *   initial state = 5381
 *   step: h = h*33 + c
 */
uint32_t hash_djb2(const void *buf, size_t len);

/* FNV-1a hash (32-bit). Well-known constants:
 *   offset = 2166136261u (0x811C9DC5)
 *   prime  = 16777619u
 * Per-byte: h ^= b; h *= prime;
 */
uint32_t hash_fnv1a(const void *buf, size_t len);

/* CRC-32 (IEEE / zlib polynomial 0xEDB88320, initial 0xFFFFFFFF, final xor
 * 0xFFFFFFFF). Matches `python3 -c 'import zlib; print(zlib.crc32(...))'`. */
uint32_t hash_crc32(const void *buf, size_t len);

#endif
