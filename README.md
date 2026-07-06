# rEngine

A 2D game engine built on top of raylib. Supports scene management, actors, cameras, dialog boxes, and extensible visual effects including 3D.

## Demo Controls

- **A/D or Arrow Keys** - Move
- **Shift** - Sprint
- **Space** - Jump / advance dialog
- **1** - Overworld scene
- **2** - Boss Arena scene
- **H** - Replay dialog
- **ESC** - Exit

## Project Structure

```
rEngine/
+-- src/
|   +-- main.cpp                      Entry point and game loop
|   +-- engine/                       Reusable engine
|   |   +-- Engine.hpp                Master include
|   |   +-- Scene.hpp/cpp             Scene with actors, camera, effects
|   |   +-- SceneActor.hpp/cpp        Base class for all game objects
|   |   +-- SceneCamera.hpp/cpp       Follow cam, zoom, shake, lookahead
|   |   +-- SceneEffect.hpp           Base class for visual effects
|   |   +-- SceneManager.hpp/cpp      Scene switching with transitions
|   |   +-- SceneInputHandler.hpp/cpp Input abstraction
|   |   +-- DialogBox.hpp/cpp         Dialog UI with portrait and typewriter
|   +-- game/                         Game code - replace for your own game
|   |   +-- PlayerActor.hpp
|   |   +-- GameScene.hpp
|   |   +-- BossScene.hpp
|   |   +-- DialogSequences.hpp
|   +-- effects/                      Reusable visual effects
|       +-- MoonEffect.hpp
|       +-- shaders/
|           +-- lighting.vs
|           +-- lighting.fs
+-- raylib/                           raylib submodule
+-- glfw/                             GLFW submodule
+-- Makefile
+-- BUILD_INSTRUCTIONS.md
```

## Engine Overview

**SceneActor** - Subclass for players, enemies, items etc.

**Scene** - Subclass for each level or screen. Holds actors, camera, and effects.

**SceneEffect** - Subclass for visual effects. `drawBackground()` runs before 2D actors, `drawForeground()` after. Depth buffer cleanup is handled automatically.

**SceneManager** - Switches between scenes with fade/slide transitions.

**DialogBox** - Typewriter dialog with portrait, speaker name, and option selection.

## Building Your Own Game

The engine lives in `src/engine/` and is self-contained. To start a new game delete `src/game/` and `src/effects/` and write your own. Include `Engine.hpp` to access everything.

See `BUILD_INSTRUCTIONS.md` to get the project building.
