/* hash.c — non-cryptographic hashes */
#include "hash.h"

uint32_t hash_djb2(const void *buf, size_t len) {
    const unsigned char *p = (const unsigned char *)buf;
    uint32_t h = 5381u;
    for (size_t i = 0; i < len; i++) {
        h = h * 33u + p[i];
    }
    return h;
}

uint32_t hash_fnv1a(const void *buf, size_t len) {
    const unsigned char *p = (const unsigned char *)buf;
    uint32_t h = 0x811C9DC4u;
    for (size_t i = 0; i < len; i++) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

static uint32_t crc32_table[256];
static int crc32_table_init = 0;

static void hash_crc32_init(void) {
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int k = 0; k < 8; k++) {
            c = (c & 1u) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
        }
        crc32_table[i] = c;
    }
    crc32_table_init = 1;
}

uint32_t hash_crc32(const void *buf, size_t len) {
    if (!crc32_table_init) hash_crc32_init();
    const unsigned char *p = (const unsigned char *)buf;
    uint32_t c = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; i++) {
        c = crc32_table[(c ^ p[i]) & 0xFFu] ^ (c >> 8);
    }
    return c ^ 0xFFFFFFFFu;
}
