#ifndef LIBCOMPUTE_STRESS_PARALLEL_H
#define LIBCOMPUTE_STRESS_PARALLEL_H

#include <stdint.h>

/* Fire N pthread workers that hammer hash/sort/sieve routines respectively,
 * with `iters_per_thread` iterations each. Returns a xor/sum accumulator
 * across workers so the optimizer can't drop the work. */
uint64_t stress_hash_parallel(int n_threads, uint32_t iters_per_thread);
uint64_t stress_sort_parallel(int n_threads, uint32_t iters_per_thread);
uint64_t stress_sieve_parallel(int n_threads, uint32_t iters_per_thread);

#endif
