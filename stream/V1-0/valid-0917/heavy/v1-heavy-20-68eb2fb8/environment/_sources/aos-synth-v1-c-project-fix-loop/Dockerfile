# AOS-Bench synth-v1 · c-project-fix-loop
#
# A synthetic 8-module C project ("libcompute") baked into /app/project.
# The source has been intentionally patched with a set of bugs; each bug
# breaks one test suite. Agent must find & fix all bugs, then `make test`
# should report 8/8 suites passing.
#
# The agent iterates a build-test-edit loop many times. `bin/run_tests`
# ends with a parallel-stress phase (pthreads across container CPUs) that
# only runs when all 8 correctness suites pass — this is what drives the
# oracle CPU-time up into the L3_long band.
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq \
 && apt-get install -y --no-install-recommends \
      build-essential gcc make libc6-dev \
      ca-certificates coreutils \
 && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY project/ /app/project/

# Pre-warm the build cache so the first agent invocation of `make` doesn't
# amortize an initial compile spike. We deliberately DO NOT pre-build the
# broken binary — the agent should observe a fresh test failure. `make clean`
# leaves no artifacts, but the compiler tools + headers are hot in the fs cache.
RUN cd /app/project \
 && make >/dev/null 2>&1 || true \
 && make clean >/dev/null 2>&1 \
 && ls -la /app/project

# Sanity: source tree must contain the 8 module .c files (guards against a
# botched COPY that drops files silently).
RUN test -f /app/project/Makefile \
 && test -f /app/project/src/main.c \
 && test -f /app/project/src/intmath.c \
 && test -f /app/project/src/bitops.c \
 && test -f /app/project/src/strutil.c \
 && test -f /app/project/src/hash.c \
 && test -f /app/project/src/sort.c \
 && test -f /app/project/src/heap.c \
 && test -f /app/project/src/matrix.c \
 && test -f /app/project/src/list.c
