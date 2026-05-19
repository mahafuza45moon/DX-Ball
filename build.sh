#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
CMAKE=$(python3 -c "import cmake; import os; print(os.path.join(cmake.CMAKE_BIN_DIR, 'cmake'))" 2>/dev/null || command -v cmake)

cd "$SCRIPT_DIR"

"$CMAKE" -S . -B build
"$CMAKE" --build build --parallel --verbose
