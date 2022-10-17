#!/usr/bin/env bash
set -e
if [[ -n $1 ]]; then
  if [[ ! -z "$LD_LIBRARY_PATH" ]]; then
    prefix="$LD_LIBRARY_PATH:"
  fi
  export LD_LIBRARY_PATH="$prefix$1"
fi
cd "$(dirname "$0")"
python3 -m unittest discover tests
