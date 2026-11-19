#!/usr/bin/env bash
#
# mayhem/build.sh — build KeePassXC's libFuzzer harnesses and the unit tests mayhem/test.sh runs.
#
# Runs inside the commit image as `mayhem` in /mayhem. The base image exports CC/CXX,
# SANITIZER_FLAGS (ASan+UBSan, halting), LIB_FUZZING_ENGINE, STANDALONE_FUZZ_MAIN and SRC.
#
#  1) build-fuzz/: upstream's CMake build of its static libraries (keepassxc_core, keepassxc_gui,
#     browser, sshagent, ...) with $SANITIZER_FLAGS + SanitizerCoverage + DWARF 3, plus the harnesses
#     in mayhem/fuzz, which mayhem/fuzz/inject.cmake adds to upstream's own build tree (no upstream
#     file is edited). Every fuzz and -standalone binary links mayhem/lsan_off.cc (LeakSanitizer
#     off; ASan and UBSan stay on). Binaries land in build-fuzz/mayhem-bin/.
#  2) build-tests/: ALL of upstream's tests (every `add_test` registered for this feature set), with
#     NORMAL flags; their names go to build-tests/mayhem-tests.txt for mayhem/test.sh.
#
# Both build dirs match upstream's `build*/` .gitignore entry, so `git clean -ffdX` removes them.
# Re-running is incremental (CMake/Ninja), and nothing is fetched: every dependency is an apt
# package installed by mayhem/Dockerfile.
set -euo pipefail

[ -n "${SOURCE_DATE_EPOCH:-}" ] || unset SOURCE_DATE_EPOCH

: "${SANITIZER_FLAGS=-fsanitize=address,undefined -fno-sanitize-recover=all -fno-omit-frame-pointer}"
: "${DEBUG_FLAGS:=-g -gdwarf-3}"
: "${CC:=clang}" ; : "${CXX:=clang++}" ; : "${LIB_FUZZING_ENGINE:=-fsanitize=fuzzer}"
: "${STANDALONE_FUZZ_MAIN:=/opt/mayhem/StandaloneFuzzTargetMain.c}"
: "${MAYHEM_JOBS:=$(nproc)}"
: "${COVERAGE_FLAGS=}"
export SANITIZER_FLAGS DEBUG_FLAGS CC CXX LIB_FUZZING_ENGINE STANDALONE_FUZZ_MAIN MAYHEM_JOBS COVERAGE_FLAGS

cd "$SRC"

# Features that need a desktop session, the network, or external tools are off; the
# browser-integration and SSH-agent code (both parse untrusted data) stay on.
FEATURES=(
  -DWITH_X11=OFF
  -DKPXC_FEATURE_DOCS=OFF
  -DKPXC_FEATURE_NETWORK=OFF
  -DKPXC_FEATURE_UPDATES=OFF
  -DKPXC_FEATURE_FDOSECRETS=OFF
  -DKPXC_FEATURE_BROWSER=ON
  -DKPXC_FEATURE_SSHAGENT=ON
)

# 1) Sanitized + coverage-instrumented project and harnesses. -fsanitize=fuzzer-no-link adds
#    SanitizerCoverage to every upstream object so the fuzzer sees the parsers' edges. DEBUG_FLAGS
#    come last so -gdwarf-3 wins. No CMAKE_BUILD_TYPE, so -Werror (Debug only) stays off; Qt then
#    defines QT_NO_DEBUG, and -DQT_FORCE_ASSERTS keeps every Q_ASSERT/Q_ASSERT_X live anyway (the
#    project's invariant checks abort under the fuzzer) without turning on QT_DEBUG code paths.
FUZZ_FLAGS="-O1 $SANITIZER_FLAGS -fsanitize=fuzzer-no-link -DQT_FORCE_ASSERTS $DEBUG_FLAGS"
cmake -S . -B build-fuzz -G Ninja \
  -DWITH_TESTS=OFF \
  "${FEATURES[@]}" \
  -DCMAKE_C_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_CXX_FLAGS="$FUZZ_FLAGS" \
  -DCMAKE_PROJECT_KeePassXC_INCLUDE="$SRC/mayhem/fuzz/inject.cmake" \
  -DMAYHEM_LSAN_OFF_SRC="$SRC/mayhem/lsan_off.cc"
cmake --build build-fuzz -j"$MAYHEM_JOBS" --target mayhem_fuzzers

# 2) Upstream's whole test suite, with the project's normal flags (no sanitizers): build every
#    target, then record every test ctest registers (the QtTest unit tests plus upstream's CMake
#    script tests) so test.sh runs them all, as upstream's `ctest` does.
cmake -S . -B build-tests -G Ninja \
  -DWITH_TESTS=ON \
  "${FEATURES[@]}" \
  -DCMAKE_C_FLAGS="$COVERAGE_FLAGS" \
  -DCMAKE_CXX_FLAGS="$COVERAGE_FLAGS"
cmake --build build-tests -j"$MAYHEM_JOBS"
ctest --test-dir build-tests -N | sed -n 's/^ *Test *#[0-9]*: *//p' > build-tests/mayhem-tests.txt
[ -s build-tests/mayhem-tests.txt ] || { echo "build.sh: ctest registered no tests" >&2; exit 1; }

ls -l build-fuzz/mayhem-bin/
