/* main.c — test harness for libcompute
 *
 * Runs 8 test suites (one per module). Each suite prints:
 *   BEGIN <suite>
 *   ...checks...
 *   END <suite> PASS   (all checks succeeded)
 * or
 *   END <suite> FAIL   (>=1 check failed)
 *
 * Exit status is 0 if all suites passed, 1 otherwise. The final
 * summary line is `RESULT: <pass>/8 suites passed`.
 *
 * Each suite mixes lightweight correctness checks with a heavier
 * stress phase (larger inputs) so the whole binary takes ~30-90s of
 * CPU on 4 cores.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <stdint.h>
#include <limits.h>

#include "intmath.h"
#include "bitops.h"
#include "strutil.h"
#include "hash.h"
#include "sort.h"
#include "heap.h"
#include "matrix.h"
#include "list.h"
#include "stress_parallel.h"

static int g_suite_ok;
static int g_total_checks;
static int g_failed_checks;

#define CHECK(cond) do {                                                \
    g_total_checks++;                                                   \
    if (!(cond)) {                                                      \
        g_suite_ok = 0;                                                 \
        g_failed_checks++;                                              \
        fprintf(stderr, "  FAIL %s:%d %s\n", __FILE__, __LINE__, #cond);\
    }                                                                   \
} while (0)

#define CHECK_EQ_U64(a, b) do {                                          \
    uint64_t _a = (uint64_t)(a), _b = (uint64_t)(b);                     \
    g_total_checks++;                                                    \
    if (_a != _b) {                                                      \
        g_suite_ok = 0;                                                  \
        g_failed_checks++;                                                \
        fprintf(stderr, "  FAIL %s:%d %s == %s   got=%" PRIu64 " want=%" PRIu64 "\n", \
                __FILE__, __LINE__, #a, #b, _a, _b);                     \
    }                                                                    \
} while (0)

#define CHECK_EQ_S32(a, b) do {                                          \
    int32_t _a = (int32_t)(a), _b = (int32_t)(b);                        \
    g_total_checks++;                                                    \
    if (_a != _b) {                                                      \
        g_suite_ok = 0;                                                  \
        g_failed_checks++;                                                \
        fprintf(stderr, "  FAIL %s:%d %s == %s   got=%" PRId32 " want=%" PRId32 "\n", \
                __FILE__, __LINE__, #a, #b, _a, _b);                     \
    }                                                                    \
} while (0)

static int run_suite(const char *name, void (*fn)(void)) {
    g_suite_ok = 1;
    fprintf(stdout, "BEGIN %s\n", name);
    fflush(stdout);
    fn();
    fprintf(stdout, "END %s %s\n", name, g_suite_ok ? "PASS" : "FAIL");
    fflush(stdout);
    return g_suite_ok;
}

/* ------------------------- suite 1: intmath ---------------------------- */
static void suite_intmath(void) {
    CHECK_EQ_U64(im_gcd(12, 18), 6);
    CHECK_EQ_U64(im_gcd(17, 5), 1);
    CHECK_EQ_U64(im_gcd(100, 0), 100);
    CHECK_EQ_U64(im_lcm(4, 6), 12);
    CHECK_EQ_U64(im_lcm(7, 3), 21);

    CHECK_EQ_U64(im_modpow(2, 10, 1000), 24);
    CHECK_EQ_U64(im_modpow(3, 200, 1000000007ull), 136318165ull);

    /* Fibonacci: catches im_fib off-by-one */
    CHECK_EQ_U64(im_fib(1), 1);
    CHECK_EQ_U64(im_fib(2), 1);
    CHECK_EQ_U64(im_fib(10), 55);
    CHECK_EQ_U64(im_fib(20), 6765);
    CHECK_EQ_U64(im_fib(50), 12586269025ull);

    /* Prime counting */
    CHECK_EQ_U64(im_count_primes_below(10), 4);       /* 2,3,5,7 */
    CHECK_EQ_U64(im_count_primes_below(30), 10);
    CHECK_EQ_U64(im_count_primes_below(100), 25);

    /* Stress: modpow + prime count. Runs on all cores if OpenMP were used;
     * here plain single-thread but with big iteration count. */
    uint64_t acc = 0;
    for (uint64_t rep = 0; rep < 50; rep++) {
        for (uint64_t i = 1; i <= 200000ull; i++) {
            acc += im_modpow(i, 65537ull, 1000000007ull);
        }
    }
    /* We don't assert the exact value (that would be a shortcut for the
     * agent); just make sure the loop is not optimized away. */
    if (acc == 12345) fputs("", stderr);

    /* Sieve stress: build a big sieve, verify π(N) at a few N. */
    CHECK_EQ_U64(im_count_primes_below(1000000ull), 78498ull);
    CHECK_EQ_U64(im_count_primes_below(10000000ull), 664579ull);
}

/* ------------------------- suite 2: bitops ----------------------------- */
static void suite_bitops(void) {
    CHECK_EQ_U64(bo_popcount(0), 0);
    CHECK_EQ_U64(bo_popcount(0xFFull), 8);
    CHECK_EQ_U64(bo_popcount(0xF0F0F0F0F0F0F0F0ull), 32);
    CHECK_EQ_U64(bo_popcount(0xDEADBEEFCAFEBABEull), 46);

    CHECK_EQ_U64(bo_reverse32(0x00000001u), 0x80000000u);
    CHECK_EQ_U64(bo_reverse32(0xAAAAAAAAu), 0x55555555u);
    CHECK_EQ_U64(bo_reverse32(0x12345678u), 0x1E6A2C48u);

    /* Round-trip encode/decode: catches bo_decode_le32 endianness bug */
    uint8_t buf[4];
    bo_encode_le32(buf, 0x12345678u);
    CHECK_EQ_U64(buf[0], 0x78);
    CHECK_EQ_U64(buf[1], 0x56);
    CHECK_EQ_U64(buf[2], 0x34);
    CHECK_EQ_U64(buf[3], 0x12);
    CHECK_EQ_U64(bo_decode_le32(buf), 0x12345678u);

    uint8_t fixed[4] = {0xEF, 0xBE, 0xAD, 0xDE};
    CHECK_EQ_U64(bo_decode_le32(fixed), 0xDEADBEEFu);

    CHECK_EQ_U64(bo_rotl32(0x00000001u, 1), 0x00000002u);
    CHECK_EQ_U64(bo_rotl32(0x80000000u, 1), 0x00000001u);
    CHECK_EQ_U64(bo_rotr32(0x00000001u, 1), 0x80000000u);

    /* Stress: hash-like mixing over a lot of bytes to keep the CPU busy. */
    uint32_t mix = 0xCAFEBABEu;
    for (uint32_t rep = 0; rep < 30; rep++) {
        for (uint32_t i = 0; i < 4000000u; i++) {
            mix ^= bo_rotl32(mix + i, (i & 31u));
            mix += bo_popcount((uint64_t)mix);
        }
    }
    if (mix == 0) fputs("", stderr);
}

/* ------------------------- suite 3: strutil ---------------------------- */
static void suite_strutil(void) {
    char *d = su_strdup("hello");
    CHECK(d != NULL);
    CHECK(strcmp(d, "hello") == 0);
    free(d);

    char buf[64];
    CHECK_EQ_U64(su_trim("  hi  ", buf), 2);
    CHECK(strcmp(buf, "hi") == 0);
    CHECK_EQ_U64(su_trim("\t\nfoo bar \n", buf), 7);
    CHECK(strcmp(buf, "foo bar") == 0);

    CHECK(su_icmp("abc", "ABC") == 0);
    CHECK(su_icmp("abc", "abd") < 0);
    CHECK(su_icmp("abcd", "abc") > 0);

    char r[] = "abcdef";
    su_reverse(r);
    CHECK(strcmp(r, "fedcba") == 0);

    /* Common prefix: catches su_common_prefix_len bug where the loop
     * skips index 0. */
    CHECK_EQ_U64(su_common_prefix_len("hello", "help"), 3);
    CHECK_EQ_U64(su_common_prefix_len("cat", "cow"), 1);
    CHECK_EQ_U64(su_common_prefix_len("abc", "xyz"), 0);
    CHECK_EQ_U64(su_common_prefix_len("", "hello"), 0);
    CHECK_EQ_U64(su_common_prefix_len("same", "same"), 4);

    /* Stress: exercise trim + reverse on many strings. */
    char scratch[128];
    for (int i = 0; i < 5000000; i++) {
        snprintf(scratch, sizeof(scratch), "  test string %d  ", i);
        char tmp[128];
        su_trim(scratch, tmp);
        su_reverse(tmp);
    }
}

/* ------------------------- suite 4: hash ------------------------------- */
static void suite_hash(void) {
    /* Well-known reference values from independent implementations. */
    CHECK_EQ_U64(hash_djb2("", 0), 5381u);
    CHECK_EQ_U64(hash_djb2("a", 1), 177670u);
    CHECK_EQ_U64(hash_djb2("hello", 5), 0x0F923099u);

    /* FNV-1a offset basis: catches hash_fnv1a wrong-constant bug */
    CHECK_EQ_U64(hash_fnv1a("", 0), 0x811C9DC5u);
    CHECK_EQ_U64(hash_fnv1a("a", 1), 0xE40C292Cu);
    CHECK_EQ_U64(hash_fnv1a("foobar", 6), 0xBF9CF968u);

    /* CRC-32 (matches python zlib.crc32) */
    CHECK_EQ_U64(hash_crc32("", 0), 0u);
    CHECK_EQ_U64(hash_crc32("123456789", 9), 0xCBF43926u);
    CHECK_EQ_U64(hash_crc32("The quick brown fox jumps over the lazy dog", 43),
                 0x414FA339u);

    /* Stress: hash an 8 MiB buffer several times. */
    const size_t N = 8u * 1024u * 1024u;
    unsigned char *buf = (unsigned char *)malloc(N);
    CHECK(buf != NULL);
    if (buf) {
        for (size_t i = 0; i < N; i++) buf[i] = (unsigned char)(i * 2654435761u);
        volatile uint32_t sink = 0;
        for (int rep = 0; rep < 30; rep++) {
            sink ^= hash_djb2(buf, N);
            sink ^= hash_fnv1a(buf, N);
            sink ^= hash_crc32(buf, N);
        }
        (void)sink;
        free(buf);
    }
}

/* ------------------------- suite 5: sort ------------------------------- */
static int32_t xorshift32(uint32_t *s) {
    uint32_t x = *s;
    x ^= x << 13; x ^= x >> 17; x ^= x << 5;
    *s = x;
    return (int32_t)x;
}

static void suite_sort(void) {
    /* Small cases: catches so_qsort comparator sign bug */
    int32_t a[] = {3, 1, 2};
    so_qsort(a, 3);
    CHECK_EQ_S32(a[0], 1);
    CHECK_EQ_S32(a[1], 2);
    CHECK_EQ_S32(a[2], 3);

    int32_t b[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    so_qsort(b, 10);
    for (int i = 0; i < 10; i++) CHECK_EQ_S32(b[i], i);

    int32_t c[] = {5, 2, 8, 1, 9, 3, 7, 4, 6, 0};
    CHECK(so_msort(c, 10) == 0);
    for (int i = 0; i < 10; i++) CHECK_EQ_S32(c[i], i);

    CHECK(so_is_sorted(b, 10) == 1);
    int32_t nb[] = {1, 3, 2};
    CHECK(so_is_sorted(nb, 3) == 0);

    /* Stress: sort a 2M-element random array multiple times. */
    const size_t N = 2000000;
    int32_t *big = (int32_t *)malloc(N * sizeof(int32_t));
    CHECK(big != NULL);
    if (big) {
        uint32_t state = 0xDEADBEEF;
        for (int rep = 0; rep < 5; rep++) {
            for (size_t i = 0; i < N; i++) big[i] = xorshift32(&state);
            so_qsort(big, N);
            CHECK(so_is_sorted(big, N) == 1);
        }
        for (int rep = 0; rep < 5; rep++) {
            for (size_t i = 0; i < N; i++) big[i] = xorshift32(&state);
            CHECK(so_msort(big, N) == 0);
            CHECK(so_is_sorted(big, N) == 1);
        }
        free(big);
    }
}

/* ------------------------- suite 6: heap ------------------------------- */
static void suite_heap(void) {
    MaxHeap *h = heap_new(16);
    CHECK(h != NULL);
    if (!h) return;

    int32_t input[] = {1, 5, 2, 8, 3, 7, 4, 6};
    for (int i = 0; i < 8; i++) CHECK(heap_push(h, input[i]) == 0);
    CHECK_EQ_U64(heap_size(h), 8);

    /* Catches heap sift_up parent-index bug */
    int32_t expected[] = {8, 7, 6, 5, 4, 3, 2, 1};
    for (int i = 0; i < 8; i++) {
        int32_t v = -12345;
        CHECK(heap_pop(h, &v) == 0);
        CHECK_EQ_S32(v, expected[i]);
    }
    CHECK_EQ_U64(heap_size(h), 0);

    heap_free(h);

    /* Stress: push+pop 2M random ints multiple times; verify pops are
     * non-increasing (depends on a correct heap). */
    const size_t N = 2000000;
    MaxHeap *h2 = heap_new(N);
    CHECK(h2 != NULL);
    if (h2) {
        for (int rep = 0; rep < 5; rep++) {
            h2->size = 0;
            uint32_t st = 0xF00DBABE + (uint32_t)rep;
            for (size_t i = 0; i < N; i++) heap_push(h2, xorshift32(&st));
            int32_t prev = INT32_MAX, cur;
            int ok = 1;
            for (size_t i = 0; i < N; i++) {
                heap_pop(h2, &cur);
                if (cur > prev) { ok = 0; break; }
                prev = cur;
            }
            CHECK(ok == 1);
        }
        heap_free(h2);
    }
}

/* ------------------------- suite 7: matrix ----------------------------- */
static void suite_matrix(void) {
    /* 2x3 * 3x2 = 2x2 — asymmetric shape catches mat_mul stride bug */
    Matrix *A = mat_new(2, 3);
    Matrix *B = mat_new(3, 2);
    Matrix *C = mat_new(2, 2);
    CHECK(A && B && C);
    if (!A || !B || !C) goto done;

    /* A = [[1,2,3],[4,5,6]] */
    mat_set(A, 0, 0, 1); mat_set(A, 0, 1, 2); mat_set(A, 0, 2, 3);
    mat_set(A, 1, 0, 4); mat_set(A, 1, 1, 5); mat_set(A, 1, 2, 6);
    /* B = [[7,8],[9,10],[11,12]] */
    mat_set(B, 0, 0, 7);  mat_set(B, 0, 1, 8);
    mat_set(B, 1, 0, 9);  mat_set(B, 1, 1, 10);
    mat_set(B, 2, 0, 11); mat_set(B, 2, 1, 12);

    CHECK(mat_mul(A, B, C) == 0);
    /* Expected A*B = [[58,64],[139,154]] */
    CHECK(fabs(mat_get(C, 0, 0) - 58.0)  < 1e-9);
    CHECK(fabs(mat_get(C, 0, 1) - 64.0)  < 1e-9);
    CHECK(fabs(mat_get(C, 1, 0) - 139.0) < 1e-9);
    CHECK(fabs(mat_get(C, 1, 1) - 154.0) < 1e-9);

    /* Identity check */
    Matrix *I = mat_new(3, 3);
    Matrix *M = mat_new(3, 3);
    Matrix *R = mat_new(3, 3);
    if (I && M && R) {
        mat_set(I, 0, 0, 1); mat_set(I, 1, 1, 1); mat_set(I, 2, 2, 1);
        double v = 1.0;
        for (size_t i = 0; i < 3; i++) for (size_t j = 0; j < 3; j++) mat_set(M, i, j, v++);
        CHECK(mat_mul(M, I, R) == 0);
        for (size_t i = 0; i < 3; i++)
            for (size_t j = 0; j < 3; j++)
                CHECK(fabs(mat_get(R, i, j) - mat_get(M, i, j)) < 1e-9);
    }
    mat_free(I); mat_free(M); mat_free(R);

    /* Stress: multiply two 400x400 matrices several times. */
    const size_t D = 400;
    Matrix *X = mat_new(D, D);
    Matrix *Y = mat_new(D, D);
    Matrix *Z = mat_new(D, D);
    if (X && Y && Z) {
        for (size_t i = 0; i < D; i++)
            for (size_t j = 0; j < D; j++) {
                mat_set(X, i, j, (double)((i * 13 + j * 7) % 17));
                mat_set(Y, i, j, (double)((i * 5  + j * 11) % 19));
            }
        for (int rep = 0; rep < 30; rep++) {
            mat_mul(X, Y, Z);
        }
        volatile double s = mat_frobenius(Z);
        (void)s;
    }
    mat_free(X); mat_free(Y); mat_free(Z);

done:
    mat_free(A); mat_free(B); mat_free(C);
}

/* ------------------------- suite 8: list ------------------------------- */
static void suite_list(void) {
    List l; list_init(&l);
    CHECK_EQ_U64(list_size(&l), 0);

    for (int32_t i = 0; i < 100; i++) CHECK(list_push(&l, i * 10) == 0);
    CHECK_EQ_U64(list_size(&l), 100);
    CHECK_EQ_S32(list_get(&l, 0), 0);
    CHECK_EQ_S32(list_get(&l, 50), 500);
    CHECK_EQ_S32(list_get(&l, 99), 990);

    /* Pop order should be LIFO — catches list_pop off-by-one bug */
    int32_t v;
    CHECK(list_pop(&l, &v) == 0); CHECK_EQ_S32(v, 990);
    CHECK(list_pop(&l, &v) == 0); CHECK_EQ_S32(v, 980);
    CHECK(list_pop(&l, &v) == 0); CHECK_EQ_S32(v, 970);
    CHECK_EQ_U64(list_size(&l), 97);

    /* Pop the rest and check LIFO order all the way down */
    for (int32_t i = 96; i >= 0; i--) {
        CHECK(list_pop(&l, &v) == 0);
        CHECK_EQ_S32(v, i * 10);
    }
    CHECK_EQ_U64(list_size(&l), 0);
    CHECK(list_pop(&l, &v) == -1);
    list_free(&l);

    /* Stress: push 5M ints multiple times, pop them all. */
    for (int rep = 0; rep < 8; rep++) {
        List big; list_init(&big);
        const int32_t N = 5000000;
        for (int32_t i = 0; i < N; i++) CHECK(list_push(&big, i) == 0);
        CHECK_EQ_U64(list_size(&big), (uint64_t)N);
        int32_t got;
        int ok = 1;
        for (int32_t i = N - 1; i >= 0; i--) {
            list_pop(&big, &got);
            if (got != i) { ok = 0; break; }
        }
        CHECK(ok == 1);
        list_free(&big);
    }
}

/* ------------------------- main --------------------------------------- */
int main(void) {
    struct { const char *name; void (*fn)(void); } suites[] = {
        {"intmath", suite_intmath},
        {"bitops",  suite_bitops},
        {"strutil", suite_strutil},
        {"hash",    suite_hash},
        {"sort",    suite_sort},
        {"heap",    suite_heap},
        {"matrix",  suite_matrix},
        {"list",    suite_list},
    };
    int n_suites = (int)(sizeof(suites) / sizeof(suites[0]));
    int passed = 0;
    for (int i = 0; i < n_suites; i++) {
        if (run_suite(suites[i].name, suites[i].fn)) passed++;
    }

    /* End-of-suite parallel stress that saturates all container CPUs. This
     * exercises the same primitives the correctness suites just checked;
     * a wrong hash/sort/sieve would either wedge here or return garbage
     * that we xor into a sink (kept live via `volatile`). No CHECK() calls
     * — the correctness of these primitives is already gated above. */
    if (passed == n_suites) {
        volatile uint64_t sink = 0;
        fprintf(stdout, "BEGIN parallel_stress\n"); fflush(stdout);
        sink ^= stress_hash_parallel(4, 10000);   /* 4 workers × N iters of 512KiB */
        sink ^= stress_sort_parallel(4, 5000);    /* 4 workers × 5000 sorts of 200k */
        sink ^= stress_sieve_parallel(4, 2500);   /* 4 workers × 2500 sieves of ~1e6 */
        fprintf(stdout, "END parallel_stress PASS  sink=%" PRIu64 "\n", sink);
        fflush(stdout);
    }

    fprintf(stdout, "RESULT: %d/%d suites passed  (checks: %d total, %d failed)\n",
            passed, n_suites, g_total_checks, g_failed_checks);
    return passed == n_suites ? 0 : 1;
}
