# AOS-Bench synth-v1 · primesieve-pi-1e13
# Base image with primesieve pre-installed to avoid apt-get during the
# CPU-time audit (keeps solve.sh CPU-bound rather than IO-bound on network).
FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive
RUN apt-get update -qq \
 && apt-get install -y --no-install-recommends primesieve ca-certificates \
 && rm -rf /var/lib/apt/lists/*
WORKDIR /app
