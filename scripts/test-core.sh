#!/usr/bin/env bash
# Builds and runs the src/core unit tests with any C++20 compiler (no Qt or CMake needed).
set -euo pipefail
cd "$(dirname "$0")/.."
CXX="${CXX:-g++}"
out="${TMPDIR:-/tmp}/rift-core-tests"; mkdir -p "$(dirname "$out")"
extra=""; case "$(uname -s)" in MINGW*|MSYS*) extra="-static";; esac
"$CXX" $extra -std=c++20 -O1 -Wall -Wextra -Werror -o "$out" tests/core_tests.cpp src/core/segments.cpp src/core/text.cpp src/core/update.cpp
"$out"
