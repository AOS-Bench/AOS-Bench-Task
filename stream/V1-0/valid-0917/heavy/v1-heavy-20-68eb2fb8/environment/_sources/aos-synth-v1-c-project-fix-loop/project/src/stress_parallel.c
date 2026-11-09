/* stress_parallel.c — pthread-parallel CPU-hot stress used by main.c.
 *
 * The main test loop is single-threaded, but a few of the heavier suites
 * (hash, sort, intmath) spin up a small thread pool to keep all container
 * CPUs busy — otherwise a 4-CPU cgroup would see roughly N wall-seconds
 * of CPU per second rather than 4N.
 */
#include "stress_parallel.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "hash.h"
#include "intmath.h"
#include "sort.h"

/* Simple xorshift32 (duplicated here so we don't need to expose main.c's
 * static helper). */
static int32_t xs32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return (int32_t)x;
}

typedef struct {
    uint32_t seed;
    uint32_t iters;
    uint64_t sink;    /* out */
} hash_arg_t;

static void *hash_worker(void *p) {
    hash_arg_t *a = (hash_arg_t *)p;
    const size_t N = 512 * 1024;
    unsigned char *buf = (unsigned char *)malloc(N);
    if (!buf) return NULL;
    for (size_t i = 0; i < N; i++) buf[i] = (unsigned char)(i * a->seed);
    uint64_t s = 0;
    for (uint32_t r = 0; r < a->iters; r++) {
        s ^= hash_djb2(buf, N);
        s ^= hash_fnv1a(buf, N);
        s ^= hash_crc32(buf, N);
        buf[r % N] ^= (unsigned char)(r + a->seed);
    }
    free(buf);
    a->sink = s;
    return NULL;
}

typedef struct {
    uint32_t seed;
    uint32_t iters;
    uint64_t sink;
} sort_arg_t;

static void *sort_worker(void *p) {
    sort_arg_t *a = (sort_arg_t *)p;
    const size_t N = 200000;
    int32_t *arr = (int32_t *)malloc(N * sizeof(int32_t));
    if (!arr) return NULL;
    uint32_t st = a->seed;
    uint64_t s = 0;
    for (uint32_t r = 0; r < a->iters; r++) {
        for (size_t i = 0; i < N; i++) arr[i] = xs32(&st);
        so_qsort(arr, N);
        s += (uint64_t)arr[N - 1] - (uint64_t)arr[0];
    }
    free(arr);
    a->sink = s;
    return NULL;
}

typedef struct {
    uint32_t seed;
    uint32_t iters;
    uint64_t sink;
} sieve_arg_t;

static void *sieve_worker(void *p) {
    sieve_arg_t *a = (sieve_arg_t *)p;
    uint64_t s = 0;
    for (uint32_t r = 0; r < a->iters; r++) {
        s += im_count_primes_below(1000000ull + a->seed + r);
    }
    a->sink = s;
    return NULL;
}

uint64_t stress_hash_parallel(int n_threads, uint32_t iters_per_thread) {
    pthread_t th[16];
    hash_arg_t args[16];
    if (n_threads > 16) n_threads = 16;
    for (int i = 0; i < n_threads; i++) {
        args[i].seed = 2654435761u + (uint32_t)i * 0x9E3779B9u;
        args[i].iters = iters_per_thread;
        args[i].sink = 0;
        pthread_create(&th[i], NULL, hash_worker, &args[i]);
    }
    uint64_t agg = 0;
    for (int i = 0; i < n_threads; i++) {
        pthread_join(th[i], NULL);
        agg ^= args[i].sink;
    }
    return agg;
}

uint64_t stress_sort_parallel(int n_threads, uint32_t iters_per_thread) {
    pthread_t th[16];
    sort_arg_t args[16];
    if (n_threads > 16) n_threads = 16;
    for (int i = 0; i < n_threads; i++) {
        args[i].seed = 0xDEADBEEF + (uint32_t)i * 0x1337BEEFu;
        args[i].iters = iters_per_thread;
        args[i].sink = 0;
        pthread_create(&th[i], NULL, sort_worker, &args[i]);
    }
    uint64_t agg = 0;
    for (int i = 0; i < n_threads; i++) {
        pthread_join(th[i], NULL);
        agg += args[i].sink;
    }
    return agg;
}

uint64_t stress_sieve_parallel(int n_threads, uint32_t iters_per_thread) {
    pthread_t th[16];
    sieve_arg_t args[16];
    if (n_threads > 16) n_threads = 16;
    for (int i = 0; i < n_threads; i++) {
        args[i].seed = (uint32_t)i * 1000u;
        args[i].iters = iters_per_thread;
        args[i].sink = 0;
        pthread_create(&th[i], NULL, sieve_worker, &args[i]);
    }
    uint64_t agg = 0;
    for (int i = 0; i < n_threads; i++) {
        pthread_join(th[i], NULL);
        agg += args[i].sink;
    }
    return agg;
}
