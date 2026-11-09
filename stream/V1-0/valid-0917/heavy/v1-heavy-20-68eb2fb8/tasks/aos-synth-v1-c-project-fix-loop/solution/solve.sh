#!/bin/bash
# Oracle solve for c-project-fix-loop.
#
# Applies the exact set of source patches that make `make test` return
# 8/8 suites passing, then rebuilds and confirms the test binary is green.
# Each sed pattern is anchored on a unique enough substring to avoid false
# matches; if any pattern misses, the follow-up `run_tests` will fail and
# test.sh will grade 0.
set -euo pipefail
cd /workspace/project

echo "[oracle] applying 8 fixes to src/"

# 1. intmath.c — im_fib off-by-one: loop starts at i=0 instead of i=1
sed -i 's|for (uint64_t i = 0; i < n; i++) {|for (uint64_t i = 1; i < n; i++) {|' src/intmath.c

# 2. bitops.c — bo_decode_le32 uses big-endian byte order instead of LE
sed -i 's|(uint32_t)buf\[3\]$|(uint32_t)buf[0]|'                   src/bitops.c
sed -i 's|((uint32_t)buf\[2\] << 8)|((uint32_t)buf[1] << 8)|'      src/bitops.c
sed -i 's|((uint32_t)buf\[1\] << 16)|((uint32_t)buf[2] << 16)|'    src/bitops.c
sed -i 's|((uint32_t)buf\[0\] << 24);|((uint32_t)buf[3] << 24);|'  src/bitops.c

# 3. strutil.c — su_common_prefix_len starts counting at 1 instead of 0
sed -i 's|    size_t n = 1;|    size_t n = 0;|' src/strutil.c

# 4. hash.c — FNV-1a offset basis is one less than the canonical value
sed -i 's|0x811C9DC4u|0x811C9DC5u|' src/hash.c

# 5. sort.c — partition comparator inverted (sorts descending)
sed -i 's|arr\[j\] > pivot|arr[j] <= pivot|' src/sort.c

# 6. heap.c — sift_up uses 1-indexed parent formula on a 0-indexed heap
sed -i 's|size_t p = i / 2;|size_t p = (i - 1) / 2;|' src/heap.c

# 7. matrix.c — mat_mul indexes b with b->rows stride instead of b->cols
sed -i 's|b->data\[k \* b->rows + j\]|b->data[k * b->cols + j]|' src/matrix.c

# 8. list.c — list_pop reads data[size-1] AFTER decrementing size
sed -i 's|\*out = l->data\[l->size - 1\];|*out = l->data[l->size];|' src/list.c

echo "[oracle] building"
make clean >/dev/null
make -j"$(nproc)" 2>&1 | tail -5

echo "[oracle] running test binary"
./bin/run_tests
echo "[oracle] done"
