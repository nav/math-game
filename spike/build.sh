#!/usr/bin/env bash
#
# build.sh - compile test_sdl.c using pkg-config for SDL2 flags.
#
# Same command works both:
#   - locally on macOS inside `nix-shell` (shell.nix at repo root), and
#   - on the Raspberry Pi with apt-installed `libsdl2-dev`
#
# Usage:
#   ./build.sh          # compile
#   ./build.sh run       # compile and run

set -euo pipefail

cd "$(dirname "$0")"

CC="${CC:-cc}"

$CC -Wall -Wextra -std=c99 test_sdl.c -o test_sdl $(pkg-config --cflags --libs sdl2)

echo "Built ./test_sdl"

if [ "${1:-}" = "run" ]; then
    ./test_sdl
fi
