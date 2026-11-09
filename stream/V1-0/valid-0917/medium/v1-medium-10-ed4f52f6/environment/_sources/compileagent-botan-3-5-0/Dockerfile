FROM --platform=linux/amd64 ubuntu:24.04

# CN-friendly apt mirror (set AOS_DOCKERFILE_MIRROR conventions upstream if this fails)
RUN sed -i 's|http://archive.ubuntu.com/ubuntu|http://mirrors.tuna.tsinghua.edu.cn/ubuntu|g; s|http://security.ubuntu.com/ubuntu|http://mirrors.tuna.tsinghua.edu.cn/ubuntu|g' /etc/apt/sources.list.d/ubuntu.sources

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    ca-certificates \
    curl \
    python3 \
    file \
    && rm -rf /var/lib/apt/lists/*

# Vendor the pinned source tree into the image at BUILD time.
# The agent works fully offline: no network is available at solve time.
RUN mkdir -p /workdir/src \
    && curl -fsSL "https://ghfast.top/https://github.com/randombit/botan/archive/refs/tags/3.5.0.tar.gz" -o /tmp/src.tar.gz \
    && tar -xzf /tmp/src.tar.gz -C /workdir/src --strip-components=1 \
    && rm /tmp/src.tar.gz

WORKDIR /workdir

# system npm so harbor's agent installer doesn't need nvm from github raw (CN-unreachable)
RUN apt-get update && apt-get install -y --no-install-recommends nodejs npm \
 && rm -rf /var/lib/apt/lists/* && npm config set registry https://registry.npmmirror.com
