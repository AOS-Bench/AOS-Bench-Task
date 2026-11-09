FROM python:3.12-slim

ENV DEBIAN_FRONTEND=noninteractive

# CN-friendly mirrors (debian bookworm deb822 sources) + pip index
RUN sed -i 's|deb.debian.org|mirrors.tuna.tsinghua.edu.cn|g' /etc/apt/sources.list.d/debian.sources \
    && apt-get update \
    && apt-get install -y --no-install-recommends gcc make libc6-dev \
    && rm -rf /var/lib/apt/lists/* \
    && pip config set global.index-url https://pypi.tuna.tsinghua.edu.cn/simple

WORKDIR /workdir

# Agent-visible inputs. The image carries NO grading assets: the expected
# output lives in tests/expected_output.txt (precomputed at authoring time
# with the same deterministic generate.sh + ref.py pipeline) and is copied to
# /tests by the harbor Verifier only AFTER the agent run ends, so it is never
# reachable from the solve-phase container filesystem.
COPY weather_stations.csv gen.py generate.sh /workdir/
RUN chmod +x /workdir/generate.sh /workdir/gen.py
