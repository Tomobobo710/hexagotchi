# rEngine Build Instructions

## Prerequisites

- **g++** (MinGW/GCC)
- **make**
- **cmake**

## Desktop Build

### 1. Clone

```bash
git clone --recurse-submodules https://github.com/Tomobobo710/rEngine.git
cd rEngine
```

### 2. Build dependencies (one-time)

```bash
make deps
```

### 3. Build and run

```bash
make
./build/desktop/game.exe
```

`make clean` to wipe the desktop build.

## Web Build

Emsdk is included as a submodule but needs to download the compiler toolchain once:

```bash
make emsdk-setup
```

Then build:

```bash
make web
```

Outputs to `build/web/`. Serve it:

```bash
python -m http.server 8080
# open http://localhost:8080/build/web/game.html
```

`make clean-web` to wipe the web build.
