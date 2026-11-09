# WildClawBench task converted to harbor format by aos_bench.task_construction.
# Source: data/harbor/wildclawbench/tasks/02_Code_Intelligence/02_Code_Intelligence_task_6_benchmark_vlmeval_ocrbench_zh.md
#
# Temporal isolation (per docs/framework/verification.md):
#   - This image contains ONLY agent-visible inputs: exec fixtures and
#     optional skills. Ground truth (gt/) and tests are NOT baked in.
#   - At verify-/oracle-time, harbor uploads:
#       <task>/tests/   →  /tests/<task_id>/      (includes /tests/<id>/.gt/)
#       <stream>/solution/ → /solution/           (oracle only)
#     and test.sh installs the symlink `/workspace/.gt → $AOS_TESTS_DIR/.gt`
#     so the grader's existing `Path("/workspace/.gt")` / `workspace/"gt"`
#     references resolve correctly.
#
# Path layout when test.sh / solve.sh runs:
#   /workspace/             ← task root (was /app)
#   /workspace/.gt          ← symlink to $AOS_TESTS_DIR/.gt (installed by shim)
#   /workspace/gt           ← symlink to /workspace/.gt (so /workspace/tmp/gt resolves)
#   /workspace/tmp          ← symlink to /workspace (was /tmp_workspace)
FROM ubuntu:24.04

ENV DEBIAN_FRONTEND=noninteractive
ENV PIP_BREAK_SYSTEM_PACKAGES=1

RUN apt-get update && apt-get install -y --no-install-recommends \
    python3 \
    python3-pip \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

# Python packages needed by the WildClawBench grader at verify-time.
# `openai` is imported by ~34 graders (LLM-as-judge); `Pillow` by image-based
# pre-checks even in tasks that fall back to text-only judging.
# Reward-time env (set by StreamVerifier / OracleAgent): OPENROUTER_API_KEY /
# OPENROUTER_BASE_URL / JUDGE_MODEL (default openai/gpt-5.4).
# --ignore-installed: ubuntu:24.04 ships python3-typing-extensions via dpkg
# with no pip RECORD file, so installing `openai` (which needs a newer
# typing_extensions) aborts with "Cannot uninstall typing_extensions 4.10.0,
# RECORD file not found. Hint: The package was installed by debian."
# Observed at STEP 75 of medium-10-scan-303. This pattern is shared by the
# whole wildclawbench family, so it blocks any stream containing one.
RUN pip install --no-cache-dir --ignore-installed \
        -i https://pypi.tuna.tsinghua.edu.cn/simple \
        openai Pillow

WORKDIR /workspace

# Workspace fixtures (initial input only — no ground truth).
COPY _workspace_exec/ /workspace/
RUN ln -sfn /workspace /workspace/tmp

# Skills referenced by this task — markdown skill descriptions.
# (no skills referenced)
