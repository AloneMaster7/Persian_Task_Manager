#!/usr/bin/env bash
set -euo pipefail

# Run this script from the MSYS2 UCRT64 terminal.
SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
SOURCE="$SCRIPT_DIR/TaskManager.cpp"
ICON="$SCRIPT_DIR/icon.ico"
OUTPUT="$SCRIPT_DIR/TaskManager.exe"

if [[ "${MSYSTEM:-}" != "UCRT64" ]]; then
    echo "Error: open the MSYS2 UCRT64 terminal, then run: bash build.sh" >&2
    exit 1
fi

for tool in g++ windres cygpath; do
    if ! command -v "$tool" >/dev/null 2>&1; then
        echo "Error: $tool was not found. Install the MSYS2 UCRT64 GCC/binutils toolchain." >&2
        exit 1
    fi
done

if [[ ! -f "$SOURCE" ]]; then
    echo "Error: TaskManager.cpp was not found beside build.sh." >&2
    exit 1
fi

if [[ ! -f "$ICON" ]]; then
    echo "Error: icon.ico was not found. Put your icon file named icon.ico in this project folder." >&2
    exit 1
fi

TEMP_DIR="$(mktemp -d)"
cleanup() {
    rm -rf "$TEMP_DIR"
}
trap cleanup EXIT

# Create the .rc file only temporarily. You do not need to write or maintain it.
ICON_WINDOWS_PATH="$(cygpath -m "$ICON")"
printf '101 ICON "%s"\n' "$ICON_WINDOWS_PATH" > "$TEMP_DIR/app_icon.rc"

windres -i "$TEMP_DIR/app_icon.rc" -o "$TEMP_DIR/app_icon.o" -O coff

echo "Building Persian Win32 Task Manager v1.3.0 with MSYS2 UCRT64..."
g++ -std=c++11 -O2 -Wall -finput-charset=UTF-8 -mwindows \
    "$SOURCE" "$TEMP_DIR/app_icon.o" \
    -lcomctl32 -ladvapi32 -lshell32 \
    -static-libgcc -static-libstdc++ -o "$OUTPUT"

echo "Build successful: $OUTPUT"
