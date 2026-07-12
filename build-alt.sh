#!/bin/bash
# build-alt.sh - alternate desktop build for restricted shells.
#
# The normal build (build-desktop.bat -> make) is the canonical path. This
# script exists for restricted shells where invoking make is flaky; it compiles
# per file with explicit paths but uses the SAME toolchain, flags, and link
# line as the Makefile, so both paths produce identical exes.
#
# TOOLCHAIN: MSYS2 UCRT64 MinGW (x86_64-w64-mingw32) -- NOT the devkitPro
# Cygwin g++. Cygwin g++ doesn't define _WIN32 and has no windows.h, so
# raylib's miniaudio compiles its POSIX branch and falls back to the Null
# audio backend at runtime: everything loads, IsAudioDeviceReady()==1, and the
# game is completely silent. That was the desktop no-audio bug. The Makefile
# pins the same toolchain for the same reason.
#
#   ./build-alt.sh          build build/desktop/game.exe
#   ./build-alt.sh debug    same, plus console window + hexa_log.txt logging
#   ./build-alt.sh clean    remove build/desktop
set -e
cd "$(dirname "$0")"

TOOLDIR=/c/msys64/ucrt64/bin
export PATH="$TOOLDIR:$PATH"   # MSYS2 compilers need their bin dir on PATH for their own DLLs
GPP=$TOOLDIR/g++.exe
INC="-I src/engine -I src/game -I src/effects -I src/events -I src/flags -I raylib/src -I rres/src"
# Must match the Makefile's LDFLAGS exactly (see Makefile comments):
#   - GLFW is inside libraylib.a (rglfw.c), no separate glfw link
#   - ole32/oleaut32/uuid/ksuser for miniaudio's WASAPI backend
#   - -static* so the exe runs without MSYS2 runtime DLLs
LD="-static -static-libgcc -static-libstdc++ -L raylib/src -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -loleaut32 -limm32 -lversion -luuid -lksuser"
OBJ=build/desktop/obj

if [ "$1" = "clean" ]; then rm -rf build/desktop; echo "cleaned"; exit 0; fi

# Debug mode: `build-alt.sh debug` keeps a console window (no -mwindows) and
# defines HEXA_DEBUG_LOG so all logging stays on and raylib TraceLog is mirrored
# to hexa_log.txt next to the exe. Used to diagnose the no-audio-on-desktop bug.
DBG_DEF=""
WIN_FLAG="-mwindows"
if [ "$1" = "debug" ]; then
  DBG_DEF="-DHEXA_DEBUG_LOG"
  WIN_FLAG=""
  echo "  (debug build: console + hexa_log.txt enabled)"
fi

# Desktop raylib lib: built by the Makefile's own rule so there is exactly ONE
# definition of how raylib gets compiled (toolchain, flags, rglfw). Only runs
# when the lib is missing -- make's rule is a no-op otherwise.
if [ ! -f raylib/src/libraylib.a ]; then
  echo "  building raylib (desktop, MinGW)"
  make raylib/src/libraylib.a
fi

# Always repack assets so a stale/mistimed pack can never ship (matches
# build-web.sh). Packed into build/ -- it's a generated artifact, never the
# repo root.
mkdir -p build
echo "  packing assets.rres"
tools/pack_assets.sh assets build/assets.rres

mkdir -p "$OBJ"
for f in src/main.cpp src/engine/*.cpp src/game/*.cpp src/effects/*.cpp src/events/*.cpp; do
  n=$(basename "$f" .cpp)
  echo "  CC  $f"
  timeout 120 $GPP -std=c++17 $DBG_DEF $INC -c "$f" -o "$OBJ/$n.o"
done

echo "  LD  build/desktop/game.exe"
# -mwindows: build as a Windows GUI app so no console window pops up alongside
# the game when the .exe is run (omitted for the debug build).
timeout 120 $GPP "$OBJ"/*.o -o build/desktop/game.exe $WIN_FLAG $LD

# game.exe reads assets.rres via a relative path, so it must sit next to the
# exe -- otherwise launching it from anywhere but the repo root can't find it.
cp build/assets.rres build/desktop/assets.rres

echo "=== built build/desktop/game.exe ==="
