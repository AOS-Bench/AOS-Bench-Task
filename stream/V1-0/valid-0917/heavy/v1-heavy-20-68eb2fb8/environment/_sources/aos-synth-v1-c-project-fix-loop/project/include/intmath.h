#ifndef LIBCOMPUTE_INTMATH_H
#define LIBCOMPUTE_INTMATH_H

#include <stdint.h>
#include <stddef.h>

/* Greatest common divisor (Euclid's algorithm). Non-negative operands only. */
uint64_t im_gcd(uint64_t a, uint64_t b);

/* Least common multiple. Returns 0 if a==0 or b==0. */
uint64_t im_lcm(uint64_t a, uint64_t b);

/* Modular exponentiation: (base^exp) mod m. m>0 assumed. */
uint64_t im_modpow(uint64_t base, uint64_t exp, uint64_t m);

/* Sieve of Eratosthenes: fills `is_prime` with 1 for primes 2..n-1, else 0.
 * `is_prime` must have room for n entries. Sets is_prime[0]=is_prime[1]=0. */
void im_sieve(uint8_t *is_prime, uint64_t n);

/* Count primes strictly below n using im_sieve. */
uint64_t im_count_primes_below(uint64_t n);

/* Nth Fibonacci number, mod 2^64 (wrap-around is fine for our tests). */
uint64_t im_fib(uint64_t n);

#endif
