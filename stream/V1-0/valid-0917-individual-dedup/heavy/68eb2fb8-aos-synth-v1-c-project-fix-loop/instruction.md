# C Project Fix Loop

A small C library — **`libcompute`** — has been baked into
`/workspace/project/`. It provides 8 modules (integer math, bit ops, string
utilities, hashes, sorting, a max-heap, a matrix type, and a growable
list) exercised by a single test harness at `bin/run_tests`.

The source is **buggy**. Multiple regressions have been introduced into
different modules; each one causes at least one test suite to fail.
Your job is to find and fix them so the full test suite passes.

## Deliverable

Edit files under `/workspace/project/src/` and `/workspace/project/include/` so
that:

```
cd /workspace/project
make clean && make -j"$(nproc)"
./bin/run_tests
```

reports **at least 7 of 8** correctness suites passing on the final
`RESULT:` line. Full 8/8 is preferable — the last suite triggers a
parallel-stress phase that is a strong smoke test for the primitives.

## Rules

- Container: 4 CPU / 8 GiB RAM, 5400 s agent budget.
- Preinstalled: `gcc`, `make`, `libc6-dev`, coreutils. `apt-get`
  works, but no internet access after the container starts.
- **You may not** cheat by rewriting `bin/run_tests`, editing
  `src/main.c` to skip suites, or short-circuiting the CHECK macros.
  The verifier rebuilds from your source and re-runs the tests. It
  greps for `^END <suite> PASS$` lines from your test binary; forging
  those lines by editing `main.c`'s printf strings is easy to spot in
  a code review, so any suite whose PASS is not backed by real fixes
  will be considered a failure at review time even if the automated
  grade lets it through this run.
- Modules are independent enough that fixes are localized: expect each
  bug to be a **1–5 line change** in one file.
- The build is fast (< 5 s on 4 cores); the test binary takes ≈ 1 min
  wall-clock on a fully-correct build. Budget a good number of edit /
  build / test iterations.

## Getting started

```
cd /workspace/project
cat README.md
make -j"$(nproc)"
./bin/run_tests 2>&1 | tail -60
```

Look for `FAIL src/main.c:LINE ...` messages — they name the exact
CHECK expression and the got-vs-want values, which usually points
directly at the module and function that need to be fixed.

## What the verifier does

1. `cd /workspace/project && make clean && make -j"$(nproc)"` — must succeed.
2. `./bin/run_tests` — count `END <name> PASS` lines.
3. `reward = 1` iff ≥ 7 correctness suites pass, else `0`.
