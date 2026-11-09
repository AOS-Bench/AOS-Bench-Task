"""Harness around the original WildClawBench `grade()` function.

Loads the function below and prints its `overall_score` to stdout so test.sh
can capture it as the reward.
"""

import json
import sys


# === BEGIN ORIGINAL WildClawBench grade() ===
def grade(**kwargs) -> dict:
    """
    VLMEvalKit OCRBench evaluation task grading (additive).

    Scoring rules:
    - VLMEvalKit cloned (found clone directory under /workspace/tmp/): +0.2
    - result.json Final Score within 810-835 range: +0.8
    - Max score 1.0

    Returns:
        dict containing overall_score (0.0-1.0)
    """
    import json
    import logging
    import os
    from pathlib import Path

    log = logging.getLogger("grade_benchmark_vlmeval_ocrbench")
    logging.basicConfig(level=logging.INFO, format="[%(name)s] %(message)s")

    score = 0.0

    def _vlmevalkit_cloned() -> bool:
        """Recursively search /workspace/tmp/ for VLMEvalKit clone directory (case-insensitive)."""
        base = Path("/workspace/tmp")
        if not base.exists():
            return False
        for root, dirs, files in os.walk(base):
            root_lower = os.path.basename(root).lower()
            if "vlmevalkit" in root_lower or "vlmeval" in root_lower:
                if any(f.lower() == "run.py" for f in files):
                    log.info("Found VLMEvalKit clone: %s", root)
                    return True
                if any(d.lower() == "vlmeval" for d in dirs):
                    log.info("Found VLMEvalKit clone: %s", root)
                    return True
        return False

    if _vlmevalkit_cloned():
        score += 0.2
        log.info("VLMEvalKit clone detected, +0.2")
    else:
        log.info("VLMEvalKit clone directory not detected")

    result_file = Path("/workspace/tmp/results/result.json")
    if not result_file.exists():
        log.warning("result.json not found: %s", result_file)
        return {"overall_score": score}

    try:
        result = json.loads(result_file.read_text(encoding="utf-8"))
    except (json.JSONDecodeError, Exception) as e:
        log.error("result.json parse failed: %s", e)
        return {"overall_score": score}

    final_score = result.get("Final Score")
    if final_score is None:
        log.warning("result.json has no Final Score field")
        return {"overall_score": score}

    try:
        final_score = int(final_score)
    except (ValueError, TypeError):
        log.warning("Final Score is not a valid integer: %s", final_score)
        return {"overall_score": score}

    log.info("Final Score: %d", final_score)

    if 810 <= final_score <= 835:
        score += 0.8
        log.info("Final Score within 810-835 range, +0.8")
    else:
        log.warning("Final Score %d not within 810-835 range", final_score)

    return {"overall_score": score}
# === END ORIGINAL WildClawBench grade() ===


if __name__ == "__main__":
    try:
        scores = grade()
        if not isinstance(scores, dict):
            print("0.0")
            sys.exit(0)
        overall = scores.get("overall_score", 0.0)
        try:
            overall = float(overall)
        except (TypeError, ValueError):
            overall = 0.0
        print(f"{overall:.4f}")
        # Also dump full breakdown to stderr for debugging.
        print(json.dumps(scores, ensure_ascii=False, indent=2), file=sys.stderr)
    except Exception as e:
        print("0.0")
        print(f"grader error: {e}", file=sys.stderr)
