#!/bin/bash
# tools/pack_assets.sh - build the pack_assets tool and run it to produce
# assets.rres from assets/. Uses the same devkitPro toolchain workaround as
# tools/screenshot.sh (see that file's comments for why).
#
# Usage:
#   tools/pack_assets.sh              pack assets/ -> build/assets.rres
#   tools/pack_assets.sh <dir> <out>  pack <dir> -> <out>
set -e
cd "$(dirname "$0")/.."

# Try devkitPro first (Windows paths), fall back to system g++ (Linux)
if [ -f /c/devkitPro/msys2/usr/bin/g++.exe ]; then
    GPP=/c/devkitPro/msys2/usr/bin/g++.exe
    G=/c/devkitPro/msys2/usr/lib/gcc/x86_64-pc-cygwin/15.2.0
    SYS="-isystem $G/include/c++ -isystem $G/include/c++/x86_64-pc-cygwin -isystem $G/include/c++/backward -isystem $G/include -isystem /c/devkitPro/msys2/usr/include"
    LD="-L raylib/src -L glfw/build/src -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm"
else
    # Linux: use system g++
    GPP=g++
    SYS=""
    LD="-L raylib/src -L glfw/build/src -lraylib -lglfw3 -lGL -ldl -lm"
fi
INC="-I raylib/src -I rres/src"

BIN=build/devtools/pack_assets.exe
OBJ=build/devtools/pack_assets.o
mkdir -p build/devtools

# Compile then link as separate steps -- one-shot `g++ file.cpp -o exe` on
# this Cygwin toolchain mysteriously expects a WinMain entry point and fails
# to link even a bare `int main(){}`; compiling to an object first and linking
# separately avoids it (matches how tools/screenshot.sh and the Makefile
# itself always build, which is why this project hasn't hit it before).
timeout 120 $GPP $SYS -std=c++11 $INC -c tools/pack_assets.cpp -o "$OBJ"
timeout 120 $GPP "$OBJ" -o "$BIN" $LD

OUT="${2:-build/assets.rres}"
mkdir -p "$(dirname "$OUT")"
"$BIN" "${1:-assets}" "$OUT"
