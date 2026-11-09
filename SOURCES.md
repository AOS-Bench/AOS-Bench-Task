# Task sources

Task IDs carry the source as a prefix. Each source's material remains under its original
license. Individual files that carry their own license notice (for example the bundled skill
files under `skills/*/LICENSE.txt` in some SkillsBench tasks) are governed by that notice.

| Prefix | Source | Repository | License |
|---|---|---|---|
| `tb2-` | Terminal-Bench 2.0 | [harbor-framework/terminal-bench-2](https://github.com/harbor-framework/terminal-bench-2) | Apache-2.0 |
| `tb3-` | Terminal-Bench 3 | [harbor-framework/terminal-bench](https://github.com/harbor-framework/terminal-bench) | Apache-2.0 |
| `skillsbench-` | SkillsBench | [benchflow-ai/skillsbench](https://github.com/benchflow-ai/skillsbench) | Apache-2.0 |
| `terminalworld-` | TerminalWorld | [EuniAI/TerminalWorld](https://github.com/EuniAI/TerminalWorld) | Apache-2.0 |
| `wildclawbench-` | WildClawBench | [InternLM/WildClawBench](https://github.com/InternLM/WildClawBench) | MIT |
| `compilebench-` | CompileBench | [QuesmaOrg/CompileBench](https://github.com/QuesmaOrg/CompileBench) | MIT |
| `compileagent-` | Tasks written for AOS-Bench from the CompileAgent project list | see the citation below | MIT (AOS-Bench); the compiled projects keep their own licenses |
| `pareval-` | ParEval | [parallelcodefoundry/ParEval](https://github.com/parallelcodefoundry/ParEval) | MIT |
| `onebrc-` | The One Billion Row Challenge | [gunnarmorling/1brc](https://github.com/gunnarmorling/1brc) | Apache-2.0 |
| `aos-synth-` | Synthetic tasks written for AOS-Bench | this repository | MIT |

The tasks were converted to the Harbor task format, and some graders and environments were
modified so that they run offline and score reliably. Comments in the affected `task.toml`,
`Dockerfile` and `tests/` files describe each change.

## Citations

- **Terminal-Bench.** Mike A. Merrill, Alexander G. Shaw, Nicholas Carlini, et al.
  *Terminal-Bench: Benchmarking Agents on Hard, Realistic Tasks in Command Line Interfaces.*
  ICLR 2026. https://openreview.net/forum?id=a7Qa4CcHak
- **SkillsBench.** Xiangyi Li, Yimin Liu, Wenbo Chen, et al. *SkillsBench.*
  https://github.com/benchflow-ai/skillsbench
- **TerminalWorld.** Zhaoyang Chu, Jiarui Hu, Xingyu Jiang, et al. *TerminalWorld: Benchmarking
  Agents on Real-World Terminal Tasks.* arXiv:2605.22535, 2026.
- **WildClawBench.** Shuangrui Ding, Xuanlang Dai, Long Xing, et al. *WildClawBench.* 2026.
  https://github.com/InternLM/WildClawBench
- **CompileBench.** Quesma. *CompileBench.* 2025. https://github.com/QuesmaOrg/CompileBench
- **CompileAgent.** Li Hu, Guoqiang Chen, Xiuwei Shang, et al. *CompileAgent: Automated
  Real-World Repo-Level Compilation with Tool-Integrated LLM-based Agent System.* ACL 2025.
- **ParEval.** Daniel Nichols, Joshua H. Davis, Zhaojun Xie, Arjun Rajaram, and Abhinav Bhatele.
  *Can Large Language Models Write Parallel Code?* HPDC 2024.
- **One Billion Row Challenge.** Gunnar Morling. 2024. https://github.com/gunnarmorling/1brc
