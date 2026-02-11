#!/bin/bash
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

cd "$PROJECT_DIR"
astyle \
--options="$SCRIPT_DIR/astyle.cfg" \
--recursive "include/*.h" "tests/*.c" \
$1 $2 $3 # additional args such as --dry-run etc.
