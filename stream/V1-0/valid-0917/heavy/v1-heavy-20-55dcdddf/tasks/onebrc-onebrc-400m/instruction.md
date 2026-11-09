# One Billion Row Challenge — 400M rows

Your working directory `/workspace` contains:

- `weather_stations.csv` — the station-name catalog (`name;latitude` rows, `#` comment lines)
- `gen.py` — a deterministic measurement generator
- `generate.sh` — wraps `gen.py` with this task's fixed row count and seed

## Step 1 — generate the data

```bash
cd /workspace && ./generate.sh
```

This produces `/workspace/measurements.txt`: **400,000,000** rows of the form
`<station name>;<temperature>` where the temperature always has exactly one
decimal digit (e.g. `Tokyo;35.6`, `Aachen;-9.2`). Generation is deterministic —
do not modify `gen.py`, `generate.sh`, or `weather_stations.csv`, and do not
delete `measurements.txt` afterwards.

## Step 2 — aggregate

Write a program (any language available in the container — Python 3.12 and gcc
are preinstalled; 4 CPUs are available) that reads `measurements.txt` and, for
every station that appears, computes the minimum, mean, and maximum temperature.

**Arithmetic requirements (mandatory):**

- Parse each temperature as an **integer number of tenths** (fixed-point). Do
  the min/max/sum bookkeeping in integers — do **not** accumulate floats (float
  accumulation drifts at this scale and will be treated as incorrect
  methodology).
- `min` and `max` are exact input values (one decimal digit).
- `mean` = (sum of tenths) / count, rounded to one decimal digit
  (half-away-from-zero recommended; the grader tolerates ±0.1 on the mean only).
- Normalize negative zero: never output `-0.0`, output `0.0`.

## Step 3 — output

Write the result to `/workspace/result.txt` as a **single line** in exactly this
format, sorted by station name in ascending Unicode code-point order (byte order
of the UTF-8 names), entries separated by `", "`, terminated by a newline:

```
{A Yun Pa=-99.6/0.7/99.6, Aachen=-99.6/-2.2/99.7, Aaley=-99.9/0.1/99.9, ...}
```

Each entry is `<name>=<min>/<mean>/<max>` with every value printed with exactly
one decimal digit.

## Rules

- No network access: solve everything with what is already in the container.
- You must compute the results from `measurements.txt` yourself; the aggregation
  is the task.
- Grading is pass/fail: station set and ordering must match exactly, min/max
  must be exact, mean must be within ±0.1.
