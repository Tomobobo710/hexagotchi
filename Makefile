CXX  = g++
EMCC = python emsdk/upstream/emscripten/emcc.py
EMAR = python emsdk/upstream/emscripten/emar.py
RL   = raylib/src

SRCS        = src/main.cpp $(wildcard src/engine/*.cpp) $(wildcard src/game/*.cpp) $(wildcard src/effects/*.cpp)
INCLUDES    = -I src/engine -I src/game -I src/effects -I $(RL) -I glfw/include
CXXFLAGS    = -std=c++11 $(INCLUDES)
LDFLAGS     = -L $(RL) -L glfw/build/src -lraylib -lglfw3 -lopengl32 -lgdi32 -lwinmm

WEBINCLUDES = -I src/engine -I src/game -I src/effects -I $(RL)
WEBFLAGS    = -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -I $(RL)
WEBLINK     = -s USE_GLFW=3 -s ASYNCIFY -s WASM=1 -s TOTAL_MEMORY=67108864 -s GL_ENABLE_GET_PROC_ADDRESS -DPLATFORM_WEB -Os -std=c++11

DESKTOP_OUT = build/desktop
WEB_OUT     = build/web
OBJ         = $(SRCS:src/%.cpp=$(DESKTOP_OUT)/obj/%.o)

# Desktop
all: $(DESKTOP_OUT)/game.exe

# Link the compiled object files into the executable.
$(DESKTOP_OUT)/game.exe: $(OBJ)
	$(CXX) $(OBJ) -o $@ $(LDFLAGS)

# Compile each source to an object file. -MMD -MP makes g++ emit a .d file
# next to each .o listing every header that source #includes, so editing a
# header rebuilds exactly the sources that use it (pulled in by -include below).
$(DESKTOP_OUT)/obj/%.o: src/%.cpp
	mkdir -p $(dir $@)
	$(CXX) -MMD -MP -c $< -o $@ $(CXXFLAGS)

# Pull in the auto-generated header-dependency lists (ignored on first build).
-include $(OBJ:.o=.d)

# Web
web: $(RL)/libraylib.web.a
	mkdir -p $(WEB_OUT)
	$(EMCC) $(SRCS) $(RL)/libraylib.web.a -o $(WEB_OUT)/game.html $(WEBINCLUDES) $(WEBLINK) --shell-file $(RL)/minshell.html

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

.PHONY: all web deps emsdk-setup clean clean-web
