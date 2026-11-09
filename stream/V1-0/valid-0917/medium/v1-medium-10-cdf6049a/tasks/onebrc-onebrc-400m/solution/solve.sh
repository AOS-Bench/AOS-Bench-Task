#!/bin/bash
# Oracle: generate the measurements, then aggregate with a 4-worker mmap
# integer-tenths (fixed-point) aggregator. Runs entirely offline.
set -ex

cd /workspace
OUT=/workspace/measurements.txt ./generate.sh   # honor the public /workspace output contract

python3 - <<'EOF'
import math
import mmap
import os
from multiprocessing import Pool

FN = "/workspace/measurements.txt"
W = 4


def worker(rg):
    a, b = rg
    mn = {}
    mx = {}
    sm = {}
    ct = {}
    with open(FN, "rb") as f:
        mm = mmap.mmap(f.fileno(), 0, prot=mmap.PROT_READ)
        if a != 0:
            # Own the lines that START in [a, b). Seek to a-1 and skip one
            # readline: if byte a-1 is '\n', a is a line start and only the
            # newline is consumed (no line lost); otherwise the partial line
            # (owned by the previous worker) is skipped.
            mm.seek(a - 1)
            mm.readline()
        else:
            mm.seek(0)
        while mm.tell() < b:
            line = mm.readline()
            if not line:
                break
            i = line.rfind(b";")
            name = line[:i]
            t = line[i + 1 :].rstrip(b"\n")
            neg = t[0:1] == b"-"
            if neg:
                t = t[1:]
            # 'd.d' or 'dd.d' -> integer tenths (exact, no float parsing)
            v = (t[0] - 48) * 10 + (t[2] - 48) if len(t) == 3 else int(t[:-2]) * 10 + (t[-1] - 48)
            if neg:
                v = -v
            if name in ct:
                if v < mn[name]:
                    mn[name] = v
                if v > mx[name]:
                    mx[name] = v
                sm[name] += v
                ct[name] += 1
            else:
                mn[name] = mx[name] = v
                sm[name] = v
                ct[name] = 1
    return mn, mx, sm, ct


def main():
    sz = os.path.getsize(FN)
    step = sz // W
    ranges = [(i * step, (i + 1) * step if i < W - 1 else sz) for i in range(W)]
    with Pool(W) as p:
        parts = p.map(worker, ranges)
    MN = {}
    MX = {}
    SM = {}
    CT = {}
    for mn, mx, sm, ct in parts:
        for k in ct:
            if k in CT:
                if mn[k] < MN[k]:
                    MN[k] = mn[k]
                if mx[k] > MX[k]:
                    MX[k] = mx[k]
                SM[k] += sm[k]
                CT[k] += ct[k]
            else:
                MN[k] = mn[k]
                MX[k] = mx[k]
                SM[k] = sm[k]
                CT[k] = ct[k]
    out = []
    for k in sorted(MN):  # bytes sort == UTF-8 byte order == code-point order
        s = SM[k]
        c = CT[k]
        # mean in tenths, rounded half-away-from-zero to the nearest tenth
        mean_t = math.floor(s / c + 0.5) if s >= 0 else -math.floor(-s / c + 0.5)
        name = k.decode("utf-8")
        # all values are int tenths: ints have no -0, so -0.0 can never appear
        out.append(f"{name}={MN[k] / 10.0:.1f}/{mean_t / 10.0:.1f}/{MX[k] / 10.0:.1f}")
    with open("/workspace/result.txt", "w", encoding="utf-8") as f:
        f.write("{" + ", ".join(out) + "}\n")
    print(len(MN), "stations written to /workspace/result.txt")


main()
EOF
