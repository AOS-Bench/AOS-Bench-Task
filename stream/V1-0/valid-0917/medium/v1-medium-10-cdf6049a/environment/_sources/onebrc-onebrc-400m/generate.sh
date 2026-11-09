#!/bin/bash
# onebrc-400m: 400,000,000 rows, seed 404, 10,000-station keyset.
# Deterministic: same seed + sorted station pool => bit-identical output every run.
set -euo pipefail
cd "$(dirname "$0")"
OUT="${OUT:-/workspace/measurements.txt}"
GEN_SEED=404 exec python3 gen.py 400000000 "$OUT" 10000 weather_stations.csv
