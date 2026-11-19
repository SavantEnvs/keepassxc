#!/usr/bin/env bash
#
# mayhem/test.sh — RUN every test KeePassXC's CMake registers for this feature set (the list
# ctest reports, written by mayhem/build.sh to build-tests/mayhem-tests.txt), built into build-tests/
# with the project's normal flags. Every QtTest binary prints `Totals: P passed, F failed,
# S skipped, ...` per test class; those asserted results are summed. A binary that exits non-zero or
# prints no Totals line counts as a failure, so a neutered (exit 0, no output) binary fails the
# oracle. Upstream's few CMake-script tests (no binary) run through ctest and count as one each.
#
# IMPORTANT:
#  * Must assert BEHAVIOR/OUTPUT, not just exit status. The oracle has to check asserted values /
#    golden-output diffs / known-answer results — so a PATCH that "fixes" a bug by making the program
#    exit(0) (or any no-op) FAILS here. Running inputs and checking only "exit 0 / didn't crash" is
#    NOT a functional test (it's trivially reward-hackable) — use the project's real assertion suite.
#  * Do NOT build here — mayhem/build.sh already compiled the test suite (with the project's normal
#    flags). This script only RUNS the pre-built tests and reports counts. If the test runner is
#    missing, that's a build.sh bug — fail loudly rather than silently rebuilding.
#  * REQUIRED OUTPUT — a CTRF (https://ctrf.io) summary so Mayhem/the PATCH grader reads the counts:
#      - writes a CTRF JSON report to ${CTRF_REPORT:-$SRC/ctrf-report.json}, and
#      - prints a one-line `CTRF {...}` marker to stdout (same JSON, compact).
#    Only `results.summary` (with tests/passed/failed/pending/skipped/other) is required.
#    Use the emit_ctrf helper below; it computes tests = passed+failed+skipped and sets the exit
#    code (0 iff failed==0). Map your framework's output to passed/failed/skipped.
set -uo pipefail
[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH
: "${MAYHEM_JOBS:=$(nproc)}"   # build parallelism; env-overridable, falls back to nproc (use -j"$MAYHEM_JOBS")
cd "$SRC"

# emit_ctrf <tool> <passed> <failed> [skipped] [pending] [other]
# Writes a CTRF report (file + stdout `CTRF {...}` marker) and returns non-zero iff failed>0.
emit_ctrf() {
  local tool="$1" passed="$2" failed="$3" skipped="${4:-0}" pending="${5:-0}" other="${6:-0}"
  local tests=$(( passed + failed + skipped + pending + other ))
  cat > "${CTRF_REPORT:-$SRC/ctrf-report.json}" <<JSON
{
  "results": {
    "tool": { "name": "$tool" },
    "summary": {
      "tests": $tests,
      "passed": $passed,
      "failed": $failed,
      "pending": $pending,
      "skipped": $skipped,
      "other": $other
    }
  }
}
JSON
  printf 'CTRF {"results":{"tool":{"name":"%s"},"summary":{"tests":%d,"passed":%d,"failed":%d,"pending":%d,"skipped":%d,"other":%d}}}\n' \
    "$tool" "$tests" "$passed" "$failed" "$pending" "$skipped" "$other"
  [ "$failed" -eq 0 ]
}

TESTDIR="$SRC/build-tests/tests"
LIST="$SRC/build-tests/mayhem-tests.txt"
if [ ! -f "$LIST" ]; then
  echo "test.sh: $LIST missing — run mayhem/build.sh first" >&2
  emit_ctrf "qtest" 0 1 0
  exit 1
fi

# C.UTF-8, not ctest's en_US.UTF-8: the image ships no en_US locale, and Qt prints a
# "switched to C.UTF-8" warning into keepassxc-cli's output, which breaks TestCli::testNonAscii.
export LANG=C.UTF-8 QT_QPA_PLATFORM=offscreen

# Test functions left out, each with the reason (everything else in every suite runs):
#  testcli:testClip  needs a clipboard program (xclip/wl-copy) talking to a real display; in this
#                    headless image the copy fails and the test only QSKIPs some failure messages.
EXCLUDE=(testcli:testClip)

# args_for <suite>: no args (run every function), or the suite's functions minus EXCLUDE.
args_for() {
  local t="$1" e fns=() fn drop
  local ex=()
  for e in "${EXCLUDE[@]}"; do [ "${e%%:*}" = "$t" ] && ex+=("${e#*:}"); done
  [ ${#ex[@]} -gt 0 ] || return 0
  while read -r fn; do
    fn="${fn%()}"; [ -n "$fn" ] || continue
    drop=0; for e in "${ex[@]}"; do [ "$fn" = "$e" ] && drop=1; done
    [ "$drop" -eq 1 ] || fns+=("$fn")
  done < <("$TESTDIR/$t" -functions 2>/dev/null)
  printf '%s\n' "${fns[@]}"
}
passed=0 failed=0 skipped=0
while read -r t; do
  [ -n "$t" ] || continue
  if [ ! -x "$TESTDIR/$t" ]; then
    # Not a test binary: one of upstream's CMake-script tests; ctest knows its command.
    log=$(mktemp)
    if ctest --test-dir "$SRC/build-tests" -R "^${t}\$" --output-on-failure >"$log" 2>&1 \
       && grep -q '100% tests passed' "$log"; then
      echo "$t: 1 passed (ctest)"; passed=$((passed + 1))
    else
      echo "FAIL $t (ctest)"; tail -20 "$log"; failed=$((failed + 1))
    fi
    rm -f "$log"
    continue
  fi
  mapfile -t args < <(args_for "$t")
  out=$("$TESTDIR/$t" "${args[@]}" 2>&1); rc=$?
  totals=$(printf '%s\n' "$out" | grep -E '^Totals: [0-9]+ passed, [0-9]+ failed, [0-9]+ skipped' || true)
  if [ -z "$totals" ]; then
    echo "FAIL $t: no QtTest Totals line (exit $rc)"; printf '%s\n' "$out" | tail -20
    failed=$((failed + 1)); continue
  fi
  read -r p f s < <(printf '%s\n' "$totals" | awk '{p+=$2; f+=$4; s+=$6} END {print p, f, s}')
  if [ "$rc" -ne 0 ] && [ "$f" -eq 0 ]; then f=1; fi
  echo "$t: $p passed, $f failed, $s skipped (exit $rc)"
  [ "$f" -eq 0 ] || printf '%s\n' "$out" | grep -E '^(FAIL!|   Loc:|   Actual|   Expected)' | head -40
  passed=$((passed + p)); failed=$((failed + f)); skipped=$((skipped + s))
done < "$LIST"

emit_ctrf "qtest" "$passed" "$failed" "$skipped"
