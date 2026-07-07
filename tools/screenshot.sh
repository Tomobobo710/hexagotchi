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
#   tools/screenshot.sh <scene> [out.png] [wait_frames]
#   tools/screenshot.sh input_test
#   tools/screenshot.sh boss build/devtools/boss.png
#   tools/screenshot.sh input_test build/devtools/anim2.png 45   # let more frames pass first
#
# Registered scene names (see src/main.cpp): game, boss, input_test
set -e
cd "$(dirname "$0")/.."

SCENE="${1:?usage: tools/screenshot.sh <scene> [out.png] [wait_frames]}"
OUT="${2:-build/devtools/shot.png}"
WAIT_FRAMES="${3:-30}"
# Resolve to an absolute path up front -- the capture step below cd's into
# build/devtools/ to run the exe, so a relative OUT would otherwise resolve
# against the wrong directory.
mkdir -p "$(dirname "$OUT")"
OUT="$(cd "$(dirname "$OUT")" && pwd)/$(basename "$OUT")"

GPP=/c/devkitPro/msys2/usr/bin/g++.exe
G=/c/devkitPro/msys2/usr/lib/gcc/x86_64-pc-cygwin/15.2.0
SYS="-isystem $G/include/c++ -isystem $G/include/c++/x86_64-pc-cygwin -isystem $G/include/c++/backward -isystem $G/include -isystem /c/devkitPro/msys2/usr/include"
INC="-I src/engine -I src/game -I src/effects -I raylib/src -I glfw/include -I rres/src"
LD="-L raylib/src -L glfw/build/src -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm"

DEVOUT=build/devtools
OBJ="$DEVOUT/obj"
mkdir -p "$OBJ"

# Repack build/assets.rres whenever anything under assets/ is newer than it.
# Packed into build/ -- a generated artifact, never the repo root.
mkdir -p build
if [ -z "$(find assets -newer build/assets.rres 2>/dev/null)" ] && [ -f build/assets.rres ]; then
    :
else
    tools/pack_assets.sh assets build/assets.rres >/dev/null
fi

for f in src/main.cpp src/engine/*.cpp src/game/*.cpp src/effects/*.cpp; do
    n=$(basename "$f" .cpp)
    timeout 120 $GPP -DHEXA_SHOT_TOOL $SYS -std=c++11 $INC -c "$f" -o "$OBJ/$n.o"
done

timeout 120 $GPP "$OBJ"/*.o -o "$DEVOUT/game_devtool.exe" $LD

# AssetPack looks for "assets.rres" next to the running exe's cwd, same as
# the real game.exe/build/desktop pairing -- copy it in rather than running
# from repo root (nothing here reads loose assets/ paths anymore).
cp build/assets.rres "$DEVOUT/assets.rres"

# The exe always writes "shot.png" into its cwd (hardcoded in main.cpp's
# TakeScreenshot() call) -- run from $DEVOUT so that lands in build/, and
# clean it up on every exit path (success, failure, or interruption) so nothing
# is ever left sitting around beyond the requested $OUT.
(
    cd "$DEVOUT"
    rm -f shot.png
    # $OUT may already point at this exact file (the default output path is
    # inside $DEVOUT) -- only clean it up on exit when it's a *different*
    # location, otherwise the trap would delete the very file we're keeping.
    if [ "$(pwd)/shot.png" != "$OUT" ]; then
        trap 'rm -f shot.png' EXIT
    fi
    HEXA_SHOT="$SCENE" HEXA_SHOT_FRAMES="$WAIT_FRAMES" timeout 30 ./game_devtool.exe >/dev/null 2>&1 || true
    if [ ! -f shot.png ]; then
        echo "no screenshot produced (scene '$SCENE' may not exist)" >&2
        exit 1
    fi
    if [ "$(pwd)/shot.png" != "$OUT" ]; then
        cp shot.png "$OUT"
    fi
)
echo "$OUT"
