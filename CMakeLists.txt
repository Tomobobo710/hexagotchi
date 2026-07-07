cmake_minimum_required(VERSION 3.16)
project(hexagotchi CXX C)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set essential CMake variables for C compilation and static library creation
set(CMAKE_C_ARCHIVE_CREATE "<CMAKE_AR> qc <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_APPEND "<CMAKE_AR> q <TARGET> <LINK_FLAGS> <OBJECTS>")
set(CMAKE_C_ARCHIVE_FINISH "<CMAKE_RANLIB> <TARGET>")
set(CMAKE_C_COMPILE_OBJECT "<CMAKE_C_COMPILER> <DEFINES> <INCLUDES> <FLAGS> -o <OBJECT> -c <SOURCE>")
set(CMAKE_C_LINK_EXECUTABLE "<CMAKE_C_COMPILER> <FLAGS> <CMAKE_C_LINK_FLAGS> <LINK_FLAGS> <OBJECTS> -o <TARGET> <LINK_LIBRARIES>")

# ============================================
# Desktop build (default)
# ============================================
option(RBUILD_DESKTOP "Build desktop executable" ON)
option(RBUILD_WEB "Build WebAssembly target" OFF)

if(RBUILD_DESKTOP)
    # Include directories
    include_directories(src/engine src/game src/effects raylib/src glfw/include)

    # Source files
    file(GLOB ENGINE_SRCS "src/engine/*.cpp")
    file(GLOB GAME_SRCS "src/game/*.cpp")
    file(GLOB EFFECTS_SRCS "src/effects/*.cpp")
    set(SOURCES src/main.cpp ${ENGINE_SRCS} ${GAME_SRCS} ${EFFECTS_SRCS})

    # Build raylib as a static library
    add_library(raylib STATIC
        raylib/src/rcore.c
        raylib/src/rshapes.c
        raylib/src/rtextures.c
        raylib/src/rtext.c
        raylib/src/rmodels.c
        raylib/src/raudio.c
    )

    target_include_directories(raylib PUBLIC
        raylib/src
    )

    target_compile_definitions(raylib PRIVATE
        PLATFORM_DESKTOP_GLFW
        _GNU_SOURCE
        RAYLIB_MODULE_AUDIO=1
        RAYLIB_MODULE_MODELS=1
        _CRT_SECURE_NO_WARNINGS
    )

    # Platform-specific libraries for raylib
    if(WIN32)
        # Windows: opengl32, gdi32, winmm
        target_link_libraries(raylib PUBLIC opengl32 gdi32 winmm)
    else()
        # Linux: GL, m, pthread, dl, rt
        target_link_libraries(raylib PUBLIC GL m pthread dl rt)
    endif()

    # Build glfw as a static library
    add_subdirectory(glfw)

    # Main game executable
    add_executable(game ${SOURCES})

    target_include_directories(game PRIVATE
        src/engine
        src/game
        src/effects
        raylib/src
        glfw/include
    )

    target_link_libraries(game PRIVATE
        raylib
        glfw
    )

    # Statically link MinGW runtime to avoid missing DLLs
    set_target_properties(game PROPERTIES
        LINK_FLAGS "-static-libgcc -static-libstdc++ -static"
    )

    # Set output directories for desktop build
    set_target_properties(game PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/desktop/$<CONFIG>
    )
endif()

# ============================================
# Web/WASM build
# ============================================
if(RBUILD_WEB)
    # Find emscripten. Prefer the repo-local emsdk submodule, then $EMSDK / $HOME.
    find_program(EMSCRIPTEN_EXECUTABLE emcc
        PATHS
            "${CMAKE_SOURCE_DIR}/emsdk/upstream/emscripten"
            "$ENV{EMSDK}/upstream/emscripten"
            "$ENV{HOME}/emsdk/upstream/emscripten"
        NO_DEFAULT_PATH)

    if(NOT EMSCRIPTEN_EXECUTABLE)
        message(FATAL_ERROR "Emscripten not found. Expected ./emsdk submodule, or set EMSDK environment / run emsdk activate.")
    endif()

    message(STATUS "Found Emscripten: ${EMSCRIPTEN_EXECUTABLE}")

    # emar lives next to emcc; derive it so we don't rely on it being on PATH.
    get_filename_component(EMSCRIPTEN_BIN_DIR "${EMSCRIPTEN_EXECUTABLE}" DIRECTORY)
    find_program(EMAR_EXECUTABLE emar PATHS "${EMSCRIPTEN_BIN_DIR}" NO_DEFAULT_PATH)
    if(NOT EMAR_EXECUTABLE)
        set(EMAR_EXECUTABLE emar)
    endif()

    # Web output directory
    set(WEB_OUTPUT_DIR "${CMAKE_SOURCE_DIR}/build/web")

    # Ensure web output directory exists
    file(MAKE_DIRECTORY ${WEB_OUTPUT_DIR})

    # Build raylib for web using emscripten
    add_custom_target(raylib-web ALL
        COMMAND ${CMAKE_COMMAND} -E echo "Building raylib for web..."
        COMMAND ${EMSCRIPTEN_EXECUTABLE} -c ${CMAKE_SOURCE_DIR}/raylib/src/rcore.c -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -o ${WEB_OUTPUT_DIR}/rcore.wasm.o
        COMMAND ${EMSCRIPTEN_EXECUTABLE} -c ${CMAKE_SOURCE_DIR}/raylib/src/rshapes.c -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -o ${WEB_OUTPUT_DIR}/rshapes.wasm.o
        COMMAND ${EMSCRIPTEN_EXECUTABLE} -c ${CMAKE_SOURCE_DIR}/raylib/src/rtextures.c -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -o ${WEB_OUTPUT_DIR}/rtextures.wasm.o
        COMMAND ${EMSCRIPTEN_EXECUTABLE} -c ${CMAKE_SOURCE_DIR}/raylib/src/rtext.c -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -o ${WEB_OUTPUT_DIR}/rtext.wasm.o
        COMMAND ${EMSCRIPTEN_EXECUTABLE} -c ${CMAKE_SOURCE_DIR}/raylib/src/rmodels.c -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -o ${WEB_OUTPUT_DIR}/rmodels.wasm.o
        COMMAND ${EMSCRIPTEN_EXECUTABLE} -c ${CMAKE_SOURCE_DIR}/raylib/src/raudio.c -Os -DPLATFORM_WEB -o ${WEB_OUTPUT_DIR}/raudio.wasm.o
        COMMAND ${EMAR_EXECUTABLE} rcs ${WEB_OUTPUT_DIR}/libraylib.web.a ${WEB_OUTPUT_DIR}/rcore.wasm.o ${WEB_OUTPUT_DIR}/rshapes.wasm.o ${WEB_OUTPUT_DIR}/rtextures.wasm.o ${WEB_OUTPUT_DIR}/rtext.wasm.o ${WEB_OUTPUT_DIR}/rmodels.wasm.o ${WEB_OUTPUT_DIR}/raudio.wasm.o
        DEPENDS raylib/src/rcore.c raylib/src/rshapes.c raylib/src/rtextures.c raylib/src/rtext.c raylib/src/rmodels.c raylib/src/raudio.c
        WORKING_DIRECTORY ${WEB_OUTPUT_DIR}
    )

    # Build web executable
    # List source files explicitly (emcc doesn't expand globs in custom commands)
    set(WEB_SOURCES
        ${CMAKE_SOURCE_DIR}/src/main.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/Button.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/DialogBox.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/Scene.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneActor.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneCamera.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneInputHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneManager.cpp
        ${CMAKE_SOURCE_DIR}/src/game/BossScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/ControlsOverlay.cpp
        ${CMAKE_SOURCE_DIR}/src/game/GameScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/InputTestScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/PauseMenuOverlay.cpp
        ${CMAKE_SOURCE_DIR}/src/game/PlayerActor.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/MoonEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/StarfieldEffect.cpp
    )
    add_custom_target(game-web ALL
        COMMAND ${EMSCRIPTEN_EXECUTABLE} ${WEB_SOURCES} ${WEB_OUTPUT_DIR}/libraylib.web.a -o ${WEB_OUTPUT_DIR}/game.html -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -I ${CMAKE_SOURCE_DIR}/src/engine -I ${CMAKE_SOURCE_DIR}/src/game -I ${CMAKE_SOURCE_DIR}/src/effects -I ${CMAKE_SOURCE_DIR}/raylib/src --shell-file ${CMAKE_SOURCE_DIR}/raylib/src/minshell.html -s USE_GLFW=3 -s ASYNCIFY -s WASM=1 -s TOTAL_MEMORY=67108864 -s GL_ENABLE_GET_PROC_ADDRESS
        DEPENDS raylib-web ${WEB_SOURCES}
        WORKING_DIRECTORY ${WEB_OUTPUT_DIR}
    )

    message(STATUS "Web build configured. Run: cmake --build . --target game-web")
endif()
