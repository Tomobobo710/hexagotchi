# hexagotchi Build Instructions

## Prerequisites

- **g++** (MinGW/GCC)
- **make**
- **cmake**

## Setup

### 1. Clone with submodules

```bash
git clone --recurse-submodules https://github.com/Tomobobo710/hexagotchi.git
cd hexagotchi
```

If you already cloned without submodules:

```bash
git submodule update --init --recursive
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
