#!/bin/bash
# Oracle solution: straightforward build of the vendored source.
set -ex

cd /workspace/src
./configure.py
make -j4
