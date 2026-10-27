#!/usr/bin/env bash
# Runs the benchmarks and prints one line per case: name, resolution, wall ms/iter, CPU ms/iter (all threads).
# QBENCHMARK re-invokes each case while calibrating, so the last "cpu:" line before each RESULT is the real one.
# Usage: summarize.sh <path-to-bench-binary> [extra QtTest args, e.g. a test function name]
set -euo pipefail
bench="$1"; shift
"$bench" -minimumvalue 200 "$@" 2>&1 | awk '
    /^QINFO .*cpu:/ { match($0, /cpu: ([^ ]+)/, m); cpu = m[1] }
    /^RESULT/       { match($0, /Bench::([^:]+):("([^"]+)")?/, n); n[2] = n[3]; getline;
                      printf "%-30s %-6s wall %9.4f ms   cpu %9.4f ms\n", n[1], n[2], $1, cpu }
'
