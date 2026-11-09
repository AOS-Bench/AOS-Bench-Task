#!/bin/bash
# Hardened verifier for ParEval OpenMP task 46_sparse_la_spmm.
# Zero network required: g++ and python3 are preinstalled in the task image.
#
# Anti-reward-hacking protocol (see tests/assets/omp-driver.cc):
#   1. The benchmark driver is built twice: a trusted baseline build (serial
#      reference only, /workspace never on the include path) and an agent build
#      (generated-code.hpp only, baseline.hpp never included).
#   2. Correctness: every timed iteration's output is dumped by trusted code
#      at the full problem size; compare.cc must match the two record streams.
#      Size-gating, iteration-skipping, caching, or macro tampering in the
#      agent header changes the agent stream (or breaks the link) -> reward 0.
#   3. Timing: nothing the agent build prints is parsed. The serial compute
#      time comes from the baseline build's stdout (no agent code in that
#      binary); the agent build's compute time is its externally measured wall
#      time minus the trusted overhead measured on the baseline run.
#   4. The agent build runs BEFORE the baseline, with mktemp'd paths, so no
#      reference dump exists anywhere for in-process agent code to copy.
set -u
# === aos-bench: stream verifier shim (added by task_construction) ===
# Make this test.sh runnable both under harbor's /tests mount and under
# AOS-Bench's per-task /tasks/<id>/tests/ mount.
__AOS_TESTS_DIR="$(cd "$(dirname "${BASH_SOURCE[0]:-$0}")" && pwd)"
# Prefer the env supplied by harbor's StreamVerifier (sets AOS_TESTS_DIR per
# arrival) when present — it points at the per-task upload dir even when
# this test.sh is sourced through a wrapper that loses __AOS_TESTS_DIR.
if [ -n "${AOS_TESTS_DIR:-}" ]; then
  __AOS_TESTS_DIR="$AOS_TESTS_DIR"
fi
# Tests dir basename in stream mode is the task_id, which often contains
# dashes (e.g. `llm-inference-batching-scheduler`). Python can't load such
# a path as a package, so relative imports (`from .cost_model import …`)
# in test_outputs.py fail. Create an underscore-only symlink alias and
# point pytest at it so the package is loadable.
__AOS_TESTS_BASE="$(basename "$__AOS_TESTS_DIR")"
__AOS_TESTS_SAFE_NAME="$(echo "$__AOS_TESTS_BASE" | tr '-' '_')"
if [ "$__AOS_TESTS_SAFE_NAME" != "$__AOS_TESTS_BASE" ]; then
  __AOS_TESTS_PARENT="$(dirname "$__AOS_TESTS_DIR")"
  __AOS_TESTS_SAFE="$__AOS_TESTS_PARENT/$__AOS_TESTS_SAFE_NAME"
  if [ ! -e "$__AOS_TESTS_SAFE" ]; then
    ln -sfn "$__AOS_TESTS_DIR" "$__AOS_TESTS_SAFE" 2>/dev/null || true
  fi
  # Point downstream commands at the safe alias so relative imports work.
  [ -e "$__AOS_TESTS_SAFE" ] && __AOS_TESTS_DIR="$__AOS_TESTS_SAFE"
fi
__AOS_REWARD_DIR="${AOS_REWARD_DIR:-/logs/verifier}"
mkdir -p "$__AOS_REWARD_DIR"
# (Old `/tests` symlink was removed: races between concurrent test.sh in
# StreamVerifier parallel mode. We rewrite hardcoded `/tests/<file>` refs
# below to `$__AOS_TESTS_DIR/<file>` instead.)
# Stub out runtime `curl https://astral.sh/uv/...install.sh | sh` so test.sh
# scripts that bootstrap uv don't hang on flaky outbound network. uv is
# pre-installed in /usr/local/bin via the merged Dockerfile (pip install uv).
# Provide $HOME/.local/bin/{uv,uvx,env} so `source $HOME/.local/bin/env` and
# subsequent `uvx ...` calls Just Work.
if command -v uv >/dev/null 2>&1 && [ ! -e "$HOME/.local/bin/uv" ]; then
  mkdir -p "$HOME/.local/bin"
  ln -sf "$(command -v uv)" "$HOME/.local/bin/uv"
  ln -sf "$(command -v uvx)" "$HOME/.local/bin/uvx" 2>/dev/null || true
  : > "$HOME/.local/bin/env"  # empty env stub so `source` is a no-op
fi
# Replace the astral.sh install pipeline with a no-op when uv exists.
__aos_install_uv_noop() { command -v uv >/dev/null 2>&1; }
# === end shim ===

LOGS="${PAREVAL_LOGS_DIR:-/logs}/verifier"
WORKDIR="${PAREVAL_WORKDIR:-/workspace}"
mkdir -p "$LOGS"
echo 0 > "$LOGS/reward.txt"

ASSETS="$(cd "$(dirname "$0")" && pwd)/assets"
GEN="$WORKDIR/generated-code.hpp"
SIZE="(1<<11)"
ATOL="1e-6"
RTOL="1e-6"
NUM_THREADS=4
RUN_TIMEOUT=360
BIN=$(mktemp -d /tmp/pareval.XXXXXXXX)
trap 'rm -rf "$BIN"' EXIT

if [ ! -f "$GEN" ]; then
    echo "FAIL: $GEN not found" | tee "$LOGS/verifier.log"
    exit 0
fi

# Trusted comparator + trusted baseline build (verifier-side; agent code never
# touches these). A failure here is an infrastructure error -> reward stays 0.
if ! g++ -std=c++17 -O2 "$ASSETS/compare.cc" -o "$BIN/compare" \
        > "$LOGS/compile.log" 2>&1; then
    echo "FAIL: comparator compilation error (verifier bug)" | tee "$LOGS/verifier.log"
    exit 0
fi
if ! g++ -std=c++17 -O3 -fopenmp -DUSE_OMP -DDRIVER_PROBLEM_SIZE="$SIZE" \
        -DPAREVAL_BASELINE_ONLY -I"$ASSETS" \
        "$ASSETS/omp-driver.cc" "$ASSETS/cpu.cc" -o "$BIN/base" \
        >> "$LOGS/compile.log" 2>&1; then
    echo "FAIL: baseline compilation error (verifier bug)" | tee "$LOGS/verifier.log"
    exit 0
fi

# Agent build. -I order matters: canonical assets first so an agent-provided
# utilities.hpp/baseline.hpp copy in /workspace cannot shadow the graded ones.
if ! g++ -std=c++17 -O3 -fopenmp -DUSE_OMP -DDRIVER_PROBLEM_SIZE="$SIZE" \
        -I"$ASSETS" -I"$WORKDIR" \
        "$ASSETS/omp-driver.cc" "$ASSETS/cpu.cc" -o "$BIN/test" \
        >> "$LOGS/compile.log" 2>&1; then
    echo "FAIL: compilation error" | tee "$LOGS/verifier.log"
    tail -n 50 "$LOGS/compile.log"
    exit 0
fi

# Run the agent build FIRST (no reference dump exists yet anywhere).
t0=$(date +%s.%N)
timeout "$RUN_TIMEOUT" "$BIN/test" "$NUM_THREADS" "$BIN/test.dump" \
    > "$LOGS/agent_run.log" 2>&1   # stdout is untrusted; kept for debugging only
agent_status=$?
t1=$(date +%s.%N)
if [ "$agent_status" -ne 0 ]; then
    echo "FAIL: agent benchmark exited with status $agent_status" | tee "$LOGS/verifier.log"
    exit 0
fi

# Trusted baseline run (serial, 1 thread).
t2=$(date +%s.%N)
timeout "$RUN_TIMEOUT" "$BIN/base" 1 "$BIN/base.dump" \
    > "$LOGS/baseline_run.log" 2>&1
base_status=$?
t3=$(date +%s.%N)
if [ "$base_status" -ne 0 ]; then
    echo "FAIL: baseline benchmark exited with status $base_status (verifier bug)" | tee "$LOGS/verifier.log"
    exit 0
fi

# Correctness: trusted record streams must match.
"$BIN/compare" "$BIN/base.dump" "$BIN/test.dump" "$ATOL" "$RTOL" \
    > "$LOGS/compare.log" 2>&1
cmp_status=$?
cat "$LOGS/compare.log"
if [ "$cmp_status" -ne 0 ]; then
    echo "FAIL: output mismatch vs serial reference (compare exit $cmp_status)" | tee "$LOGS/verifier.log"
    exit 0
fi

# Performance gate, from trusted measurements only.
AGENT_WALL_T0="$t0" AGENT_WALL_T1="$t1" BASE_WALL_T2="$t2" BASE_WALL_T3="$t3" \
    LOGS="$LOGS" python3 - <<'PY'
import os, re

MIN_SPEEDUP = 1.5
logs = os.environ["LOGS"]
text = open(os.path.join(logs, "baseline_run.log")).read()
m = re.search(r"SerialComputeTime:\s*([0-9.eE+-]+)", text)

reward = 0
if m is None:
    print("FAIL: could not parse baseline timing (verifier bug)")
else:
    serial = float(m.group(1))
    agent_wall = float(os.environ["AGENT_WALL_T1"]) - float(os.environ["AGENT_WALL_T0"])
    base_wall = float(os.environ["BASE_WALL_T3"]) - float(os.environ["BASE_WALL_T2"])
    # Trusted overhead (init/advance/dump) is identical in both builds.
    overhead = max(base_wall - serial, 0.0)
    agent_compute = agent_wall - overhead
    if serial <= 0:
        print("FAIL: non-positive serial time")
    elif agent_compute <= 0:
        print("FAIL: non-positive agent compute time estimate")
    else:
        speedup = serial / agent_compute
        print(f"serial={serial:.6f}s agent={agent_compute:.6f}s "
              f"(agent_wall={agent_wall:.6f}s overhead={overhead:.6f}s) "
              f"speedup={speedup:.2f}x (need >= {MIN_SPEEDUP}x)")
        if speedup >= MIN_SPEEDUP:
            print("PASS")
            reward = 1
        else:
            print("FAIL: insufficient parallel speedup")

with open(os.path.join(logs, "reward.txt"), "w") as f:
    f.write(str(reward) + "\n")
PY
exit 0
