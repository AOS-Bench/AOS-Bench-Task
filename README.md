# AOS-Bench-Task

Workloads and tasks for [AOS-Bench](https://github.com/AOS-Bench/AOS-Bench), a benchmark that
evaluates LLM agent systems as workload managers. This repository holds exactly the workloads
used in the paper, in the Harbor task format, together with the same tasks laid out one per
directory for the individual (single-task) baseline.

[中文说明](README_zh.md)

## Contents

| Path | Contents |
|---|---|
| `stream/V1-0/valid-0917/medium/` | 10 workloads, 10 tasks each |
| `stream/V1-0/valid-0917/heavy/` | 5 workloads, 20 tasks each |
| `stream/V1-0/valid-0917/heavy-plus/` | 1 workload of 30 tasks (case study) |
| `stream/V1-0/valid-0917-individual-dedup/<tier>/` | the same tasks, one directory per unique task, for individual runs |

A workload directory (`<tier>/<workload-id>/`) contains:

| Entry | Purpose |
|---|---|
| `stream.toml` | resources (4 CPU, 8 GiB), the prebuilt image, and the release schedule (`[[arrivals]]`) |
| `environment/Dockerfile` | the merged environment of all tasks in the workload; `_sources/` holds the per-task build inputs |
| `tasks/<task-id>/` | `instruction.md`, `tests/` (the grader) and `solution/` (the reference solution) |
| `solution/solve.sh` | oracle entry point that runs every task's reference solution in arrival order |

An individual task directory (`<parent-workload-hash>-<task-id>/`) contains `task.toml`,
`instruction.md`, `environment/` and `tests/`. Its `task.toml` names the parent workload and
uses the parent workload's image, cores, memory and network policy, so the only difference from
the stream setting is that the task arrives alone.

The stream format is specified in
[`docs/tasks/task_stream.md`](https://github.com/AOS-Bench/AOS-Bench/blob/main/docs/tasks/task_stream.md)
in the AOS-Bench repository.

## Getting the data

Some environment inputs (videos, archives, PDFs and one large TSV) are stored with Git LFS.
Install it before cloning:

```bash
git lfs install
git clone https://github.com/AOS-Bench/AOS-Bench-Task.git data
```

## Images

Every `stream.toml` and `task.toml` points to a prebuilt image on Docker Hub,
`zhuerle/aosbench:<workload-id>`. Harbor pulls it on first use. Docker Hub rate-limits
anonymous pulls, so run `docker login` first. To build from the Dockerfiles instead, pass
`--force-build` to `harbor run`; some images take more than an hour to build.

The Dockerfiles install system and Python packages from `mirrors.cloud.tencent.com`, the mirror
used when the images were built. It is publicly reachable, but may be slow outside mainland
China. The prebuilt images do not depend on it at run time.

## Running

See the AOS-Bench [README](https://github.com/AOS-Bench/AOS-Bench#quick-start) and
[getting-started guide](https://github.com/AOS-Bench/AOS-Bench/blob/main/docs/getting_started.md).
For example, from `AOS-Bench/harbor`:

```bash
uv run harbor run \
  --path ../data/stream/V1-0/valid-0917/medium/v1-medium-10-cdf6049a \
  --task-type stream --aos-pool-v03 --env docker \
  --agent claude-code --model claude-opus-5 \
  --ae ANTHROPIC_API_KEY="$ANTHROPIC_API_KEY" \
  --agent-timeout-sec 9000 --jobs-dir ../jobs --n-attempts 1 --yes
```

A few WildClawBench graders call an LLM judge through `OPENROUTER_API_KEY`; pass it with `--ve`.

## Known issues

- **Shared output file.** Three tasks (`onebrc-onebrc-400m`, `terminalworld-tw_593620` and
  `terminalworld-tw_528959`) grade the same file, `/workspace/result.txt`. Five medium
  workloads each contain two of them, and those two cannot both pass.
- **Fixed `doom.wad`.** In the images for `v1-medium-10-080f10d5`, `v1-medium-10-cdf6049a`,
  `v1-medium-10-ce75e317` and `v1-heavy-20-55dcdddf`, the paper runs had a 341-byte HTML page in
  place of `/workspace/doom.wad`, because the download URL had stopped working. The published
  images add one layer with the real shareware IWAD; the Dockerfiles fetch it from a working URL
  and check its sha256. This affects `tb2-make-mips-interpreter` and `tb2-make-doom-for-mips`.
  The `v1-heavy-20-55dcdddf` image is also flattened to a single layer.
- **Excluded tasks.** The paper excludes tasks whose reference solution fails and tasks whose
  grader produced a reward without any test result. The scoring rules are listed in the
  AOS-Bench README; the released trajectories on
  [Hugging Face](https://huggingface.co/datasets/AOS-Bench/AOS-Bench-Trajectories-v2) record
  which tasks were scored.

## Sources and licenses

The tasks are adapted from existing benchmarks; [`SOURCES.md`](SOURCES.md) lists each source,
its license and its citation. Upstream material stays under its original license, and files that
carry their own license notice are governed by that notice. Material written for AOS-Bench (the
workload definitions, merged environments, adapted graders and the synthetic `aos-synth-*` tasks)
is released under the [MIT License](LICENSE).

## Citation

```bibtex
@article{zhu2026aosbench,
  title   = {AOS-Bench: Can LLM Agents Manage Dynamic Workloads?},
  author  = {Zhu, Erle and Zhang, Genrui and Hu, Keyang and Su, Mingyu and Wang, Hongning},
  journal = {arXiv preprint arXiv:2610.33178},
  year    = {2026}
}
```

Please also cite the source benchmarks of the tasks you use (see `SOURCES.md`).
