#!/bin/bash
# build-alt.sh - alternate desktop build for restricted shells.
#
# The normal build (build-desktop.bat -> make) is instant in a normal cmd.
# Some restricted shells launch the devkitPro g++ with (a) an empty include
# search list and (b) -pipe deadlocking, so plain make fails/hangs there. This
# script handles that case: it passes the devkitPro cygwin system include dirs
# explicitly, avoids -pipe, and compiles per file.
#
# Nothing here affects the normal build. Use build-desktop.bat on a real machine.
#
#   ./build-alt.sh          build build/desktop/game.exe
#   ./build-alt.sh clean    remove build/desktop
set -e
cd "$(dirname "$0")"

GPP=/c/devkitPro/msys2/usr/bin/g++.exe
G=/c/devkitPro/msys2/usr/lib/gcc/x86_64-pc-cygwin/15.2.0
SYS="-isystem $G/include/c++ -isystem $G/include/c++/x86_64-pc-cygwin -isystem $G/include/c++/backward -isystem $G/include -isystem /c/devkitPro/msys2/usr/include"
INC="-I src/engine -I src/game -I src/effects -I src/events -I src/flags -I raylib/src -I glfw/include -I rres/src"
LD="-L raylib/src -L glfw/build/src -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm"
OBJ=build/desktop/obj

if [ "$1" = "clean" ]; then rm -rf build/desktop; echo "cleaned"; exit 0; fi

# Repack build/assets.rres whenever anything under assets/ is newer than it,
# so asset changes are picked up automatically -- no manual pack step. Packed
# into build/ -- it's a generated artifact, never the repo root.
mkdir -p build
if [ -z "$(find assets -newer build/assets.rres 2>/dev/null)" ] && [ -f build/assets.rres ]; then
  echo "  assets.rres up to date"
else
  echo "  packing assets.rres"
  tools/pack_assets.sh assets build/assets.rres
fi

mkdir -p "$OBJ"
for f in src/main.cpp src/engine/*.cpp src/game/*.cpp src/effects/*.cpp src/events/*.cpp; do
  n=$(basename "$f" .cpp)
  echo "  CC  $f"
  timeout 120 $GPP $SYS -std=c++17 $INC -c "$f" -o "$OBJ/$n.o"
done

echo "  LD  build/desktop/game.exe"
timeout 120 $GPP "$OBJ"/*.o -o build/desktop/game.exe $LD

# game.exe reads assets.rres via a relative path, so it must sit next to the
# exe -- otherwise launching it from anywhere but the repo root can't find it.
cp build/assets.rres build/desktop/assets.rres

echo "=== built build/desktop/game.exe ==="
