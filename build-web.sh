#!/bin/bash
# build-web.sh - web (wasm) build, done directly in bash instead of `make web`.
#
# Why not `make`: this project's `make` runs recipes through msys-bash, and the
# native asset-packer compile inside `force-assets` fails there ("Cannot create
# temporary file in C:\WINDOWS"). Building directly sidesteps that. Mirrors the
# Makefile's `web:` recipe exactly (same flags/output), so the two stay
# equivalent -- keep them in sync if either changes.
#
#   ./build-web.sh          build build/web/index.html
#   ./build-web.sh clean    remove build/web + the web raylib lib
set -e
cd "$(dirname "$0")"

RL=raylib/src
WEB_OUT=build/web

# Plain MSYS2 bash (as launched by build-web.bat) has a minimal PATH that
# doesn't include python, even when it's installed for Windows -- search a
# few known install locations before giving up.
PYTHON=""
for candidate in python python3 \
    "/c/Users/$USER/AppData/Local/Programs/Python/Python310/python" \
    "/c/Users/$USER/AppData/Local/Programs/Python/Python311/python" \
    "/c/Users/$USER/AppData/Local/Programs/Python/Python38/python"; do
  if command -v "$candidate" >/dev/null 2>&1; then
    PYTHON="$candidate"
    break
  fi
done
if [ -z "$PYTHON" ]; then
  echo "build-web.sh: no python interpreter found (checked PATH and common install dirs)" >&2
  exit 1
fi

EMCC="$PYTHON emsdk/upstream/emscripten/emcc.py"
EMAR="$PYTHON emsdk/upstream/emscripten/emar.py"

# emcc shells out to other tools (e.g. file_packager.py) using its own
# interpreter resolution, which ignores our $PATH searching above -- point it
# at the same interpreter via the env var emsdk actually honors.
export EMSDK_PYTHON="$(cygpath -w "$PYTHON" 2>/dev/null || echo "$PYTHON")"

if [ "$1" = "clean" ]; then
  rm -rf "$WEB_OUT" "$RL"/*.wasm.o "$RL/libraylib.web.a"
  echo "cleaned web"
  exit 0
fi

WEBINCLUDES="-I src/engine -I src/game -I src/effects -I src/events -I src/flags -I $RL -I rres/src"
WEBFLAGS="-Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -I $RL"
WEBLINK="-s USE_GLFW=3 -s ASYNCIFY -s WASM=1 -s TOTAL_MEMORY=67108864 -s GL_ENABLE_GET_PROC_ADDRESS -DPLATFORM_WEB -Os -std=c++17"

SRCS="src/main.cpp $(ls src/engine/*.cpp src/game/*.cpp src/effects/*.cpp src/events/*.cpp)"

# Web raylib static lib (emscripten). Built once; matches the Makefile's
# libraylib.web.a rule.
if [ ! -f "$RL/libraylib.web.a" ]; then
  echo "  building raylib (web)"
  $EMCC -c "$RL/rcore.c"     $WEBFLAGS -o "$RL/rcore.wasm.o"
  $EMCC -c "$RL/rshapes.c"   $WEBFLAGS -o "$RL/rshapes.wasm.o"
  $EMCC -c "$RL/rtextures.c" $WEBFLAGS -o "$RL/rtextures.wasm.o"
  $EMCC -c "$RL/rtext.c"     $WEBFLAGS -o "$RL/rtext.wasm.o"
  $EMCC -c "$RL/rmodels.c"   $WEBFLAGS -o "$RL/rmodels.wasm.o"
  $EMCC -c "$RL/raudio.c"    -Os -DPLATFORM_WEB -I "$RL" -o "$RL/raudio.wasm.o"
  $EMAR rcs "$RL/libraylib.web.a" "$RL/rcore.wasm.o" "$RL/rshapes.wasm.o" "$RL/rtextures.wasm.o" "$RL/rtext.wasm.o" "$RL/rmodels.wasm.o" "$RL/raudio.wasm.o"
fi

# Always repack assets so a stale/mistimed pack can never ship (matches the
# Makefile's force-assets dependency).
echo "  packing assets.rres"
tools/pack_assets.sh assets build/assets.rres

mkdir -p "$WEB_OUT"
echo "  LD  $WEB_OUT/index.html"
$EMCC $SRCS "$RL/libraylib.web.a" -o "$WEB_OUT/index.html" $WEBINCLUDES $WEBLINK \
  --shell-file web/shell.html --preload-file build/assets.rres@assets.rres

echo "=== built $WEB_OUT/index.html ==="
