# AOS-Bench synth-v1 - compression-pareto
# Bakes a deterministic 2 GiB corpus at /app/corpus/ and installs the six
# compressors the agent gets to choose between:
#   zstd    (zstd-utils)          -1..-22, --long, --ultra, -T <threads>
#   xz      (xz-utils)            -0..-9 (-e for extreme), -T <threads>
#   brotli  (brotli)              -q 0..11
#   bzip2   (libbz2-1.0 + bzip2)  -1..-9
#   gzip    (default)             -1..-9
#   lzma    (from xz-utils)       -0..-9
#
# The task is to find a (algo, level, options) tuple that beats both a
# compressed-size cap AND a wall-clock cap; the pareto frontier is what
# the agent iterates on. See instruction.md for the numbers.
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update -qq \
 && apt-get install -y --no-install-recommends \
      ca-certificates coreutils diffutils \
      python3 python3-numpy \
      zstd xz-utils brotli bzip2 gzip \
      time \
 && rm -rf /var/lib/apt/lists/* \
 && command -v zstd && command -v xz && command -v brotli \
 && command -v bzip2 && command -v gzip && command -v lzma

# Compression targets are baked as env vars — the agent reads these from
# the container env (also disclosed in instruction.md); discovering the
# numbers is not the point. What's hidden is which (algo, level) tuple
# actually hits both.
# 618611712 bytes ≈ 590 MiB (~28.8% of 2 GiB). Reachable by xz -T4 -6
# (~560 MiB on this corpus) but not by any zstd/gzip/brotli/bzip2 at
# levels that also fit inside TARGET_COMPRESS_WALL.
ENV TARGET_COMPRESSED_BYTES=618611712
# TARGET_COMPRESS_WALL is the wall-clock cap the agent must record into
# /app/compression_wall_seconds.txt after running their chosen compressor.
# 360s comfortably covers xz -T4 -6 (measured 249-297s across runs, ~1x
# margin) but rejects zstd -19 --long -T4 (~350-410s) and xz -T4 -e -9
# (~500s+).
ENV TARGET_COMPRESS_WALL=360
ENV CORPUS_DIR=/app/corpus
ENV CORPUS_SIZE_BYTES=2147483648
ENV CORPUS_SEED=20260714

WORKDIR /app
COPY gen_corpus.py /root/gen_corpus.py

# Bake the 2 GiB corpus at build time. This is a one-shot deterministic
# generator; it takes ~90-180s on this host and produces reproducible
# bytes for the given seed.
RUN OUT_DIR="${CORPUS_DIR}" SIZE_BYTES="${CORPUS_SIZE_BYTES}" SEED="${CORPUS_SEED}" \
      python3 /root/gen_corpus.py \
 && du -sh "${CORPUS_DIR}" \
 && ls "${CORPUS_DIR}" | wc -l \
 && test "$(du -sb ${CORPUS_DIR} | awk '{print $1}')" = "${CORPUS_SIZE_BYTES}"

# Verifier deps: the test.sh does a byte-identical diff of the agent's
# decompressed output against /app/corpus/, so no extra libs needed.
