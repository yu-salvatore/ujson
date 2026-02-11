#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

SRC="$PROJECT_DIR/tests/test_conformance.c"
BIN="$PROJECT_DIR/tests/build/test_conformance"
INCLUDE="$PROJECT_DIR/include"
TEST_DATA="$PROJECT_DIR/tests/vendor/JSONTestSuite/test_parsing"

mkdir -p "$(dirname "$BIN")"

CFLAGS="-Wall -Wextra -pedantic"

# Optional sanitizers: ./run_tests.sh --sanitize
if [ "$1" = "--sanitize" ]; then
    CFLAGS="$CFLAGS -fsanitize=address,undefined -fno-omit-frame-pointer"
fi

echo "Compiling..."
cc $CFLAGS \
    -I"$INCLUDE" \
    -DTEST_PARSING_DIR="\"$TEST_DATA\"" \
    "$SRC" -o "$BIN" || exit 1

echo "Running conformance tests..."
echo
"$BIN"
