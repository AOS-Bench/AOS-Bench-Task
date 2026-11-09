#!/bin/bash
# Oracle solution: straightforward build of the vendored source.
set -ex

cd /workspace/src
./config
make -j4
