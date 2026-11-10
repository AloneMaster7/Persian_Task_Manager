#!/usr/bin/env bash
set -euo pipefail

# Run this script from the MSYS2 UCRT64 terminal.
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SOURCE="$SCRIPT_DIR/TaskManager.cpp"
OUTPUT="$SCRIPT_DIR/TaskManager.exe"

if [[ "${MSYSTEM:-}" != "UCRT64" ]]; then
    echo "Error: open the MSYS2 UCRT64 terminal, then run: bash build.sh" >&2
    exit 1
fi

if ! command -v g++ >/dev/null 2>&1; then
    echo "Error: g++ was not found. Install the UCRT64 GCC toolchain in MSYS2." >&2
    exit 1
fi

if [[ ! -f "$SOURCE" ]]; then
    echo "Error: TaskManager.cpp was not found beside build.sh." >&2
    exit 1
fi

echo "Building Persian Win32 Task Manager with MSYS2 UCRT64..."
g++ -std=c++11 -O2 -Wall -finput-charset=UTF-8 -mwindows \
    "$SOURCE" -lcomctl32 -ladvapi32 -static-libgcc -static-libstdc++ -o "$OUTPUT"
echo "Build successful: $OUTPUT"
