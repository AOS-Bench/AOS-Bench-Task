#!/bin/bash
set -ex

# Already in /workspace per Dockerfile
tar -xzf coreutils-9.7.tar.gz
cd coreutils-9.7

# Configure with prefix and static linking
# Disable stdbuf to avoid shared library issues
FORCE_UNSAFE_CONFIGURE=1 ./configure \
  --prefix=/workspace/result \
  --enable-no-install-program=stdbuf \
  LDFLAGS='-static'

# Build and install
make -j$(nproc)
make install

# Create symlinks for all utilities in /workspace/result
cd /workspace/result
for utility in bin/*; do
    if [ -f "$utility" ] && [ -x "$utility" ]; then
        ln -sf "$utility" "$(basename "$utility")"
    fi
done
