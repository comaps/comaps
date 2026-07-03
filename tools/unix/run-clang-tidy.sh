#!/usr/bin/env bash
set -euo pipefail
BUILD_DIR="${BUILD_DIR:-../omim-build-debug}"
COMPILE_DB="$BUILD_DIR/compile_commands.json"
EXCLUDE_DIRS="3party|build|omim-build"

if [ ! -f "$COMPILE_DB" ]; then
  echo "compile_commands.json not found at $COMPILE_DB"
  exit 1
fi

if [ $# -eq 0 ]; then
  FILES=$(git diff --name-only origin/main...HEAD -- '*.cpp' '*.hpp' | grep -vE "^($EXCLUDE_DIRS)/" || true)
elif [ $# -eq 1 ] && [[ "$1" == *".."* ]]; then
  FILES=$(git diff --name-only "$1" -- '*.cpp' '*.hpp' | grep -vE "^($EXCLUDE_DIRS)/" || true)
else
  FILES="$*"
fi

if [ -z "$FILES" ]; then exit 0; fi
echo "$FILES" | xargs -r -P4 clang-tidy -p "$BUILD_DIR" --quiet
