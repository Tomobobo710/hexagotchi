#!/bin/bash
# build-scene-editor.sh - the standalone scene layout editor, built directly in
# bash instead of `make scene-editor` (which fails on Windows: the native MinGW
# compiler run through make's msys-bash recipes dies with "Cannot create
# temporary file in C:\WINDOWS"). Mirrors the Makefile's scene-editor rule.
#
#   ./build-scene-editor.sh          build build/scene_editor/scene_editor.exe
#   ./build-scene-editor.sh clean    remove build/scene_editor
set -e
cd "$(dirname "$0")"

TOOLDIR=/c/msys64/ucrt64/bin
export PATH="$TOOLDIR:$PATH"
GPP=$TOOLDIR/g++.exe

OUT=build/scene_editor
BIN=$OUT/scene_editor.exe
# Same link line as the game (build-alt.sh): GLFW is inside libraylib.a,
# ole32/... for WASAPI, -static* so the tool is self-contained.
LD="-static -static-libgcc -static-libstdc++ -L raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -loleaut32 -limm32 -lversion -luuid -lksuser"

if [ "$1" = "clean" ]; then rm -rf "$OUT"; echo "cleaned scene editor"; exit 0; fi

# Desktop raylib lib (built by the Makefile's rule if missing) -- but do it in
# bash to dodge the make bug. build-alt.sh has the same guard.
if [ ! -f raylib/src/libraylib.a ]; then
  echo "  building raylib (desktop)"; bash build-alt.sh >/dev/null 2>&1 || true
fi

# Ensure the pack + manifest exist to copy next to the editor.
if [ ! -f build/assets.rres ]; then
  tools/pack_assets.sh assets build/assets.rres
fi

mkdir -p "$OUT"
echo "  CC/LD $BIN"
$GPP -std=c++11 -I raylib/src -I rres/src tools/scene_editor.cpp -o "$BIN" $LD

# The editor loads assets relative to its own dir, so give it its own copies.
cp build/assets.rres "$OUT/assets.rres"
cp build/assets_manifest.txt "$OUT/assets_manifest.txt"

echo "=== built $BIN ==="
