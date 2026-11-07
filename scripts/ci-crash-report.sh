#!/usr/bin/env bash
# Run a command (CI smoke tests) with its output in <log>. If it crashes (signal exit, >= 128), run it again
# under gdb and turn the backtrace into GitHub annotations, which are public, unlike the job log.
# Exits with the command's code. Follows exec, so a launcher script (AppImage AppRun) works too.
#   bash scripts/ci-crash-report.sh <label> <log> <command> [args...]
label=$1 log=$2; shift 2
"$@" > "$log" 2>&1 < /dev/null; rc=$?
cat "$log"
if [ "$rc" -ge 128 ] && command -v gdb >/dev/null; then
    echo "::error::$label crashed (exit $rc, signal $((rc - 128))). Backtrace follows."
    gdb -batch -q -ex 'set pagination off' -ex 'set follow-exec-mode same' -ex run -ex bt \
        --args "$@" > "$log.gdb" 2>&1 < /dev/null
    cat "$log.gdb"
    # Frames only ("#0  0x... in fn (...) at file:line"), shortened to fit an annotation.
    grep -E '^#[0-9]+ ' "$log.gdb" | head -n 18 | cut -c1-240 | sed "s/^/::error::$label bt /"
    # The last lines the app printed before dying often name the culprit too.
    grep -v '^\s*$' "$log" | tail -n 6 | cut -c1-240 | sed "s/^/::error::$label log /"
fi
exit "$rc"
