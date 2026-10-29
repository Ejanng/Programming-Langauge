#!/usr/bin/env bash
# Runs every examples/*.bc and compares the output (stdout and stderr) with the matching .expected file.
# Usage: tests/run_tests.sh [path-to-mylang-binary]
set -u
cd "$(dirname "$0")/.."
bin="${1:-./mylang}"
pass=0
fail=0
for src in examples/*.bc; do
  expected="${src%.bc}.expected"
  # tr strips carriage returns, so Windows (CRLF) output compares equal to the LF .expected files.
  actual="$("$bin" "$src" 2>&1 | tr -d '\r')"
  if [ "$actual" == "$(tr -d '\r' < "$expected")" ]; then
    pass=$((pass + 1))
    echo "PASS $src"
  else
    fail=$((fail + 1))
    echo "FAIL $src"
    diff <(echo "$actual") "$expected" | head -n 10
  fi
done
echo "$pass passed, $fail failed"
[ "$fail" -eq 0 ]
