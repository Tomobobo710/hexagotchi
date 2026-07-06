# hexagotchi Build Instructions

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
git submodule update --init --recursive
```

### 3. Build dependencies (one-time)

```bash
make deps
```

### 4. Install Emscripten toolchain (one-time, needed for web build)

```bash
python emsdk/emsdk.py install latest
python emsdk/emsdk.py activate latest
```

### 5. Build and run

**Desktop:**
```bash
make
./build/desktop/game.exe
```

**Web:**
```bash
make web
python -m http.server 8080
# open http://localhost:8080/build/web/game.html
```

### Cleanup

```bash
make clean          # wipe desktop build
make clean-web      # wipe web build
```
