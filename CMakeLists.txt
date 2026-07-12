cmake_minimum_required(VERSION 3.16)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are banned. Run: cmake -B build -D RBUILD_DESKTOP=ON -D RBUILD_WEB=OFF")
endif()
project(hexagotchi CXX C)

set(CMAKE_CXX_STANDARD 17)
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
    include_directories(src/engine src/game src/effects src/events src/flags raylib/src glfw/include rres/src)

    # Source files
    file(GLOB ENGINE_SRCS "src/engine/*.cpp" "src/engine/*/*.cpp")
    file(GLOB GAME_SRCS
        "src/game/ControlsOverlay.cpp"
        "src/game/CharacterRegistry.cpp"
        "src/game/CreditsScene.cpp"
        "src/game/DeathScene.cpp"
        "src/game/DriversController.cpp"
        "src/game/EndingScene.cpp"
        "src/game/Gotchi.cpp"
        "src/game/GotchiMood.cpp"
        "src/game/GotchiScene.cpp"
        "src/game/GotchiStats.cpp"
        "src/game/GotchiStatsScene.cpp"
        "src/game/GotchiSim.cpp"
        "src/game/HexBoard.cpp"
        "src/game/HexViewScene.cpp"
        "src/game/Hotbar.cpp"
        "src/game/InputTestScene.cpp"
        "src/game/KidsVisitScene.cpp"
        "src/game/MergeController.cpp"
        "src/game/MergeScene.cpp"
        "src/game/Model3DTestScene.cpp"
        "src/game/OfficeScene.cpp"
        "src/game/OptionsScene.cpp"
        "src/game/PauseMenuOverlay.cpp"
        "src/game/PizzaParlorScene.cpp"
        "src/game/PlayerActor.cpp"
        "src/game/ApartmentScene.cpp"
        "src/game/SchoolScene.cpp"
        "src/game/SceneSelectScene.cpp"
        "src/game/SkipSceneOverlay.cpp"
        "src/game/SplashScreenScene.cpp"
        "src/game/SpriteTestScene.cpp"
        "src/game/StorySequencer.cpp"
        "src/game/DeathSequencer.cpp"
        "src/game/ScenarioDirector.cpp"
        "src/game/TherapistOfficeScene.cpp"
        "src/game/TitleScene.cpp"
        "src/game/TutorialController.cpp"
        "src/game/ToyAnimationScene.cpp"
    )
    file(GLOB EFFECTS_SRCS "src/effects/*.cpp")
    file(GLOB EVENT_SRCS "src/events/*.cpp")
    file(GLOB TEST_SRCS "tests/*.cpp")

    # Only include TestGrimeShader.cpp for the tests build
    # TestSaveManager.cpp has complex dependencies (GameState, SaveManager)
    set(GRIME_TEST_SRCS "")
    foreach(f ${TEST_SRCS})
        if(f MATCHES "TestGrimeShader")
            list(APPEND GRIME_TEST_SRCS ${f})
        endif()
    endforeach()

    # Test executable for ExploreGate
    add_executable(test_explore_gate
        tests/TestExploreGate.cpp
        src/game/GotchiStats.cpp
    )
    target_include_directories(test_explore_gate PRIVATE . src src/engine src/game src/events src/flags raylib/src)
    target_link_libraries(test_explore_gate PRIVATE raylib glfw)

    set(SOURCES src/main.cpp ${ENGINE_SRCS} ${GAME_SRCS} ${EFFECTS_SRCS} ${EVENT_SRCS})

    # Test executable for MergeController
    # Test executables - disabled for this release build to avoid dependency
    # complexity; tests can be built separately if needed for development
    if(0)
        set(TEST_ENGINE_SRCS
            src/engine/AssetPack.cpp
            src/engine/Config.cpp
            src/engine/DialogBox.cpp
            src/engine/GameState.cpp
            src/engine/SaveManager.cpp
            src/engine/Scene.cpp
            src/engine/SceneActor.cpp
            src/engine/SceneCamera.cpp
            src/engine/SceneInputHandler.cpp
            src/engine/SceneManager.cpp
            src/main.cpp
        )
        set(TEST_EVENT_SRCS
            src/events/EventBus.cpp
        )
        set(TEST_GAME_SRCS
            src/game/MergeController.cpp
            src/game/StorySequencer.cpp
            src/game/MergeScene.cpp
            src/game/ScenarioDirector.cpp
            src/game/GotchiSim.cpp
            src/game/GotchiStats.cpp
            src/game/GotchiMood.cpp
            src/game/CharacterRegistry.cpp
        )
        set(TEST_EFFECTS_SRCS
            src/effects/PortalEffect.cpp
            src/effects/PortalShader.cpp
            src/effects/LitShader.cpp
            src/effects/PortalRingMesh.cpp
            src/effects/PortalBaseMesh.cpp
            src/effects/MoonEffect.cpp
        )
        set(TEST_SRCS
            tests/TestMergeController.cpp
            ${TEST_ENGINE_SRCS}
            ${TEST_EVENT_SRCS}
            ${TEST_GAME_SRCS}
            ${TEST_EFFECTS_SRCS}
        )
        add_executable(test_merge_controller ${TEST_SRCS})
        target_include_directories(test_merge_controller PRIVATE . src src/engine src/game src/events src/flags raylib/src)
        target_link_libraries(test_merge_controller PRIVATE raylib glfw)

        # Test executable for GotchiSim
        set(GOTCHI_SIM_TEST_SRCS
            tests/TestGotchiSim.cpp
            ${TEST_ENGINE_SRCS}
            ${TEST_EVENT_SRCS}
            ${TEST_GAME_SRCS}
            ${TEST_EFFECTS_SRCS}
        )
        add_executable(test_gotchi_sim ${GOTCHI_SIM_TEST_SRCS})
        target_include_directories(test_gotchi_sim PRIVATE . src src/engine src/game src/events src/flags raylib/src)
        target_link_libraries(test_gotchi_sim PRIVATE raylib glfw)

        # Test executable for StorySequencer
        add_executable(test_story_sequencer
            tests/TestStorySequencer.cpp
            ${TEST_ENGINE_SRCS}
            ${TEST_EVENT_SRCS}
            ${TEST_GAME_SRCS}
            ${TEST_EFFECTS_SRCS}
        )
        target_include_directories(test_story_sequencer PRIVATE . src src/engine src/game src/events src/flags raylib/src)
        target_link_libraries(test_story_sequencer PRIVATE raylib glfw)
    endif()

    # DISABLED: Save system shut off for game jam
    # Test executable for SaveManager
    # set(SAVE_TEST_SRCS
    #     tests/TestSaveManager.cpp
    #     src/engine/SaveManager.cpp
    #     src/engine/GameState.cpp
    #     src/game/GotchiStats.cpp
    #     src/game/GotchiMood.cpp
    # )
    # add_executable(test_save_manager ${SAVE_TEST_SRCS})
    # target_include_directories(test_save_manager PRIVATE . src src/engine src/game raylib/src rres/src)
    # target_link_libraries(test_save_manager PRIVATE raylib glfw)

    # Test executable for SaveWiring (includes SaveManager tests)
    # set(SAVE_WIRING_TEST_SRCS
    #     tests/TestSaveWiring.cpp
    #     src/engine/SaveManager.cpp
    #     src/engine/GameState.cpp
    #     src/game/GotchiStats.cpp
    #     src/game/GotchiMood.cpp
    #     src/events/EventBus.cpp
    # )
    # add_executable(test_save_wiring ${SAVE_WIRING_TEST_SRCS})
    # target_include_directories(test_save_wiring PRIVATE . src src/engine src/game src/events src/flags raylib/src rres/src)
    # target_link_libraries(test_save_wiring PRIVATE raylib glfw)

    # Test executable for MergeController


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
        # Windows: opengl32/gdi32/winmm plus the WASAPI/COM libs miniaudio's
        # audio backend needs (ole32 etc.) -- see the Makefile's LDFLAGS
        # comment: without a real backend miniaudio silently falls back to the
        # Null device and the game is mute.
        target_link_libraries(raylib PUBLIC opengl32 gdi32 winmm ole32 oleaut32 uuid ksuser)
    else()
        # Linux: GL, m, pthread, dl, rt
        target_link_libraries(raylib PUBLIC GL m pthread dl rt)
    endif()

    # Build glfw as a static library
    add_subdirectory(glfw)

    # Build pack_assets tool (C++ version for Linux/macOS)
    add_executable(pack_assets tools/pack_assets.cpp)
    target_include_directories(pack_assets PRIVATE raylib/src rres/src)
    target_link_libraries(pack_assets PRIVATE raylib glfw)

    # Main game executable
    add_executable(game ${SOURCES})

    target_include_directories(game PRIVATE
        src/engine
        src/game
        src/effects
        raylib/src
        glfw/include
        rres/src
    )

    target_link_libraries(game PRIVATE
        raylib
        glfw
    )

    # Statically link MinGW runtime to avoid missing DLLs (Windows only)
    if(WIN32)
        set_target_properties(game PROPERTIES
            LINK_FLAGS "-static-libgcc -static-libstdc++ -static"
        )
    else()
        # On Linux, we can't use -static because libGL is only available as a shared library
        set_target_properties(game PROPERTIES
            LINK_FLAGS "-static-libgcc -static-libstdc++"
        )
    endif()

    # Set output directories for desktop build
    set_target_properties(game PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/build/desktop/$<CONFIG>
    )

    # Build assets.rres automatically before building the game
    # Note: On Windows (cross-compile), assets.rres must be built on the target
    # machine using the native pack_assets tool
    if(NOT RBUILD_WEB AND NOT WIN32)
        # Ensure pack_assets is built before assets.rres
        add_dependencies(pack_assets raylib glfw)

        # Add a custom target to build assets.rres using the pack_assets tool
        # The pack_assets executable is built by the pack_assets target
        add_custom_target(assets.rres ALL
            COMMAND ${CMAKE_COMMAND} -E echo "Building assets.rres..."
            COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/devtools
            COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_FILE:pack_assets> $<TARGET_FILE_DIR:pack_assets>/pack_assets
            COMMAND $<TARGET_FILE:pack_assets> ${CMAKE_SOURCE_DIR}/assets ${CMAKE_SOURCE_DIR}/assets.rres
            DEPENDS pack_assets
            WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
        )

        # Copy assets.rres to the build output directory
        add_custom_command(TARGET game POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/assets.rres
            $<TARGET_FILE_DIR:game>/assets.rres
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_SOURCE_DIR}/assets_manifest.txt
            $<TARGET_FILE_DIR:game>/assets_manifest.txt
        )
    endif()
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
        ${CMAKE_SOURCE_DIR}/src/engine/AssetPack.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/AudioManager.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/Config.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/Button.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/DialogBox.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/GameState.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/HexPathFinder.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/HexTile.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/HexWorld.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SaveManager.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/Scene.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneActor.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneCamera.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneInputHandler.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/SceneManager.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/GotchiDialogBox.cpp
        ${CMAKE_SOURCE_DIR}/src/engine/TileType.cpp
        ${CMAKE_SOURCE_DIR}/src/game/ControlsOverlay.cpp
        ${CMAKE_SOURCE_DIR}/src/game/Gotchi.cpp
        ${CMAKE_SOURCE_DIR}/src/game/GotchiMood.cpp
        ${CMAKE_SOURCE_DIR}/src/game/GotchiScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/GotchiStats.cpp
        ${CMAKE_SOURCE_DIR}/src/game/HexBoard.cpp
        ${CMAKE_SOURCE_DIR}/src/game/HexViewScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/Hotbar.cpp
        ${CMAKE_SOURCE_DIR}/src/game/HexItemOverlay.cpp
        ${CMAKE_SOURCE_DIR}/src/game/InputTestScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/KidsVisitScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/MergeController.cpp
        ${CMAKE_SOURCE_DIR}/src/game/PauseMenuOverlay.cpp
        ${CMAKE_SOURCE_DIR}/src/game/PizzaParlorScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/PlayerActor.cpp
        ${CMAKE_SOURCE_DIR}/src/game/ApartmentScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/OfficeScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/OptionsScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/SchoolScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/SceneSelectScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/SkipSceneOverlay.cpp
        ${CMAKE_SOURCE_DIR}/src/game/SplashScreenScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/GotchiStatsScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/GotchiSim.cpp
        ${CMAKE_SOURCE_DIR}/src/game/StorySequencer.cpp
        ${CMAKE_SOURCE_DIR}/src/game/DeathSequencer.cpp
        ${CMAKE_SOURCE_DIR}/src/game/DeathScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/EndingScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/CreditsScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/ScenarioDirector.cpp
        ${CMAKE_SOURCE_DIR}/src/game/TutorialController.cpp
        ${CMAKE_SOURCE_DIR}/src/game/TherapistOfficeScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/SpriteTestScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/TitleScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/TomSceneActor.cpp
        ${CMAKE_SOURCE_DIR}/src/game/ToyAnimationScene.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/CityWindowEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/MoonEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/SunEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/TherapistWindowEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/VehicleMesh.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/SchoolSkyEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/StarfieldEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/LitShader.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/GrimeShader.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/JetMesh.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/UnlitShader.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/TomagotchiToyMesh.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/TitleScrollShader.cpp
        ${CMAKE_SOURCE_DIR}/src/game/MergeScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/Model3DTestScene.cpp
        ${CMAKE_SOURCE_DIR}/src/game/DriversController.cpp
        ${CMAKE_SOURCE_DIR}/src/game/CharacterRegistry.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/PortalEffect.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/PortalBaseMesh.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/PortalRingMesh.cpp
        ${CMAKE_SOURCE_DIR}/src/effects/PortalShader.cpp
        ${CMAKE_SOURCE_DIR}/src/events/EventBus.cpp
    )

    # Ensure assets.rres and assets_manifest.txt exist for web build (copy from project root if not present)
    add_custom_target(web_assets_rres
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets.rres ${WEB_OUTPUT_DIR}/assets.rres
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_SOURCE_DIR}/assets_manifest.txt ${WEB_OUTPUT_DIR}/assets_manifest.txt
        DEPENDS ${CMAKE_SOURCE_DIR}/assets.rres ${CMAKE_SOURCE_DIR}/assets_manifest.txt
        WORKING_DIRECTORY ${WEB_OUTPUT_DIR}
    )

    add_custom_target(game-web ALL
        COMMAND ${EMSCRIPTEN_EXECUTABLE} ${WEB_SOURCES} ${WEB_OUTPUT_DIR}/libraylib.web.a -o ${WEB_OUTPUT_DIR}/game.html -Os -DPLATFORM_WEB -DGRAPHICS_API_OPENGL_ES2 -I ${CMAKE_SOURCE_DIR}/src/engine -I ${CMAKE_SOURCE_DIR}/src/game -I ${CMAKE_SOURCE_DIR}/src/effects -I ${CMAKE_SOURCE_DIR}/src/events -I ${CMAKE_SOURCE_DIR}/src/flags -I ${CMAKE_SOURCE_DIR}/raylib/src -I ${CMAKE_SOURCE_DIR}/rres/src --shell-file ${CMAKE_SOURCE_DIR}/web/shell.html -s USE_GLFW=3 -s ASYNCIFY -s WASM=1 -s TOTAL_MEMORY=67108864 -s GL_ENABLE_GET_PROC_ADDRESS --preload-file ${WEB_OUTPUT_DIR}/assets.rres@assets.rres
        DEPENDS raylib-web web_assets_rres ${WEB_SOURCES}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )

    message(STATUS "Web build configured. Run: cmake --build . --target game-web")
endif()
