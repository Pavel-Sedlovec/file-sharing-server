#!/usr/bin/env sh
set -eu

ROOT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
BUILD_DIR=$(mktemp -d "${TMPDIR:-/tmp}/file-sharing-tests.XXXXXX")
trap 'rm -rf "$BUILD_DIR"' EXIT HUP INT TERM

CXX=${CXX:-c++}
"$CXX" -std=c++17 -Wall -Wextra -Wpedantic \
    -I"$ROOT_DIR/include" \
    "$ROOT_DIR/tests/test_main.cpp" \
    "$ROOT_DIR/src/protocol/command_buffer.cpp" \
    "$ROOT_DIR/src/storage/password_hash.cpp" \
    -lcrypt \
    -o "$BUILD_DIR/unit_tests"

"$BUILD_DIR/unit_tests"
