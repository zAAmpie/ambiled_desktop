#!/usr/bin/env bash
# Per-thread CPU usage of a running ambiled_desktop over a fixed window, read from /proc.
# Usage: measure_live.sh <label> [seconds=30]      (override the process with PID=1234)
set -euo pipefail
label="${1:?usage: measure_live.sh <label> [seconds]}"
secs="${2:-30}"
pid="${PID:-$(pidof ambiled_desktop)}"
clk=$(getconf CLK_TCK)

# One line per thread: tid name cpu-ticks(utime+stime)
snapshot() {
    for task in /proc/"$pid"/task/*; do
        local comm stat rest
        comm=$(tr ' ' '_' < "$task/comm") || continue
        stat=$(< "$task/stat") || continue
        rest=${stat##*) }            # drop "pid (comm) " - comm may contain spaces/parens
        set -- $rest                 # $1 = field 3 (state), so utime = $12, stime = $13
        echo "${task##*/} $comm $(( ${12} + ${13} ))"
    done
}

startTime=$(date +%T)
before=$(snapshot)
sleep "$secs"
after=$(snapshot)

echo "== $label  ($startTime - $(date +%T), ${secs}s, pid $pid) =="
awk -v secs="$secs" -v clk="$clk" '
    NR == FNR { start[$1] = $3; next }
    { delta = $3 - ($1 in start ? start[$1] : 0); pct = 100 * delta / clk / secs
      total += pct; if (pct >= 0.1) printf "  %-18s %6.1f%%\n", $2, pct }
    END { printf "  %-18s %6.1f%%  (100%% = one core)\n", "TOTAL", total }
' <(echo "$before") <(echo "$after") | sort -k2 -n -r
