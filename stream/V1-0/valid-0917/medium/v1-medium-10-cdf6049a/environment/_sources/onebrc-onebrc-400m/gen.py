#!/usr/bin/env python3
# Deterministic 1BRC-style measurement generator.
# Determinism guarantees (verified bit-identical across runs/machines):
#   - station name pool is sorted() before sampling (independent of set/hash order)
#   - single fixed random.seed taken from GEN_SEED (default 42)
#   - values formatted with fixed "%.1f" -> exactly one decimal digit
import os
import sys
import random

SEED = int(os.environ.get("GEN_SEED", "42"))
N = int(sys.argv[1].replace("_", ""))
OUT = sys.argv[2]
CARD = int(sys.argv[3]) if len(sys.argv) > 3 else 10000


def stations():
    s = set()
    path = sys.argv[4] if len(sys.argv) > 4 else "weather_stations.csv"
    for line in open(path, encoding="utf-8"):
        if line.startswith("#"):
            continue
        parts = line.split(";")
        if len(parts) >= 1 and parts[0]:
            s.add(parts[0])
    return sorted(s)  # sort => deterministic regardless of hash seed


def main():
    random.seed(SEED)
    names = stations()
    pool = random.choices(names, k=CARD)
    batch = 10000
    chunks = N // batch
    with open(OUT, "w", encoding="utf-8") as f:
        for _ in range(chunks):
            b = random.choices(pool, k=batch)
            f.write("\n".join(f"{s};{random.uniform(-99.9, 99.9):.1f}" for s in b))
            f.write("\n")


main()
