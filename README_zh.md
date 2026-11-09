# AOS-Bench-Task

[AOS-Bench](https://github.com/AOS-Bench/AOS-Bench) 的工作负载和任务。AOS-Bench 把 LLM agent
系统当作工作负载管理者来评测。本仓库只包含论文使用的工作负载（Harbor 任务格式），以及把同一批
任务逐个拆开、供 individual（单任务）基线使用的目录。[English](README.md)

## 内容

| 路径 | 内容 |
|---|---|
| `stream/V1-0/valid-0917/medium/` | 10 个工作负载，每个 10 个任务 |
| `stream/V1-0/valid-0917/heavy/` | 5 个工作负载，每个 20 个任务 |
| `stream/V1-0/valid-0917/heavy-plus/` | 1 个 30 任务的工作负载（案例研究） |
| `stream/V1-0/valid-0917-individual-dedup/<tier>/` | 同一批任务，每个不重复的任务一个目录，用于 individual 运行 |

每个工作负载目录包含 `stream.toml`（资源、预构建镜像、释放计划）、`environment/`（合并后的
Dockerfile 及各任务的构建输入）、`tasks/<task-id>/`（指令、验证器、参考解）和
`solution/solve.sh`（按释放顺序运行全部参考解）。individual 任务的 `task.toml` 沿用其所属工作
负载的镜像、核数、内存和网络设置，唯一的区别是该任务单独到达。

## 获取数据

部分环境文件（视频、压缩包、PDF 和一个较大的 TSV）使用 Git LFS 存储，clone 前先安装：

```bash
git lfs install
git clone https://github.com/AOS-Bench/AOS-Bench-Task.git data
```

## 镜像

所有 `stream.toml` 和 `task.toml` 都指向 Docker Hub 上的预构建镜像
`zhuerle/aosbench:<workload-id>`，Harbor 首次使用时自动拉取。建议先 `docker login`，以免触发
匿名拉取限额。如需从 Dockerfile 构建，在 `harbor run` 中加上 `--force-build`。Dockerfile 使用
`mirrors.cloud.tencent.com` 镜像源安装软件包；预构建镜像在运行时不依赖它。

## 运行

见 AOS-Bench 的 [README](https://github.com/AOS-Bench/AOS-Bench/blob/main/README_zh.md) 和
[入门指南](https://github.com/AOS-Bench/AOS-Bench/blob/main/docs/getting_started_zh.md)。

## 已知问题

- 有三个任务（`onebrc-onebrc-400m`、`terminalworld-tw_593620`、`terminalworld-tw_528959`）的
  验证器检查同一个文件 `/workspace/result.txt`。五个 medium 工作负载各包含其中两个，这两个任务
  无法同时通过。
- 在 `v1-medium-10-080f10d5`、`v1-medium-10-cdf6049a`、`v1-medium-10-ce75e317` 和
  `v1-heavy-20-55dcdddf` 的镜像中，论文运行时 `/workspace/doom.wad` 是一个 341 字节的 HTML 页面（原
  下载地址已失效）。发布的镜像多加了一层，放入正确的 shareware IWAD；Dockerfile 改为从可用地址
  下载并校验 sha256。受影响的是 `tb2-make-mips-interpreter` 和 `tb2-make-doom-for-mips`。
  `v1-heavy-20-55dcdddf` 的镜像另外被压平成一层。
- 论文计分时排除了参考解失败的任务，以及验证器在没有任何测试结果时写入分数的任务，规则见
  AOS-Bench README。

## 来源与许可

任务改编自现有 benchmark，来源、许可和引用见 [`SOURCES.md`](SOURCES.md)。上游内容沿用其原
许可，自带许可声明的文件以该声明为准。为 AOS-Bench 编写的内容（工作负载定义、合并环境、改写的
验证器和 `aos-synth-*` 合成任务）以 [MIT License](LICENSE) 发布。
