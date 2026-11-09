#!/bin/bash
# Oracle solve for primesieve-pi-1e13.
# Uses the primesieve library CLI (pre-installed in the base image).
set -euo pipefail
mkdir -p /workspace
primesieve 10000000000000 --quiet > /workspace/answer.txt
echo "wrote /workspace/answer.txt: $(cat /workspace/answer.txt)"
