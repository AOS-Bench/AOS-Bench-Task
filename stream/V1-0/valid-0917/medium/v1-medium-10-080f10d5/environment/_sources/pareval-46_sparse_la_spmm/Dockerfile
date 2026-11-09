FROM --platform=linux/amd64 ubuntu:24.04

# CN-friendly apt mirror (ubuntu 24.04 uses deb822 sources)
RUN sed -i 's|http://archive.ubuntu.com/ubuntu|http://mirrors.tuna.tsinghua.edu.cn/ubuntu|g; s|http://security.ubuntu.com/ubuntu|http://mirrors.tuna.tsinghua.edu.cn/ubuntu|g' /etc/apt/sources.list.d/ubuntu.sources

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    g++ \
    make \
    python3 \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workdir

# Function-signature stub the agent should complete (also described in the instructions).
COPY generated-code.hpp /workdir/generated-code.hpp
