/* intmath.c — integer number theory primitives */
#include "intmath.h"
#include <stdlib.h>
#include <string.h>

uint64_t im_gcd(uint64_t a, uint64_t b) {
    while (b != 0) {
        uint64_t t = a % b;
        a = b;
        b = t;
    }
    return a;
}

uint64_t im_lcm(uint64_t a, uint64_t b) {
    if (a == 0 || b == 0) return 0;
    return (a / im_gcd(a, b)) * b;
}

uint64_t im_modpow(uint64_t base, uint64_t exp, uint64_t m) {
    if (m == 1) return 0;
    __uint128_t result = 1;
    __uint128_t b = base % m;
    while (exp > 0) {
        if (exp & 1) {
            result = (result * b) % m;
        }
        b = (b * b) % m;
        exp >>= 1;
    }
    return (uint64_t)result;
}

void im_sieve(uint8_t *is_prime, uint64_t n) {
    if (n == 0) return;
    memset(is_prime, 1, (size_t)n);
    if (n >= 1) is_prime[0] = 0;
    if (n >= 2) is_prime[1] = 0;
    for (uint64_t i = 2; i * i < n; i++) {
        if (is_prime[i]) {
            for (uint64_t j = i * i; j < n; j += i) {
                is_prime[j] = 0;
            }
        }
    }
}

uint64_t im_count_primes_below(uint64_t n) {
    if (n <= 2) return 0;
    uint8_t *buf = (uint8_t *)malloc((size_t)n);
    if (!buf) return 0;
    im_sieve(buf, n);
    uint64_t c = 0;
    for (uint64_t i = 2; i < n; i++) c += buf[i];
    free(buf);
    return c;
}

uint64_t im_fib(uint64_t n) {
    /* Iteratively compute fib(n): fib(0)=0, fib(1)=1, fib(n)=fib(n-1)+fib(n-2). */
    if (n == 0) return 0;
    uint64_t a = 0, b = 1;
    for (uint64_t i = 0; i < n; i++) {
        uint64_t t = a + b;
        a = b;
        b = t;
    }
    return b;
}
