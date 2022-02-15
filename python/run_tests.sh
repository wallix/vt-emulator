#!/usr/bin/env bash
set -e
if [[ -n $1 ]]; then
  export LD_LIBRARY_PATH="$1"
fi
cd "$(dirname "$0")"
python3 -m unittest discover tests
