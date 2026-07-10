#include "SceneManager.hpp"
#include "DialogBox.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "game/InputTestScene.hpp"
#include "game/HexViewScene.hpp"
#include "game/SpriteTestScene.hpp"
#include "game/GotchiScene.hpp"
#include "game/GotchiSim.hpp"
#include "game/DriversController.hpp"
#include "game/PizzaParlorScene.hpp"
#include "game/ApartmentScene.hpp"
#include "game/TherapistOfficeScene.hpp"
#include "game/OfficeScene.hpp"
#include "game/SchoolScene.hpp"
#include "game/Model3DTestScene.hpp"
#include "game/MergeScene.hpp"
#include "game/SceneSelectScene.hpp"
#include "game/TitleScene.hpp"
#include "game/GotchiStatsScene.hpp"
#include "game/MergeController.hpp"
#include "game/StorySequencer.hpp"
#include "game/DialogSequences.hpp"
#include "engine/GameState.h"
#include "engine/SaveManager.h"
#include "engine/SaveWiring.h"
#include "events/EventBus.h"
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>

// Global exit request flag
bool exitRequested = false;

// Global game state and event bus
GameState globalGameState;
EventBus  globalEventBus;

// MergeController - created in main() and used in UpdateDrawFrame
MergeController* mergeController = nullptr;

// StorySequencer - created in main() and used in UpdateDrawFrame
StorySequencer* storySequencer = nullptr;

// GotchiSim - simulation reducer (C-core)
GotchiSim* gotchiSim = nullptr;

// DriversController - Box C: affection/mercy driver computation
DriversController* driversController = nullptr;

// SaveManager - handles save/load/delete operations
SaveManager saveManager;

// SaveWiring - subscribes to checkpoint events for autosave
SaveWiring* saveWiring = nullptr;

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

SceneManager* sceneManager = nullptr;
DialogBox*    dialog       = nullptr;
RenderTexture2D gameTarget;

int         dialogIndex = 0;
std::string lastScene   = "title";

static Rectangle GetLetterboxRect() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float scaleX = (float)sw / (float)GAME_W;
    float scaleY = (float)sh / (float)GAME_H;
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    if (IsIntegerScalingEnabled()) {
        // Snap down to the nearest whole multiple so every source pixel maps
        // to an even NxN block -- avoids uneven/aliased pixel-art scaling.
        float snapped = floorf(scale);
        scale = (snapped >= 1.0f) ? snapped : scale;  // Don't snap to 0 on tiny windows
    }

    float drawW  = GAME_W * scale;
    float drawH  = GAME_H * scale;
    return { (sw - drawW) * 0.5f, (sh - drawH) * 0.5f, drawW, drawH };
}

// GetMousePosition() reports raw OS-window pixels, but every button/click
// check in the game (Button, SceneActor clickables, PauseMenuOverlay,
// ControlsOverlay) assumes game-space (0..GAME_W, 0..GAME_H) coordinates.
// Without this correction, mouse hit-testing is offset by the letterbox's
// scale and centering whenever the window isn't exactly GAME_W x GAME_H --
// call once per frame, before any scene/UI update.
static void SyncMouseToGameSpace() {
    Rectangle dest = GetLetterboxRect();
    if (dest.width <= 0.0f || dest.height <= 0.0f) return;
    float scale = dest.width / (float)GAME_W;
    SetMouseOffset((int)-dest.x, (int)-dest.y);
    SetMouseScale(1.0f / scale, 1.0f / scale);
}

void UpdateDrawFrame() {
    SyncMouseToGameSpace();

    float dt = GetFrameTime();
    std::string currentScene = sceneManager->getCurrentSceneName();

    if (currentScene != lastScene) {
        dialogIndex = 0;
        // The Tom world-scenes (pizza_parlor/apartment/etc.) drive the shared
        // dialog box themselves via triggerScenario()/DialogBox::show() --
        // only force-hide it once, on the frame we transition in, so a stale
        // line doesn't linger. Do NOT hide it every frame here or
        // it fights with the scene's own show() calls and the dialog can
        // never stay visible (softlock: it disappears the instant it shows).
        if (currentScene == "pizza_parlor" || currentScene == "apartment" || currentScene == "therapist_office" ||
            currentScene == "office" || currentScene == "school" || currentScene == "scene_select" ||
            currentScene == "gotchi" || currentScene == "title") {
            dialog->hide();
        }
        lastScene = currentScene;

        // Autosave on scene transition (if active slot is set)
        if (saveWiring) {
            saveWiring->onSceneTransition();
        }
    }

    // These standalone debug/demo scenes never use the shared dialog box at
    // all, so it's safe to keep it hidden every frame unconditionally.
    if (currentScene == "input_test" || currentScene == "hexboard" || currentScene == "sprite_test" ||
        currentScene == "gotchi" || currentScene == "title") {
        dialog->hide();
    }

    if (IsKeyPressed(KEY_THREE) && currentScene != "input_test")
        sceneManager->switchScene("input_test");
    if (IsKeyPressed(KEY_FOUR) && currentScene != "sprite_test")
        sceneManager->switchScene("sprite_test");
    if (IsKeyPressed(KEY_FIVE) && currentScene != "hexboard")
        sceneManager->switchScene("hexboard");
    if (IsKeyPressed(KEY_NINE) && currentScene != "title")
        sceneManager->switchScene("title");
    if (IsKeyPressed(KEY_SEVEN) && currentScene != "scene_select")
        sceneManager->switchScene("scene_select");

    // Pause key (0) is reserved for future use - no scene supports it yet

    // Debug trigger for the apartment's/etc scripted story beat (normally
    // invoked by the tomagotchi/stat side, not by a raw key). Pizza Parlor
    // is now driven entirely by StorySequencer -- see startStep()/update()'s
    // Phase::EnteringStep -- so it no longer needs this manual E-press hook.
    // Gated to scenes entered via the scene_select debug hub (key 7) --
    // Scene::getEntrySceneName() is stamped by SceneManager on every switch
    // -- so this never fires when a scene is reached through the real
    // sequencer/merge flow.
    Scene* curScene = sceneManager->getCurrentScene();
    bool fromDebugHub = curScene && curScene->getEntrySceneName() == "scene_select";
    if (fromDebugHub && IsKeyPressed(KEY_E) && currentScene == "apartment") {
        ApartmentScene* apartment = (ApartmentScene*)curScene;
        if (apartment && !apartment->isPlayingScenario()) apartment->triggerScenario(0);
    }
    if (fromDebugHub && IsKeyPressed(KEY_E) && currentScene == "therapist_office") {
        TherapistOfficeScene* office = (TherapistOfficeScene*)curScene;
        if (office && !office->isPlayingScenario()) office->triggerScenario(0);
    }
    if (fromDebugHub && IsKeyPressed(KEY_E) && currentScene == "office") {
        static int nextOfficeScenario = 0;
        OfficeScene* office = (OfficeScene*)curScene;
        if (office && !office->isPlayingScenario()) {
            office->triggerScenario(nextOfficeScenario);
            nextOfficeScenario = (nextOfficeScenario + 1) % 2;
        }
    }
    if (fromDebugHub && IsKeyPressed(KEY_E) && currentScene == "school") {
        SchoolScene* school = (SchoolScene*)curScene;
        if (school && !school->isPlayingScenario()) school->triggerScenario(0);
    }

    sceneManager->update(dt);
    if (mergeController) {
        mergeController->update(dt);
    }
    if (storySequencer) {
        storySequencer->update(dt);
    }
    if (gotchiSim) {
        gotchiSim->update(dt);
    }
    if (driversController) {
        driversController->update(dt);
    }

    dialog->update(dt);

    BeginTextureMode(gameTarget);
        ClearBackground(BLACK);
        sceneManager->draw();
        dialog->draw();
    EndTextureMode();

    Rectangle dest = GetLetterboxRect();
    Rectangle src  = { 0.0f, 0.0f, (float)GAME_W, -(float)GAME_H };

    BeginDrawing();
        ClearBackground(BLACK);
        DrawTexturePro(gameTarget.texture, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
    EndDrawing();
}

int main() {
#ifdef HEXA_SHOT_TOOL
    // Dev tooling only, compiled in solely when built with -DHEXA_SHOT_TOOL
    // (see tools/screenshot.sh). Starts on the scene named by the HEXA_SHOT
    // env var, renders a few frames, saves a PNG, and exits. Absent entirely
    // from normal builds.
    const char* shotScene = getenv("HEXA_SHOT");
    const char* shotFramesEnv = getenv("HEXA_SHOT_FRAMES");
    int shotWaitFrames = shotFramesEnv ? atoi(shotFramesEnv) : 30;
    if (shotWaitFrames <= 0) shotWaitFrames = 30;
#endif

#if defined(PLATFORM_WEB)
    // In web builds, assets.rres is embedded into the virtual filesystem
    // at the root directory. The AssetPack library uses rres which uses
    // standard C file I/O, so we need to set the pack file path explicitly.
    AssetPack::setPackFile("/assets.rres");
#endif

#if !defined(PLATFORM_WEB)
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
#endif
    InitWindow(720, 720, "2D Engine Demo");
    SetTargetFPS(60);

    gameTarget = LoadRenderTexture(GAME_W, GAME_H);
    SetTextureFilter(gameTarget.texture, TEXTURE_FILTER_POINT);

    dialog = new DialogBox(
        {(float)GAME_W / 2.0f, (float)GAME_H - 20.0f},
        680.0f, 160.0f
    );

    // Initialize the asset pack system
    AssetPack::setPackFile("assets.rres");

    sceneManager = new SceneManager();
    sceneManager->registerScene("hexboard", new HexViewScene());
    sceneManager->registerScene("input_test", new InputTestScene());
    GotchiScene* gotchiScene = new GotchiScene();
    sceneManager->registerScene("gotchi", gotchiScene);
    sceneManager->registerScene("sprite_test", new SpriteTestScene());
    sceneManager->registerScene("pizza_parlor", new PizzaParlorScene(dialog));
    sceneManager->registerScene("apartment", new ApartmentScene(dialog));
    sceneManager->registerScene("therapist_office", new TherapistOfficeScene(dialog));
    sceneManager->registerScene("office", new OfficeScene(dialog));
    sceneManager->registerScene("school", new SchoolScene(dialog));
    sceneManager->registerScene("model3d_test", new Model3DTestScene());
    sceneManager->registerScene("merge", new MergeScene());
    sceneManager->registerScene("title", new TitleScene());
    sceneManager->registerScene("scene_select", new SceneSelectScene(sceneManager));
    GotchiStatsScene* gotchiStatsScene = new GotchiStatsScene();
    sceneManager->registerScene("gotchi_stats", gotchiStatsScene);

    // Wire up shared GameState to scenes (vitals and mood are now shared)
    gotchiScene->setGameState(&globalGameState);
    gotchiStatsScene->setGameState(&globalGameState);

    // Wire up event bus for hexboard scene
    HexViewScene* hexViewScene = static_cast<HexViewScene*>(sceneManager->getScene("hexboard"));
    if (hexViewScene) {
        hexViewScene->setEventBus(&globalEventBus);
    }

    // Initialize save directory
    saveManager.initSaveDir();

    // Wire up the merge controller
    mergeController = new MergeController(globalEventBus, globalGameState, *sceneManager);
    gotchiScene->setEventBus(&globalEventBus);

    // Wire up the story sequencer
    storySequencer = new StorySequencer(globalEventBus, globalGameState, *sceneManager, *dialog);

    // Wire up the sim reducer (C-core)
    gotchiSim = new GotchiSim(globalEventBus, globalGameState);

    // Wire up the drivers controller (Box C: affection/mercy computation)
    driversController = new DriversController(globalEventBus, globalGameState);

    // Wire up SaveWiring for autosave on checkpoints
    saveWiring = new SaveWiring(saveManager, globalEventBus);
    saveWiring->setGameState(&globalGameState);

#ifdef HEXA_SHOT_TOOL
    sceneManager->switchSceneImmediate(
        (shotScene && shotScene[0]) ? shotScene : "title");
#else
    sceneManager->switchSceneImmediate("title");
#endif

    dialog->setAnchor("bottom");
    dialog->setCharacterRevealSpeed(45.0f);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
#ifdef HEXA_SHOT_TOOL
    int shotFrame = 0;
#endif
    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE) && !exitRequested) {
        UpdateDrawFrame();
#ifdef HEXA_SHOT_TOOL
        if (shotScene && shotScene[0]) {
            // Let the scene settle a few frames, then capture and quit.
            if (++shotFrame == shotWaitFrames) {
                TakeScreenshot("shot.png");
                break;
            }
        }
#endif
    }

    // Shutdown autosave (if active slot is set)
    if (saveManager.activeSlot() >= 0) {
        saveManager.autosave(globalGameState);
        std::cout << "[Shutdown] Autosaved GameState to slot " << saveManager.activeSlot() << std::endl << std::flush;
    }

    UnloadRenderTexture(gameTarget);
    delete dialog;
    delete mergeController;
    delete storySequencer;
    delete gotchiSim;
    delete driversController;
    delete saveWiring;
    delete sceneManager;
    CloseWindow();
#endif
    return 0;
}
