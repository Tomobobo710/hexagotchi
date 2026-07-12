# hexagotchi Build Instructions

## Linux

### 1. Prerequisites

- CMake (3.16 or higher)
- C++ compiler with C++14 support (GCC 5+, Clang 3.4+)
- OpenGL libraries (libGL, m, pthread, dl, rt)

On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake libgl1-mesa-dev libglu1-mesa-dev libglfw3-dev
```

### 2. Clone with submodules

```bash
git clone --recurse-submodules https://github.com/Tomobobo710/hexagotchi.git
cd hexagotchi
```

If you already cloned without submodules:

```bash
git submodule init
git config --file .git/config --add submodule.raylib.url https://github.com/raysan5/raylib.git
git config --file .git/config --add submodule.raylib.active true
git config --file .git/config --add submodule.glfw.url https://github.com/glfw/glfw.git
git config --file .git/config --add submodule.glfw.active true
git config --file .git/config --add submodule.emsdk.url https://github.com/emscripten-core/emsdk.git
git config --file .git/config --add submodule.emsdk.active true
git config --file .git/config --add submodule.rres.url https://github.com/raysan5/rres.git
git config --file .git/config --add submodule.rres.active true
git submodule update --init --recursive
```

### 3. Build dependencies (one-time)

```bash
make deps
```

### 4. Install Emscripten toolchain (one-time, needed for web build)

If the `emsdk` submodule is not already initialized:

```bash
git submodule update --init --recursive
```

Then install and activate Emscripten:

```bash
python emsdk/emsdk.py install latest
python emsdk/emsdk.py activate latest
```

### 5. Build and run

**Desktop (CMake):**
```bash
cmake -B build -D RBUILD_DESKTOP=ON -D RBUILD_WEB=OFF
cmake --build build
./build/desktop/Debug/game
# or ./build/desktop/Release/game depending on build type
```

**Web (CMake):**
```bash
# The emsdk submodule is preferred; if not present, set EMSDK env var:
# export EMSDK=$HOME/emsdk
# Then run:
cmake -B build -D RBUILD_DESKTOP=OFF -D RBUILD_WEB=ON
cmake --build build --target game-web
# Output: build/web/game.html
```

### Cleanup

```bash
rm -rf build/desktop      # wipe desktop build
rm -rf build/web          # wipe web build
```

## What worked for Tom on Windows

Follow these steps exactly. Tested on Windows with MSYS2 (UCRT64 environment).

### 1. Prerequisites

- **MSYS2** (install from https://www.msys2.org/) — use the **UCRT64** environment
- Inside MSYS2 UCRT64, install:
  ```bash
  pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-cmake
  ```

### 2. Clone with submodules

```bash
git clone --recurse-submodules https://github.com/Tomobobo710/hexagotchi.git
cd hexagotchi
```

If you already cloned without submodules:

```bash
git submodule init
git config --file .git/config --add submodule.raylib.url https://github.com/raysan5/raylib.git
git config --file .git/config --add submodule.raylib.active true
git config --file .git/config --add submodule.glfw.url https://github.com/glfw/glfw.git
git config --file .git/config --add submodule.glfw.active true
git config --file .git/config --add submodule.emsdk.url https://github.com/emscripten-core/emsdk.git
git config --file .git/config --add submodule.emsdk.active true
git config --file .git/config --add submodule.rres.url https://github.com/raysan5/rres.git
git config --file .git/config --add submodule.rres.active true
git submodule update --init --recursive
```

### 3. Build dependencies (one-time)

```bash
make deps
```

### 4. Install Emscripten toolchain (one-time, needed for web build)

If the `emsdk` submodule is not already initialized:

```bash
git submodule update --init --recursive
```

Then install and activate Emscripten:

```bash
python emsdk/emsdk.py install latest
python emsdk/emsdk.py activate latest
```

### 5. Build and run

**Desktop (using Makefile):**
```bash
make
./build/desktop/game.exe
```

**Web (using Makefile):**
```bash
make web
python -m http.server 8080
# open http://localhost:8080/build/web/ (serves index.html)
```

**Desktop (using CMake):**
```bash
cmake -B build -D RBUILD_DESKTOP=ON -D RBUILD_WEB=OFF
cmake --build build
./build/desktop/Debug/game.exe
```

**Web (using CMake):**
```bash
# The emsdk submodule is preferred; if not present, set EMSDK env var before running cmake:
# set EMSDK=%HOME%\emsdk (Windows)
# export EMSDK=$HOME/emsdk (Linux/macOS)
cmake -B build -D RBUILD_DESKTOP=OFF -D RBUILD_WEB=ON
cmake --build build --target game-web
# Output: build/web/game.html
```

### Cleanup

```bash
make clean          # wipe desktop build
make clean-web      # wipe web build
rm -rf build        # wipe all CMake builds
```

## Linux Release Build

For a production-ready release on Linux, follow these steps to build with optimizations and prepare a distributable folder:

### 1. Configure and build (Release configuration)

```bash
cmake -B build -D RBUILD_DESKTOP=ON -D RBUILD_WEB=OFF -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### 2. Create release directory and copy files

```bash
mkdir -p release
cp build/desktop/game release/
cp build/desktop/assets.rres release/
cp build/desktop/assets_manifest.txt release/
```

### 3. Strip debug symbols from the binary

```bash
strip release/game
```

### 4. Verify the stripped binary

```bash
file release/game
# Should show: ELF 64-bit LSB executable, x86-64, version 1 (GNU/Linux), dynamically linked, stripped
```

### One-liner for the entire release process:

```bash
cmake -B build -D RBUILD_DESKTOP=ON -D RBUILD_WEB=OFF -DCMAKE_BUILD_TYPE=Release && \
cmake --build build && \
mkdir -p release && \
cp build/desktop/{game,assets.rres,assets_manifest.txt} release/ && \
strip release/game
```

### Notes:

- **Static linking**: The build already uses `-static-libgcc -static-libstdc++` to bundle the C++ runtime. Full static linking isn't possible because `libGL` is only available as a shared library on Linux.

- **Dependencies**: The binary is dynamically linked to `libGL.so`, system libraries (`m`, `pthread`, `dl`, `rt`), and bundled GLFW via `libraylib.a`.

- **Assets**: The `assets.rres` and `assets_manifest.txt` must be in the same directory as the executable since the game loads them via relative path.

- **For portability**: To create a fully self-contained release, you can bundle the required shared libraries with `ldd release/game | grep "=> /lib" | awk '{print $3}' | xargs -I {} cp {} release/lib/` (add this only if targeting machines without those libraries installed).
