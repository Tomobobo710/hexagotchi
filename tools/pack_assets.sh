#!/bin/bash
# tools/pack_assets.sh - build the pack_assets tool and run it to produce
# assets.rres from assets/. Uses the MSYS2 UCRT64 MinGW toolchain, same as the
# Makefile / build-alt.sh / tools/screenshot.sh (see the Makefile's toolchain
# comment for why the devkitPro Cygwin g++ must never be used).
#
# Usage:
#   tools/pack_assets.sh              pack assets/ -> build/assets.rres
#   tools/pack_assets.sh <dir> <out>  pack <dir> -> <out>
set -e
cd "$(dirname "$0")/.."

# Windows: use the MSYS2 UCRT64 MinGW toolchain -- the SAME one the Makefile
# pins for the game build (see Makefile's toolchain block). Do NOT use the
# devkitPro Cygwin g++: mixing it with ucrt64 tools on PATH cross-contaminates
# the link (cygwin collect2 grabs ucrt64's ld and fails on the LTO plugin).
# pack_assets.cpp is pure rres + std file I/O -- it needs NO raylib/glfw.
if [ -f /c/msys64/ucrt64/bin/g++.exe ]; then
    export PATH="/c/msys64/ucrt64/bin:$PATH"
    GPP=/c/msys64/ucrt64/bin/g++.exe
    SYS=""
    LD="-static -static-libgcc -static-libstdc++"
else
    # Linux: use system g++ - no raylib/glfw needed for asset packing
    GPP=g++
    SYS=""
    LD=""
fi
INC="-I rres/src"

BIN=build/devtools/pack_assets.exe
OBJ=build/devtools/pack_assets.o
mkdir -p build/devtools

# No `timeout` wrapper here: timeout is a Cygwin coreutil, and a Cygwin parent
# rewrites TMP/TEMP when spawning the NATIVE MinGW g++, which then can't create
# its temp files ("Cannot create temporary file in C:\WINDOWS"). The wrapper
# was hang-protection for the old Cygwin toolchain; the native one doesn't
# hang. Compile+link kept as two steps to match the rest of the build.
$GPP $SYS -std=c++11 $INC -c tools/pack_assets.cpp -o "$OBJ"
$GPP "$OBJ" -o "$BIN" $LD

OUT="${2:-build/assets.rres}"
mkdir -p "$(dirname "$OUT")"
"$BIN" "${1:-assets}" "$OUT"
