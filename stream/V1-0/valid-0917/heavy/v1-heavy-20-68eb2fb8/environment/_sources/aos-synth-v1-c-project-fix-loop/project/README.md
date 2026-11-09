# libcompute

A small C library of numeric and data-structure primitives, exercised
by an 8-suite test harness (`bin/run_tests`).

## Build & test

```
make            # build bin/run_tests
make test       # build + run the test suite
make clean      # remove build artifacts
```

## Modules

| module   | header               | notes                                        |
|----------|----------------------|----------------------------------------------|
| intmath  | include/intmath.h    | gcd/lcm/modpow, prime sieve, fibonacci       |
| bitops   | include/bitops.h     | popcount, bit-reverse, LE encode/decode, rot |
| strutil  | include/strutil.h    | strdup / trim / icmp / reverse / prefix-len  |
| hash     | include/hash.h       | djb2, FNV-1a, CRC-32 (zlib polynomial)       |
| sort     | include/sort.h       | Lomuto quicksort, merge sort                 |
| heap     | include/heap.h       | fixed-capacity max-heap of int32_t           |
| matrix   | include/matrix.h     | row-major dense double matrix ops            |
| list     | include/list.h       | growable int32_t list                        |

## Test format

`bin/run_tests` prints one block per suite:

```
BEGIN <name>
  FAIL src/main.c:LINE some_expr == other_expr   got=X want=Y
END <name> {PASS|FAIL}
```

and finishes with a summary line:

```
RESULT: <passed>/8 suites passed  (checks: <total> total, <failed> failed)
```

Exit status is 0 iff all 8 suites pass, otherwise 1.
