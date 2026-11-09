# Authoritative grading reference: integer tenths (fixed-point), order-independent,
# half-away-from-zero mean rounding. Runs at AUTHORING time only, to produce
# tests/expected_output.txt; it is never shipped in the task image.
import math
import sys

mn = {}
mx = {}
sm = {}
ct = {}
with open(sys.argv[1], "rb") as f:
    for line in f:
        i = line.rfind(b";")
        name = line[:i]
        t = line[i + 1 :].rstrip(b"\n")
        neg = t[0:1] == b"-"
        if neg:
            t = t[1:]
        # form d.d -> integer tenths
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


def r(vt):
    return vt / 10.0


out = []
for k in sorted(mn):
    s = sm[k]
    c = ct[k]
    # mean in tenths = s/c ; round half-away-from-zero to nearest integer tenth
    mean_t = math.floor(s * 1.0 / c + 0.5) if s >= 0 else -math.floor(-s * 1.0 / c + 0.5)
    name = k.decode("utf-8")
    out.append(f"{name}={r(mn[k]):.1f}/{mean_t / 10.0:.1f}/{r(mx[k]):.1f}")
sys.stdout.write("{" + ", ".join(out) + "}\n")
