# Hexagotchi

A game jam submission for [Raylib Game Jam 6.x](https://itch.io/jam/raylib-game-jam-6). Hexagotchi is a hex-based life simulation game where you raise a digital pet through various life stages and scenarios.

## Game Overview

Hexagotchi features:
- **Hex-based movement and interaction** - Navigate a world built on hexagonal tiles
- **Gotchi simulation system** - Care for your digital pet with mood, stats, and needs
- **Multiple scenes** - Explore various locations including Apartment, Office, School, Pizza Parlor, and more
- **Merge mechanics** - Combine items to unlock new features
- **Dynamic events** - Story-driven scenarios and character interactions

## Demo Controls

- **A/D or Arrow Keys** - Move
- **Shift** - Sprint
- **Space** - Interact / advance dialog
- **ESC** - Open pause menu / exit

## Project Structure

```
hexagotchi/
+-- src/
|   +-- main.cpp                      Entry point and game loop
|   +-- engine/                       Reusable engine
|   |   +-- AssetPack.hpp/cpp         Asset management
|   |   +-- AudioManager.hpp/cpp      Audio management
|   |   +-- Button.hpp/cpp            UI button
|   |   +-- Config.hpp/cpp            Configuration
|   |   +-- DialogBox.hpp/cpp         Dialog UI with typewriter
|   |   +-- GameState.hpp/cpp         Global game state
|   |   +-- GotchiDialogBox.hpp/cpp   Gotchi-specific dialog
|   |   +-- HexPathFinder.hpp/cpp     Pathfinding on hex grid
|   |   +-- HexTile.hpp/cpp           Hex tile representation
|   |   +-- HexWorld.hpp/cpp          Hex world management
|   |   +-- SaveManager.hpp/cpp       Save system (disabled for jam)
|   |   +-- SaveWiring.h              Save wiring
|   |   +-- EventBus.hpp/cpp          Event system
|   +-- game/                         Game code
|   |   +-- ApartmentScene.hpp/cpp
|   |   +-- CharacterRegistry.hpp/cpp
|   |   +-- ControlsOverlay.hpp/cpp
|   |   +-- CreditsScene.hpp/cpp
|   |   +-- DeathScene.hpp/cpp
|   |   +-- DeathSequencer.hpp/cpp
|   |   +-- DriversController.hpp/cpp
|   |   +-- EndingScene.hpp/cpp
|   |   +-- Gotchi.hpp/cpp            Main gotchi entity
|   |   +-- GotchiMood.hpp/cpp        Gotchi mood system
|   |   +-- GotchiScene.hpp/cpp       Gotchi interaction scene
|   |   +-- GotchiSim.hpp/cpp         Gotchi simulation (C-core)
|   |   +-- GotchiStats.hpp/cpp       Gotchi stats management
|   |   +-- GotchiStatsScene.hpp/cpp  Stats display scene
|   |   +-- HexBoard.hpp/cpp          Hex board logic
|   |   +-- HexViewScene.hpp/cpp      Hex view scene
|   |   +-- Hotbar.hpp/cpp            Item hotbar
|   |   +-- InputTestScene.hpp/cpp
|   |   +-- KidsVisitScene.hpp/cpp
|   |   +-- MergeController.hpp/cpp   Merge mechanics
|   |   +-- MergeScene.hpp/cpp        Merge scene
|   |   +-- Model3DTestScene.hpp/cpp
|   |   +-- OfficeScene.hpp/cpp
|   |   +-- OptionsScene.hpp/cpp
|   |   +-- PauseMenuOverlay.hpp/cpp
|   |   +-- PizzaParlorScene.hpp/cpp
|   |   +-- PlayerActor.hpp/cpp
|   |   +-- SchoolScene.hpp/cpp
|   |   +-- SceneSelectScene.hpp/cpp
|   |   +-- SkipSceneOverlay.hpp/cpp
|   |   +-- SplashScreenScene.hpp/cpp
|   |   +-- SpriteTestScene.hpp/cpp
|   |   +-- StorySequencer.hpp/cpp    Story progression
|   |   +-- ScenarioDirector.hpp/cpp  Scenario management
|   |   +-- TherapistOfficeScene.hpp/cpp
|   |   +-- TitleScene.hpp/cpp
|   |   +-- TutorialController.hpp/cpp
|   |   +-- ToyAnimationScene.hpp/cpp
|   +-- effects/                      Visual effects
|   |   +-- CityWindowEffect.hpp/cpp
|   |   +-- GrimeShader.hpp/cpp
|   |   +-- JetMesh.hpp/cpp
|   |   +-- LitShader.hpp/cpp
|   |   +-- MeshBuilders.hpp          Mesh construction utilities
|   |   +-- MoonEffect.hpp/cpp
|   |   +-- PortalEffect.hpp/cpp
|   |   +-- PortalBaseMesh.hpp/cpp
|   |   +-- PortalRingMesh.hpp/cpp
|   |   +-- PortalShader.hpp/cpp
|   |   +-- SchoolSkyEffect.hpp/cpp
|   |   +-- StarfieldEffect.hpp/cpp
|   |   +-- SunEffect.hpp/cpp
|   |   +-- TherapistWindowEffect.hpp/cpp
|   +-- events/                       Event system
|       +-- EventBus.hpp/cpp
|       +-- EventType.h
+-- assets/                           Game assets
|   +-- assets.rres                   Packed asset resource
|   +-- assets_manifest.txt           Asset manifest
+-- raylib/                           raylib submodule
+-- glfw/                             GLFW submodule
+-- rres/                             RRES asset format submodule
+-- emsdk/                            Emscripten submodule (optional)
+-- CMakeLists.txt
+-- Makefile
+-- BUILD_INSTRUCTIONS.md
+-- release/                          Built releases
|   +-- rc1/, rc2/, rc3/, rc4/        Release candidates
+-- web/                              Web build files
```

## Engine Overview

**SceneActor** - Base class for all game objects. Handles positioning, rendering, and updates.

**Scene** - Represents a screen or level. Contains actors, camera, and effects. The scene manager handles transitions between scenes.

**SceneEffect** - Base class for visual effects. `drawBackground()` runs before 2D actors, `drawForeground()` after. Depth buffer cleanup is handled automatically.

**SceneManager** - Handles scene switching with fade/slide transitions. Manages the active scene and transitions.

**DialogBox** - Typewriter-style dialog with portrait, speaker name, and option selection. SupportsGotchi-specific dialog variants.

**HexWorld/HexTile/HexPathFinder** - Hexagonal grid system for movement and interaction. Supports pathfinding on the hex grid.

**GotchiSim** - C-core simulation that drives the gotchi's behavior, mood, and stats over time.

**EventBus** - Event system for decoupled communication between game systems.

## Building Your Own Game

The engine lives in `src/engine/` and is self-contained. To start a new game, modify or replace `src/game/` and `src/effects/` with your own content. Include `Engine.hpp` to access everything.

See `BUILD_INSTRUCTIONS.md` for detailed build instructions for Linux, Windows, and Web.

## Release Builds

Pre-built releases are available in the `release/` directory:
- **rc4/** - Latest release candidate with desktop and web builds

### Desktop
- `release/rc4/desktop/game.exe` - Windows executable
- `release/rc4/desktop/assets.rres` - Asset resource file

### Web
- `release/rc4/web/index.html` - Web entry point
- `release/rc4/web/index.js` - JavaScript loader
- `release/rc4/web/index.wasm` - WebAssembly binary
- `release/rc4/web/index.data` - Asset data

## Game Jam Information

**Raylib Game Jam 6.x** - This game was created during the Raylib Game Jam 6.x event. The save system has been disabled for this jam submission to focus on core gameplay mechanics.

## License

This project uses raylib (zlib license), GLFW (GPL), and custom code.
