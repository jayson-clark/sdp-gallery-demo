#!/usr/bin/env bash
# build.sh — Compile a student FEH project to WebAssembly.
#
# Usage:
#   ./build.sh <path-to-student-project> [output-name]
#   DEBUG=1 ./build.sh <path-to-student-project> [output-name]
#
# Example:
#   ./build.sh ../cpp_template hello_world
#   → outputs to web/dist/hello_world/
#
# DEBUG=1 turns on assertions, C++ exception catching, demangled stack traces,
# and source-map symbols. The wasm is bigger and slower, but a crash gives you
# a real backtrace instead of "RuntimeError: unreachable".
#
# Requirements:
#   - Emscripten SDK active in shell (emcc on PATH).
#     Install: https://emscripten.org/docs/getting_started/downloads.html
#     Then run: source <emsdk>/emsdk_env.sh

set -euo pipefail

WEB_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

if [ $# -lt 1 ]; then
    echo "Usage: $0 <student-project-dir> [game-name]"
    echo "       <student-project-dir> must contain main.cpp and a simulator_libraries/ folder"
    exit 1
fi

SRC_DIR="$(cd "$1" && pwd)"
GAME="${2:-$(basename "$SRC_DIR")}"

# Sanity checks
if [ ! -d "$SRC_DIR/simulator_libraries" ]; then
    echo "Error: $SRC_DIR has no simulator_libraries/ subfolder."
    echo "       Make sure to run 'make update' first to pull the FEH libraries."
    exit 1
fi
if ! command -v emcc >/dev/null 2>&1; then
    echo "Error: emcc not found on PATH."
    echo "       Install emscripten: https://emscripten.org/docs/getting_started/downloads.html"
    echo "       Then: source <emsdk>/emsdk_env.sh"
    exit 1
fi

echo "[web] Source : $SRC_DIR"
echo "[web] Game   : $GAME"
echo "[web] Output : $WEB_DIR/dist/$GAME/"
echo

make -C "$WEB_DIR" -f "$WEB_DIR/Makefile" \
    SRC_DIR="$SRC_DIR" \
    GAME="$GAME" \
    OUT_DIR="$WEB_DIR/dist/$GAME" \
    DEBUG="${DEBUG:-0}"

echo
echo "[web] To preview locally:"
echo "      cd '$WEB_DIR/dist/$GAME' && python3 -m http.server 8000"
echo "      then open http://localhost:8000"
