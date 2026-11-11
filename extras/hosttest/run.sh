#!/usr/bin/env bash
# Compile and run the host-side unit tests (no Arduino toolchain required).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")" && pwd)"
SRC="$(cd "$ROOT/../../src" && pwd)"
OUT="$(mktemp)"
g++ -std=c++17 -Wall -Wextra -I"$ROOT" \
  "$ROOT/test.cpp" \
  "$SRC/GeorgianCore.cpp" \
  "$SRC/GeorgianLCDGlyphs.cpp" \
  -o "$OUT"
"$OUT"
rc=$?
rm -f "$OUT"
exit $rc
