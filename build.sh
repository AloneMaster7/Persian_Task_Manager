#!/usr/bin/env bash
set -euo pipefail

if [[ "${MSYSTEM:-}" != "UCRT64" ]]; then
    echo "Open the MSYS2 UCRT64 terminal, then run: bash build.sh" >&2
    exit 1
fi

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"
g++ -std=c++11 -O2 -Wall -finput-charset=UTF-8 -mwindows \
    TaskManager.cpp -lcomctl32 -ladvapi32 -static-libgcc -static-libstdc++ \
    -o TaskManager.exe
echo "Build successful: $SCRIPT_DIR/TaskManager.exe"
