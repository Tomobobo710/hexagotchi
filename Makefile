# ---- Desktop toolchain ------------------------------------------------------
# The desktop build MUST use a real Windows (MinGW) compiler -- one that
# defines _WIN32 and ships windows.h. A Cygwin g++ (e.g. devkitPro's, which is
# what a bare `g++` resolves to on some machines) compiles raylib's miniaudio
# down its POSIX branch: it looks for ALSA/PulseAudio at runtime, finds none on
# Windows, and silently falls back to the Null backend -- audio loads, device
# reports "ready", but the game is mute. Baked in at raylib compile time; no
# link flag can fix it. So: prefer MSYS2 UCRT64 when installed, else fall back
# to PATH g++ (which must then be a MinGW one).
UCRT64_GXX := $(wildcard /c/msys64/ucrt64/bin/g++.exe)
ifneq ($(UCRT64_GXX),)
TOOLDIR := /c/msys64/ucrt64/bin/
export PATH := /c/msys64/ucrt64/bin:$(PATH)
endif
# msys-based makes (e.g. devkitPro's) scrub TMP/TEMP from recipe environments.
# Native MinGW gcc then falls back to C:\WINDOWS for its temp files and dies
# with "Cannot create temporary file: Permission denied". Restore a writable
# Windows-style temp dir whenever the environment lost it.
ifeq ($(strip $(TMP)),)
export TMP  := $(shell cygpath -w /tmp 2>/dev/null)
export TEMP := $(TMP)
endif
CXX  = $(TOOLDIR)g++
CC   = $(TOOLDIR)gcc
AR   = $(TOOLDIR)ar
EMCC = python emsdk/upstream/emscripten/emcc.py
EMAR = python emsdk/upstream/emscripten/emar.py
RL   = raylib/src

SRCS        = src/main.cpp $(wildcard src/engine/*.cpp) $(wildcard src/game/*.cpp) $(wildcard src/effects/*.cpp) $(wildcard src/events/*.cpp)
INCLUDES    = -I src/engine -I src/game -I src/effects -I src/events -I src/flags -I $(RL) -I glfw/include -I rres/src
CXXFLAGS    = -std=c++17 $(INCLUDES)
# GLFW is compiled INTO libraylib.a via rglfw.c (see the libraylib.a rule), so
# no separate -lglfw3 / glfw cmake build is needed for desktop anymore.
# ole32/oleaut32/uuid/ksuser: WASAPI (miniaudio) COM dependencies.
# -static*: bundle libstdc++/libgcc/winpthread so game.exe runs on machines
# without the MSYS2 runtime DLLs (teammates, jam judges).
LDFLAGS     = -static -static-libgcc -static-libstdc++ -L $(RL) -lraylib -lopengl32 -lgdi32 -lwinmm -lole32 -loleaut32 -limm32 -lversion -luuid -lksuser

WEBINCLUDES = -I src/engine -I src/game -I src/effects -I src/events -I src/flags -I $(RL) -I rres/src
WEBFLAGS    = -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -I $(RL)
WEBLINK     = -s USE_GLFW=3 -s ASYNCIFY -s WASM=1 -s TOTAL_MEMORY=67108864 -s GL_ENABLE_GET_PROC_ADDRESS -DPLATFORM_WEB -Os -std=c++17

DESKTOP_OUT = build/desktop
WEB_OUT     = build/web
OBJ         = $(SRCS:src/%.cpp=$(DESKTOP_OUT)/obj/%.o)

# Desktop
all: $(DESKTOP_OUT)/game.exe $(DESKTOP_OUT)/assets.rres

# Link the compiled object files into the executable.
$(DESKTOP_OUT)/game.exe: $(OBJ) $(RL)/libraylib.a
	$(CXX) $(OBJ) -o $@ -mwindows $(LDFLAGS)

# Desktop raylib static lib, built from the vendored sources with the SAME
# MinGW toolchain as the game (see the toolchain block up top -- a Cygwin-built
# raylib means Null audio backend / silent game). rglfw.c compiles GLFW in, so
# desktop needs no separate glfw build. Mirrors the libraylib.web.a rule below.
RLFLAGS = -Os -DPLATFORM_DESKTOP -DGRAPHICS_API_OPENGL_33 -I $(RL) -I $(RL)/external -I $(RL)/external/glfw/include
$(RL)/libraylib.a:
	$(CC) -c $(RL)/rcore.c     $(RLFLAGS) -o $(RL)/rcore.desk.o
	$(CC) -c $(RL)/rshapes.c   $(RLFLAGS) -o $(RL)/rshapes.desk.o
	$(CC) -c $(RL)/rtextures.c $(RLFLAGS) -o $(RL)/rtextures.desk.o
	$(CC) -c $(RL)/rtext.c     $(RLFLAGS) -o $(RL)/rtext.desk.o
	$(CC) -c $(RL)/rmodels.c   $(RLFLAGS) -o $(RL)/rmodels.desk.o
	$(CC) -c $(RL)/raudio.c    $(RLFLAGS) -o $(RL)/raudio.desk.o
	$(CC) -c $(RL)/rglfw.c     $(RLFLAGS) -o $(RL)/rglfw.desk.o
	$(AR) rcs $@ $(RL)/rcore.desk.o $(RL)/rshapes.desk.o $(RL)/rtextures.desk.o $(RL)/rtext.desk.o $(RL)/rmodels.desk.o $(RL)/raudio.desk.o $(RL)/rglfw.desk.o

# game.exe reads assets.rres via a relative path, so it must sit next to the
# exe. Depends on force-assets (phony) so the pack is ALWAYS rebuilt first,
# matching the web: rule -- an mtime-based dependency on build/assets.rres would
# let a stale pack ship (e.g. after a git checkout, or when assets/ isn't newer
# than an existing pack), which is how the desktop build silently shipped a pack
# with no audio in it. force-assets repacks unconditionally, then we copy.
.PHONY: $(DESKTOP_OUT)/assets.rres
$(DESKTOP_OUT)/assets.rres: force-assets
	mkdir -p $(DESKTOP_OUT)
	cp build/assets.rres $(DESKTOP_OUT)/assets.rres

# Compile each source to an object file. -MMD -MP makes g++ emit a .d file
# next to each .o listing every header that source #includes, so editing a
# header rebuilds exactly the sources that use it (pulled in by -include below).
$(DESKTOP_OUT)/obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) -MMD -MP -c $< -o $@ $(CXXFLAGS)

# Pull in the auto-generated header-dependency lists (ignored on first build).
-include $(OBJ:.o=.d)

# Web
# --preload-file embeds build/assets.rres into the virtual filesystem at
# /assets.rres (see main.cpp's AssetPack::setPackFile("/assets.rres") under
# PLATFORM_WEB) -- without it the web build has no asset pack at all and
# every AssetPack::loadTexture() call silently misses.
# --shell-file uses our own web/shell.html (centered, 100vmin-scaled canvas
# with pixelated image-rendering), NOT raylib's vendored minshell.html --
# minshell.html has no centering/scaling CSS at all, which is why the canvas
# renders tiny/offset if that ever gets used here by mistake.
# NOTE: depends on force-assets (phony), NOT build/assets.rres directly. make
# decides build/assets.rres is "up to date" purely by mtime, but a git
# merge/checkout can land renamed art (e.g. idle_two -> blink) with an mtime
# OLDER than an existing build/assets.rres -- make then skips the repack and the
# web bundle preloads a pack whose CRC32 keys no longer match what the code asks
# for, so every gotchi sprite silently misses. force-assets always repacks, so a
# lying timestamp can never ship a stale pack. (Desktop's CMake path already
# force-repacks unconditionally, which is why only web hit this.)
web: force-assets $(RL)/libraylib.web.a
	mkdir -p $(WEB_OUT)
	$(EMCC) $(SRCS) $(RL)/libraylib.web.a -o $(WEB_OUT)/index.html $(WEBINCLUDES) $(WEBLINK) --shell-file web/shell.html --preload-file build/assets.rres@assets.rres

$(RL)/libraylib.web.a:
	$(EMCC) -c $(RL)/rcore.c     $(WEBFLAGS) -o $(RL)/rcore.wasm.o
	$(EMCC) -c $(RL)/rshapes.c   $(WEBFLAGS) -o $(RL)/rshapes.wasm.o
	$(EMCC) -c $(RL)/rtextures.c $(WEBFLAGS) -o $(RL)/rtextures.wasm.o
	$(EMCC) -c $(RL)/rtext.c     $(WEBFLAGS) -o $(RL)/rtext.wasm.o
	$(EMCC) -c $(RL)/rmodels.c   $(WEBFLAGS) -o $(RL)/rmodels.wasm.o
	$(EMCC) -c $(RL)/raudio.c    -Os -DPLATFORM_WEB -I $(RL) -o $(RL)/raudio.wasm.o
	$(EMAR) rcs $@ $(RL)/rcore.wasm.o $(RL)/rshapes.wasm.o $(RL)/rtextures.wasm.o $(RL)/rtext.wasm.o $(RL)/rmodels.wasm.o $(RL)/raudio.wasm.o

# Scene layout editor (tools/scene_editor.cpp) -- standalone GUI tool, not
# part of the game itself. Own subfolder with its own copy of assets.rres +
# the manifest (built via tools/pack_assets.sh, same file build-alt.sh keeps
# up to date for the desktop build) so this tool works regardless of the
# caller's cwd instead of assuming repo root.
SCENE_EDITOR_OUT = build/scene_editor
SCENE_EDITOR_BIN = $(SCENE_EDITOR_OUT)/scene_editor.exe

scene-editor: $(SCENE_EDITOR_BIN) $(SCENE_EDITOR_OUT)/assets.rres $(SCENE_EDITOR_OUT)/assets_manifest.txt

$(SCENE_EDITOR_BIN): tools/scene_editor.cpp
	mkdir -p $(SCENE_EDITOR_OUT)
	$(CXX) -std=c++11 -I raylib/src -I rres/src tools/scene_editor.cpp -o $(SCENE_EDITOR_BIN) $(LDFLAGS)

# Repack whenever the packer OR any file under assets/ changes, so all the
# build.bat targets (which go through make) pick up new/edited art without a
# manual pack step. ASSET_FILES is every file under assets/ -- make treats each
# as a prerequisite, so adding a PNG makes this rule out of date.
ASSET_FILES = $(shell find assets -type f)
build/assets.rres: tools/pack_assets.sh tools/pack_assets.cpp $(ASSET_FILES)
	tools/pack_assets.sh assets build/assets.rres

# force-assets always repacks, regardless of mtimes -- see the web: rule above
# for why the mtime-based build/assets.rres dependency isn't trustworthy after a
# git merge/checkout that renames art.
.PHONY: force-assets
force-assets:
	tools/pack_assets.sh assets build/assets.rres

$(SCENE_EDITOR_OUT)/assets.rres: build/assets.rres
	mkdir -p $(SCENE_EDITOR_OUT)
	cp build/assets.rres $(SCENE_EDITOR_OUT)/assets.rres

$(SCENE_EDITOR_OUT)/assets_manifest.txt: build/assets.rres
	mkdir -p $(SCENE_EDITOR_OUT)
	cp build/assets_manifest.txt $(SCENE_EDITOR_OUT)/assets_manifest.txt

# Dependencies. Desktop raylib is built by our own $(RL)/libraylib.a rule with
# the pinned MinGW toolchain -- do NOT use raylib's own Makefile here, it grabs
# whatever g++ is on PATH (Cygwin on some machines -> Null audio backend).
# The glfw cmake build is gone: rglfw.c compiles GLFW into libraylib.a.
deps: $(RL)/libraylib.a

emsdk-setup:
	emsdk/emsdk.bat install latest
	emsdk/emsdk.bat activate latest --permanent

# Cleanup
clean:
	rm -rf $(DESKTOP_OUT)

clean-web:
	rm -rf $(WEB_OUT) $(RL)/*.wasm.o $(RL)/libraylib.web.a

.PHONY: all web deps emsdk-setup clean clean-web scene-editor
