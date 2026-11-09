#!/usr/bin/env python3
"""Deterministic 2 GiB corpus generator for compression-pareto.

Emits a directory of files at OUT_DIR (default /workspace/corpus/) containing a
mix of compressibility profiles:

  50% JSON logs (structured, highly repetitive, mid-cardinality strings)
  35% text      (Zipf-distributed word stream, English-like)
  10% binary    (fixed-schema records, mostly-low-entropy payload)
   5% random    (incompressible tail — sets the hard lower bound)

The total on-disk size is exactly SIZE_BYTES (default 2 GiB), truncating the
final file if necessary. Seed is fixed so the entire corpus is reproducible.

Compression frontier at 2 GiB on 4-CPU / 8 GiB (measured on this host):
  zstd -T4 -3         : ~654 MiB / 3s    (size fail, wall pass)
  zstd -T4 -9         : ~609 MiB / 12s   (size fail, wall pass)
  zstd -T4 -15 --long : ~583 MiB / 123s  (size fail, wall pass)
  zstd -T4 -19 --long : ~547 MiB / 353s  (size pass, WALL fail)
  xz -T4 -6           : ~535 MiB / 249s  (size pass, wall pass) *WINNER*
  brotli -q 6         : ~622 MiB / 80s   (size fail, wall pass)
  gzip -6             : ~683 MiB / 58s   (size fail, wall pass)

With TARGET_COMPRESSED_BYTES=590 MiB and TARGET_COMPRESS_WALL=360s the
frontier has exactly one point that passes both gates (xz -T4 -6).
Agent must iterate through 5-7 trials to find it.
"""
from __future__ import annotations

import hashlib
import json
import os
import random
import string
import struct
import sys
from pathlib import Path


GIB = 1024 * 1024 * 1024


def zipf_vocabulary(rng: random.Random, size: int = 4096) -> list[str]:
    """Build a deterministic ~English-shaped vocabulary."""
    letters = string.ascii_lowercase
    vocab: list[str] = []
    seen: set[str] = set()
    while len(vocab) < size:
        # word length ~ Zipf-ish 3..12
        n = 3 + int(rng.random() ** 3 * 9)
        w = "".join(rng.choice(letters) for _ in range(n))
        if w not in seen:
            seen.add(w)
            vocab.append(w)
    # Zipf frequency: word i (1-indexed) has weight 1/i
    return vocab


def zipf_choice(rng: random.Random, vocab: list[str], zipf_cdf: list[float]) -> str:
    r = rng.random()
    # bisect on cdf
    lo, hi = 0, len(zipf_cdf) - 1
    while lo < hi:
        mid = (lo + hi) // 2
        if zipf_cdf[mid] < r:
            lo = mid + 1
        else:
            hi = mid
    return vocab[lo]


def build_zipf_cdf(n: int) -> list[float]:
    # weights 1/1 + 1/2 + ... + 1/n
    total = 0.0
    weights = []
    for i in range(1, n + 1):
        w = 1.0 / i
        weights.append(w)
        total += w
    cdf = []
    running = 0.0
    for w in weights:
        running += w / total
        cdf.append(running)
    return cdf


def write_json_logs(fh, bytes_target: int, rng: random.Random) -> int:
    """Write ~bytes_target of structured JSON log lines."""
    users = [f"user_{i:05d}" for i in range(200)]
    endpoints = [
        "/api/v1/users", "/api/v1/orders", "/api/v1/products",
        "/api/v1/checkout", "/api/v1/search", "/api/v1/reviews",
        "/api/v1/cart/add", "/api/v1/cart/remove", "/api/v1/session",
        "/api/v2/users", "/api/v2/orders", "/health", "/metrics",
    ]
    methods = ["GET", "POST", "PUT", "DELETE", "PATCH"]
    statuses = [200, 200, 200, 200, 201, 204, 301, 400, 404, 500, 503]
    regions = ["us-east-1", "us-west-2", "eu-west-1", "ap-southeast-1"]
    services = ["web", "api", "worker", "scheduler", "auth"]
    # Pre-generate a pool of req_ids and reuse — lots of duplication is
    # realistic (retries, correlated requests) and gives compressors
    # something to chew.
    req_id_pool = [f"{rng.getrandbits(48):012x}" for _ in range(4096)]
    written = 0
    ts = 1_700_000_000
    while written < bytes_target:
        ts += rng.randint(1, 4)
        rec = {
            "ts": ts,
            "svc": rng.choice(services),
            "region": rng.choice(regions),
            "user": rng.choice(users),
            "method": rng.choice(methods),
            "endpoint": rng.choice(endpoints),
            "status": rng.choice(statuses),
            "latency_ms": round(rng.expovariate(1 / 40.0), 2),
            "bytes_out": rng.randint(100, 20_000),
            "req_id": rng.choice(req_id_pool),
        }
        line = json.dumps(rec, separators=(",", ":")) + "\n"
        b = line.encode("ascii")
        fh.write(b)
        written += len(b)
    return written


def write_text(fh, bytes_target: int, rng: random.Random,
               vocab: list[str], zipf_cdf: list[float]) -> int:
    """Write ~bytes_target of Zipf-distributed English-like text."""
    written = 0
    # write ~80 char lines
    while written < bytes_target:
        line_words: list[str] = []
        line_len = 0
        while line_len < 80:
            w = zipf_choice(rng, vocab, zipf_cdf)
            line_words.append(w)
            line_len += len(w) + 1
        line = " ".join(line_words) + "\n"
        b = line.encode("ascii")
        fh.write(b)
        written += len(b)
    return written


def write_binary(fh, bytes_target: int, rng: random.Random) -> int:
    """Write ~bytes_target of fixed-schema binary log records.

    Record: <u32 magic> <u64 timestamp> <u16 code> <u16 flags>
            <f64 value> <32-byte payload>   -> 56 bytes each
    Magic and code have low cardinality (compressible), payload is random.
    But we make payload NOT fully random: half of its bytes are drawn from
    a low-cardinality alphabet so LZ codecs can chew it, and half are
    uniformly random. Zstd/xz should chew the mag/code well; the pseudo-
    random half-payload is where levels start to matter.
    """
    magic = 0xCAFEBABE
    codes = [0x1000, 0x1001, 0x1002, 0x2000, 0x2001, 0x3000, 0xFF00, 0xFF01]
    alphabet = bytes(range(0x20, 0x40))  # 32-char alphabet for low-entropy half
    written = 0
    ts = 1_700_000_000
    chunk_records = 65536
    fmt = "<IQHHd"
    header_size = struct.calcsize(fmt)  # 4+8+2+2+8 = 24; total 24+32 = 56
    while written < bytes_target:
        buf = bytearray()
        for _ in range(chunk_records):
            ts += 1
            code = rng.choice(codes)
            flags = rng.randint(0, 0xFFFF)
            value = rng.gauss(0.0, 1.0)
            header = struct.pack(fmt, magic, ts, code, flags, value)
            low_ent = bytes(rng.choice(alphabet) for _ in range(28))
            hi_ent = bytes(rng.getrandbits(8) for _ in range(4))
            buf += header
            buf += low_ent
            buf += hi_ent
            if written + len(buf) >= bytes_target:
                break
        b = bytes(buf)
        fh.write(b)
        written += len(b)
    return written


def write_random(fh, bytes_target: int, rng: random.Random) -> int:
    """Write ~bytes_target of high-entropy random bytes."""
    written = 0
    chunk = 4 * 1024 * 1024
    # Seed a python-hashlib CSPRNG-lite stream deterministically
    seed_bytes = rng.getrandbits(256).to_bytes(32, "big")
    counter = 0
    while written < bytes_target:
        remaining = bytes_target - written
        n = min(chunk, remaining)
        # Deterministic-random stream via SHAKE-like sha256 chain
        out = bytearray()
        while len(out) < n:
            counter += 1
            h = hashlib.sha256(seed_bytes + counter.to_bytes(8, "big")).digest()
            out += h
        fh.write(bytes(out[:n]))
        written += n
    return written


def truncate_or_pad(path: Path, target: int) -> None:
    """Ensure path is exactly target bytes."""
    cur = path.stat().st_size
    if cur == target:
        return
    if cur > target:
        with open(path, "r+b") as fh:
            fh.truncate(target)
        return
    # pad with zeros — should not really happen since we overshoot then trim,
    # but keep it defensive.
    with open(path, "ab") as fh:
        fh.write(b"\x00" * (target - cur))


def main() -> None:
    out_dir = Path(os.environ.get("OUT_DIR", "/workspace/corpus"))
    size_bytes = int(os.environ.get("SIZE_BYTES", str(2 * GIB)))
    seed = int(os.environ.get("SEED", "20260714"))
    out_dir.mkdir(parents=True, exist_ok=True)

    rng = random.Random(seed)
    vocab = zipf_vocabulary(rng, size=4096)
    zipf_cdf = build_zipf_cdf(len(vocab))

    # Budget (bytes) per file. Slight overshoot per section is fine — we
    # truncate to hit the exact per-file target so total is deterministic.
    json_target   = int(size_bytes * 0.50)   # ~1024 MiB
    text_target   = int(size_bytes * 0.35)   #  ~716 MiB
    binary_target = int(size_bytes * 0.10)   #  ~205 MiB
    # random takes whatever remains so the total sums to size_bytes exactly
    random_target = size_bytes - json_target - text_target - binary_target

    # Split json + text into multiple shards to make it feel like a real
    # log/text bundle (also gives the agent something to iterate on).
    def write_shards(name: str, total: int, shards: int, writer, *args) -> None:
        per = total // shards
        rem = total - per * shards
        for i in range(shards):
            tgt = per + (rem if i == shards - 1 else 0)
            p = out_dir / f"{name}_{i:02d}.dat"
            print(f"[gen] {p.name} target={tgt}", flush=True)
            with open(p, "wb") as fh:
                writer(fh, tgt, *args)
            truncate_or_pad(p, tgt)
            print(f"[gen]  wrote {p.name} size={p.stat().st_size}", flush=True)

    print(f"[gen] out_dir={out_dir} size_bytes={size_bytes} seed={seed}", flush=True)
    write_shards("logs", json_target, 4, write_json_logs, rng)
    write_shards("text", text_target, 4, write_text, rng, vocab, zipf_cdf)
    write_shards("bin",  binary_target, 4, write_binary, rng)
    write_shards("rand", random_target, 2, write_random, rng)

    # Sanity: sum of file sizes must equal size_bytes exactly
    total = sum(p.stat().st_size for p in sorted(out_dir.iterdir()))
    print(f"[gen] total corpus size = {total} (target {size_bytes})", flush=True)
    if total != size_bytes:
        print(f"[gen] FATAL: total {total} != target {size_bytes}", file=sys.stderr)
        sys.exit(1)

    # Emit a manifest with per-file sha256 (used by decompress correctness
    # check; agent can also compare against it if they want, though the
    # verifier does a direct diff -r so hashes are informational only).
    manifest = {}
    for p in sorted(out_dir.iterdir()):
        with open(p, "rb") as fh:
            h = hashlib.sha256()
            while True:
                b = fh.read(1 << 20)
                if not b:
                    break
                h.update(b)
            manifest[p.name] = {"size": p.stat().st_size, "sha256": h.hexdigest()}
    manifest_path = out_dir.parent / "corpus_manifest.json"
    with open(manifest_path, "w") as fh:
        json.dump({"seed": seed, "size_bytes": size_bytes, "files": manifest}, fh, indent=2, sort_keys=True)
    print(f"[gen] wrote manifest {manifest_path}", flush=True)


if __name__ == "__main__":
    main()
