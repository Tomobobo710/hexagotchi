#!/bin/bash
# tools/screenshot.sh - build a throwaway dev binary and capture a scene as a PNG.
#
# This does NOT touch build/desktop or any output Tom uses. It compiles into
# build/devtools/ with -DHEXA_SHOT_TOOL, which enables a small dev-only branch
# in src/main.cpp (stripped out of every normal build). The branch starts the
# game on the requested scene, renders a few frames, and calls raylib's
# TakeScreenshot() so the PNG comes straight from the real framebuffer.
#
# Usage:
#   tools/screenshot.sh <scene> [out.png]
#   tools/screenshot.sh input_test
#   tools/screenshot.sh boss build/devtools/boss.png
#
# Registered scene names (see src/main.cpp): game, boss, input_test
set -e
cd "$(dirname "$0")/.."

SCENE="${1:?usage: tools/screenshot.sh <scene> [out.png]}"
OUT="${2:-shot.png}"

GPP=/c/devkitPro/msys2/usr/bin/g++.exe
G=/c/devkitPro/msys2/usr/lib/gcc/x86_64-pc-cygwin/15.2.0
SYS="-isystem $G/include/c++ -isystem $G/include/c++/x86_64-pc-cygwin -isystem $G/include/c++/backward -isystem $G/include -isystem /c/devkitPro/msys2/usr/include"
INC="-I src/engine -I src/game -I src/effects -I raylib/src -I glfw/include"
LD="-L raylib/src -L glfw/build/src -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm"

DEVOUT=build/devtools
OBJ="$DEVOUT/obj"
mkdir -p "$OBJ"

for f in src/main.cpp src/engine/*.cpp src/game/*.cpp src/effects/*.cpp; do
    n=$(basename "$f" .cpp)
    timeout 120 $GPP -DHEXA_SHOT_TOOL $SYS -std=c++11 $INC -c "$f" -o "$OBJ/$n.o"
done

timeout 120 $GPP "$OBJ"/*.o -o "$DEVOUT/game_devtool.exe" $LD

rm -f "$DEVOUT/shot.png"
(cd "$DEVOUT" && HEXA_SHOT="$SCENE" timeout 30 ./game_devtool.exe) >/dev/null 2>&1 || true

if [ ! -f "$DEVOUT/shot.png" ]; then
    echo "no screenshot produced (scene '$SCENE' may not exist)" >&2
    exit 1
fi

mv "$DEVOUT/shot.png" "$OUT"
echo "$OUT"
