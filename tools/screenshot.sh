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

# MSYS2 UCRT64 MinGW -- same pinned toolchain as the Makefile/build-alt.sh
# (NOT devkitPro's Cygwin g++; see the Makefile's toolchain comment). Must
# match, since this links the same MinGW-built raylib/src/libraylib.a.
TOOLDIR=/c/msys64/ucrt64/bin
export PATH="$TOOLDIR:$PATH"
GPP=$TOOLDIR/g++.exe
SYS=""
INC="-I src/engine -I src/game -I src/effects -I src/events -I src/flags -I raylib/src -I rres/src"
# Mirrors the Makefile's LDFLAGS: GLFW lives inside libraylib.a (rglfw),
# ole32/oleaut32/uuid/ksuser for the WASAPI audio backend, -static* so the
# tool exe is self-contained.
LD="-static -static-libgcc -static-libstdc++ -L raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -loleaut32 -limm32 -lversion -luuid -lksuser"

# Desktop raylib lib: built by the Makefile's own rule if missing, so there is
# exactly ONE definition of how raylib gets compiled.
if [ ! -f raylib/src/libraylib.a ]; then
    make raylib/src/libraylib.a >/dev/null
fi

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

# No `timeout` around the compiler: Cygwin's timeout rewrites TMP/TEMP when
# spawning the native MinGW g++, which then dies with "Cannot create temporary
# file in C:\WINDOWS" (same fix as tools/pack_assets.sh).
for f in src/main.cpp src/engine/*.cpp src/game/*.cpp src/effects/*.cpp src/events/*.cpp; do
    n=$(basename "$f" .cpp)
    $GPP -DHEXA_SHOT_TOOL $SYS -std=c++17 $INC -c "$f" -o "$OBJ/$n.o"
done

$GPP "$OBJ"/*.o -o "$DEVOUT/game_devtool.exe" $LD

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
