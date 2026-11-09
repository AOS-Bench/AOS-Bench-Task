#!/bin/bash
# Generated from the parent stream's graders -- do not edit by hand.
#
# Reproduces harbor StreamVerifier's per-sub-task contract for a single
# sub-task, so the sub-task's own grader runs in exactly the shape it was
# rewritten for:
#   AOS_TASK_ID    = <task_id>
#   AOS_REWARD_DIR = /logs/verifier/<task_id>   (grader writes reward.txt here)
#   AOS_TESTS_DIR  = /tests/<task_id>
# then aggregates to /logs/verifier/reward.json as {task_id: score}.
#
# Never `set -e`: a failing grader must still leave a 0.0 reward behind, the
# same way StreamVerifier's `|| true` guarantees.
TASK_ID='tb2-regex-chess-3'
TESTS_DIR="/tests/$TASK_ID"
REWARD_DIR="/logs/verifier/$TASK_ID"
LEGACY_REWARD="/logs/verifier/reward.txt"

mkdir -p "$REWARD_DIR"
# Clear any stale top-level reward.txt so the fallback-move below is
# unambiguous about who wrote it.
rm -f "$LEGACY_REWARD"

chmod +x "$TESTS_DIR/test.sh" 2>/dev/null || true

AOS_TASK_ID="$TASK_ID" \
AOS_REWARD_DIR="$REWARD_DIR" \
AOS_TESTS_DIR="$TESTS_DIR" \
  bash "$TESTS_DIR/test.sh" > "$REWARD_DIR/stdout.txt" 2>&1
echo "grader exit: $?"

# Legacy shim, same as StreamVerifier's: older graders ignore AOS_REWARD_DIR
# and write straight to /logs/verifier/reward.txt.
if [ ! -f "$REWARD_DIR/reward.txt" ] && [ -f "$LEGACY_REWARD" ]; then
  mv "$LEGACY_REWARD" "$REWARD_DIR/reward.txt"
fi

# MUST NOT survive: harbor's Verifier checks verifier/reward.txt BEFORE
# reward.json, and a surviving reward.txt would yield {"reward": x} instead of
# {"<task_id>": x} -- a silent schema change that still looks like a good run.
rm -f "$LEGACY_REWARD"

# Passed through the environment, not interpolated into the heredoc body: the
# body is single-quoted so a task_id containing a quote can never break out.
export AOS_INDIVIDUAL_TASK_ID="$TASK_ID"
python3 - <<'PYEOF'
import json, os, pathlib
task_id = os.environ["AOS_INDIVIDUAL_TASK_ID"]
root = pathlib.Path("/logs/verifier")
raw = (root / task_id / "reward.txt")
try:
    score = float(raw.read_text().strip())
except (OSError, ValueError):
    score = 0.0
(root / "reward.json").write_text(json.dumps({task_id: score}))
PYEOF

cat "/logs/verifier/reward.json" 2>/dev/null
exit 0
