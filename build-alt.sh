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
INC="-I src/engine -I src/game -I src/effects -I raylib/src -I glfw/include"
LD="-L raylib/src -L glfw/build/src -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm"
OBJ=build/desktop/obj

if [ "$1" = "clean" ]; then rm -rf build/desktop; echo "cleaned"; exit 0; fi

mkdir -p "$OBJ"
for f in src/main.cpp src/engine/*.cpp src/game/*.cpp src/effects/*.cpp; do
  n=$(basename "$f" .cpp)
  echo "  CC  $f"
  timeout 120 $GPP $SYS -std=c++11 $INC -c "$f" -o "$OBJ/$n.o"
done

echo "  LD  build/desktop/game.exe"
timeout 120 $GPP "$OBJ"/*.o -o build/desktop/game.exe $LD
echo "=== built build/desktop/game.exe ==="
