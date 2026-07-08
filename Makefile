CXX  = g++
EMCC = emsdk/python/3.13.3_64bit/python.exe emsdk/upstream/emscripten/emcc.py
EMAR = emsdk/python/3.13.3_64bit/python.exe emsdk/upstream/emscripten/emar.py
RL   = raylib/src

SRCS        = src/main.cpp $(wildcard src/engine/*.cpp) $(wildcard src/game/*.cpp) $(wildcard src/effects/*.cpp)
INCLUDES    = -I src/engine -I src/game -I src/effects -I $(RL) -I glfw/include -I rres/src
CXXFLAGS    = -std=c++17 $(INCLUDES)

# Desktop linker flags differ per platform: Windows (devkitPro/Cygwin) links
# against the Win32 GL/window libs, Linux needs raylib's X11 dependencies
# instead. Windows_NT is set by cmd.exe/PowerShell's environment; anything
# else falls through to the Linux flags.
ifeq ($(OS),Windows_NT)
LDFLAGS     = -L $(RL) -L glfw/build/src -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm
else
LDFLAGS     = -L $(RL) -L glfw/build/src -lraylib -lglfw3 -pthread -lm -ldl -lX11 -lXrandr -lXinerama -lXi -lXcursor
endif

WEBINCLUDES = -I src/engine -I src/game -I src/effects -I $(RL) -I rres/src
WEBFLAGS    = -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -I $(RL)
# Preload just the packed assets.rres, not the whole assets/ tree -- everything
# loads through AssetPack now, so the loose PNGs never need to be in the
# virtual filesystem.
WEBLINK     = -s USE_GLFW=3 -s ASYNCIFY -s WASM=1 -s TOTAL_MEMORY=67108864 -s GL_ENABLE_GET_PROC_ADDRESS -DPLATFORM_WEB -Os -std=c++11 --preload-file build/assets.rres@assets.rres

DESKTOP_OUT = build/desktop
WEB_OUT     = build/web
OBJ         = $(SRCS:src/%.cpp=$(DESKTOP_OUT)/obj/%.o)

# Every file under assets/ -- assets.rres is rebuilt whenever any of these
# change, so packing is automatic and never needs a manual step or a list of
# files to remember to add. Packed straight into build/ -- it's a generated
# artifact like everything else there, never the repo root.
ASSET_FILES = $(shell find assets -type f 2>/dev/null)
PACKER_BIN  = build/devtools/pack_assets.exe
ASSET_PACK  = build/assets.rres

# Desktop
all: $(ASSET_PACK) $(DESKTOP_OUT)/game.exe $(DESKTOP_OUT)/assets.rres

# Link the compiled object files into the executable.
$(DESKTOP_OUT)/game.exe: $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

# game.exe reads assets.rres via a relative path, so it must sit next to the
# exe -- otherwise launching game.exe from anywhere other than the repo root
# can't find it (the same bug this whole packing system was built to fix,
# just one level up).
$(DESKTOP_OUT)/assets.rres: $(ASSET_PACK)
	mkdir -p $(DESKTOP_OUT)
	cp $(ASSET_PACK) $(DESKTOP_OUT)/assets.rres

# Compile each source to an object file. -MMD -MP makes g++ emit a .d file
# next to each .o listing every header that source #includes, so editing a
# header rebuilds exactly the sources that use it (pulled in by -include below).
$(DESKTOP_OUT)/obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) -MMD -MP -c $< -o $@ $(CXXFLAGS)

# Pull in the auto-generated header-dependency lists (ignored on first build).
-include $(OBJ:.o=.d)

# Asset packing: rebuild assets.rres whenever any file under assets/ changes.
# No manual step and no list to keep in sync -- pack_assets.cpp walks assets/
# itself and packs everything it finds.
$(PACKER_BIN): tools/pack_assets.cpp
	mkdir -p build/devtools
	$(CXX) -std=c++11 $(INCLUDES) tools/pack_assets.cpp -o $(PACKER_BIN) $(LDFLAGS)

$(ASSET_PACK): $(PACKER_BIN) $(ASSET_FILES)
	mkdir -p build
	$(PACKER_BIN) assets $(ASSET_PACK)

# Scene layout editor (tools/scene_editor.cpp) -- standalone GUI tool, not
# part of the game itself. Own subfolder so it doesn't get lost/confused
# among the other devtools binaries (pack_assets, the screenshot tool's
# game_devtool.exe, etc), with its own copy of assets.rres + the manifest
# pack_assets.cpp writes alongside it -- same convention as build/desktop/'s
# own assets.rres copy -- so this tool works regardless of the caller's cwd
# (Explorer double-click, a shortcut, whatever) instead of assuming repo root.
SCENE_EDITOR_OUT = build/scene_editor
SCENE_EDITOR_BIN = $(SCENE_EDITOR_OUT)/scene_editor.exe

scene-editor: $(SCENE_EDITOR_BIN) $(SCENE_EDITOR_OUT)/assets.rres $(SCENE_EDITOR_OUT)/assets_manifest.txt

$(SCENE_EDITOR_BIN): tools/scene_editor.cpp
	mkdir -p $(SCENE_EDITOR_OUT)
	$(CXX) -std=c++11 -I raylib/src -I rres/src tools/scene_editor.cpp -o $(SCENE_EDITOR_BIN) $(LDFLAGS)

$(SCENE_EDITOR_OUT)/assets.rres: $(ASSET_PACK)
	mkdir -p $(SCENE_EDITOR_OUT)
	cp $(ASSET_PACK) $(SCENE_EDITOR_OUT)/assets.rres

$(SCENE_EDITOR_OUT)/assets_manifest.txt: $(ASSET_PACK)
	mkdir -p $(SCENE_EDITOR_OUT)
	cp build/assets_manifest.txt $(SCENE_EDITOR_OUT)/assets_manifest.txt

# Web
web: $(ASSET_PACK) $(RL)/libraylib.web.a
	mkdir -p $(WEB_OUT)
	$(EMCC) $(SRCS) $(RL)/libraylib.web.a -o $(WEB_OUT)/index.html $(WEBINCLUDES) $(WEBLINK) --shell-file web/shell.html

$(RL)/libraylib.web.a:
	$(EMCC) -c $(RL)/rcore.c     $(WEBFLAGS) -o $(RL)/rcore.wasm.o
	$(EMCC) -c $(RL)/rshapes.c   $(WEBFLAGS) -o $(RL)/rshapes.wasm.o
	$(EMCC) -c $(RL)/rtextures.c $(WEBFLAGS) -o $(RL)/rtextures.wasm.o
	$(EMCC) -c $(RL)/rtext.c     $(WEBFLAGS) -o $(RL)/rtext.wasm.o
	$(EMCC) -c $(RL)/rmodels.c   $(WEBFLAGS) -o $(RL)/rmodels.wasm.o
	$(EMCC) -c $(RL)/raudio.c    -Os -DPLATFORM_WEB -I $(RL) -o $(RL)/raudio.wasm.o
	$(EMAR) rcs $@ $(RL)/rcore.wasm.o $(RL)/rshapes.wasm.o $(RL)/rtextures.wasm.o $(RL)/rtext.wasm.o $(RL)/rmodels.wasm.o $(RL)/raudio.wasm.o

# Dependencies
deps:
	make -C raylib/src
	cmake -S glfw -B glfw/build -DBUILD_SHARED_LIBS=OFF -DGLFW_BUILD_WAYLAND=OFF -DGLFW_BUILD_X11=OFF
	make -C glfw/build

emsdk-setup:
	emsdk/emsdk.bat install latest
	emsdk/emsdk.bat activate latest --permanent

# Cleanup
clean:
	rm -rf $(DESKTOP_OUT)

clean-web:
	rm -rf $(WEB_OUT) $(RL)/*.wasm.o $(RL)/libraylib.web.a

.PHONY: all web deps emsdk-setup clean clean-web scene-editor
