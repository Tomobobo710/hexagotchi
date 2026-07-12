#include <cstdio>
#include <cstdarg>
#include <iostream>
#include <fstream>
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
#include "game/KidsVisitScene.hpp"
#include "game/EndingScene.hpp"
#include "game/CreditsScene.hpp"
#include "game/SchoolScene.hpp"
#include "game/Model3DTestScene.hpp"
#include "game/MergeScene.hpp"
#include "game/ToyAnimationScene.hpp"
#include "game/DeathScene.hpp"
#include "game/SceneSelectScene.hpp"
#include "game/OptionsScene.hpp"
#include "game/TitleScene.hpp"
#include "game/GotchiStatsScene.hpp"
#include "game/MergeController.hpp"
#include "game/TutorialController.hpp"
#include "game/StorySequencer.hpp"
#include "game/DeathSequencer.hpp"
#include "game/DialogSequences.hpp"
#include "engine/GameState.h"
#include "engine/SaveManager.h"
#include "engine/SaveWiring.h"
#include "engine/AudioManager.hpp"
#include "engine/Config.hpp"
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

// DeathSequencer - created in main() and used in UpdateDrawFrame
DeathSequencer* deathSequencer = nullptr;

// TutorialController - created in main() and used in UpdateDrawFrame
TutorialController* tutorialController = nullptr;

// GotchiSim - simulation reducer (C-core)
GotchiSim* gotchiSim = nullptr;

// DriversController - Box C: affection/mercy driver computation
DriversController* driversController = nullptr;

// SaveManager - DISABLED: Save system shut off for game jam
// SaveManager saveManager;

// SaveWiring - DISABLED: Save system shut off for game jam
// SaveWiring* saveWiring = nullptr;

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
    #include <emscripten/html5.h>

// Pause music when the browser tab is hidden and resume when it's shown again.
// Fires off the Page Visibility API, so it runs even while the game loop is
// parked by the browser (which is exactly when the music stream would otherwise
// underrun and glitch). Registered once, after AudioManager::init().
static EM_BOOL OnVisibilityChange(int, const EmscriptenVisibilityChangeEvent* e, void*) {
    if (e->hidden) AudioManager::Get().pauseMusic();
    else           AudioManager::Get().resumeMusic();
    return EM_TRUE;
}
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

    // Clamp dt so a backgrounded/throttled tab can't deliver a monster delta.
    // On web the loop is requestAnimationFrame-driven (emscripten fps=0), which
    // the browser pauses while the tab is unfocused; on return, GetFrameTime()
    // can report the entire away-duration as one frame. An unbounded dt makes
    // timers fire thousands of times, animations/physics integrate to garbage,
    // and state corrupt ("game breaks on return"). Capping at 100ms means the
    // worst case is a single slightly-long frame, never a catastrophic one.
    float dt = GetFrameTime();
    if (dt > 0.1f) dt = 0.1f;
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

        // Autosave on scene transition - DISABLED for game jam
        // if (saveWiring) {
        //     saveWiring->onSceneTransition();
        // }
    }

    // These standalone debug/demo scenes never use the shared dialog box at
    // all, so it's safe to keep it hidden every frame unconditionally.
    if (currentScene == "input_test" || currentScene == "hexboard" || currentScene == "sprite_test" ||
        currentScene == "gotchi" || currentScene == "title" || currentScene == "death") {
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

    // --- Global music: pick a "world" from the current scene name and let the
    // AudioManager own the single music track. setMusicWorld() is idempotent,
    // so calling it every frame just keeps the right track playing and only
    // swaps when the world actually changes. Silence (None) on menus/death/etc.
    // Re-read the scene name here (not the top-of-frame `currentScene`) because
    // sceneManager->update() may have just flipped scenes.
    {
        const std::string& s = sceneManager->getCurrentSceneName();
        AudioManager::MusicWorld world = AudioManager::MusicWorld::None;
        if (s == "gotchi" || s == "hexboard") {
            world = AudioManager::MusicWorld::Gotchi;
        } else if (s == "merge" || s == "toy_animation") {
            // toy_animation shares the merge theme -- both are "crossing between
            // worlds" transitions (the toy intro zooms into the gotchi's world).
            world = AudioManager::MusicWorld::Merge;
        } else if (s == "office" || s == "apartment" || s == "pizza_parlor" ||
                   s == "therapist_office" || s == "school" || s == "kids_visit" ||
                   s == "ending") {
            world = AudioManager::MusicWorld::RealWorld;
        }
        // Everything else (title, options, credits, death, debug/test scenes)
        // stays silent.
        AudioManager::Get().setMusicWorld(world);
        AudioManager::Get().updateMusic();
    }

    if (mergeController) {
        mergeController->update(dt);
    }
    if (storySequencer) {
        storySequencer->update(dt);
        // Handle skip action - skip current story step (for testing)
        // Key S works in any scene when story sequencer is active
        if (IsKeyPressed(KEY_S)) {
            storySequencer->skipCurrentStep();
        }
    }
    if (deathSequencer) {
        deathSequencer->update(dt);
    }
    // Freeze vitals/mood ticking here, once, regardless of which scene is
    // currently active -- GotchiScene/HexViewScene used to each set this
    // themselves in their own update(), which left it unset (stale/false)
    // during any other scene visited while the tutorial is active (e.g. the
    // toy_animation intro on the way into GotchiScene), letting the sim run
    // unguarded during that window. Also stays frozen on the title screen
    // itself and before the very first tutorial start -- gotchiSim ticks
    // from app boot, well before the player has even clicked START GAME, so
    // without this check stats were already visibly non-full by the time
    // the tutorial's first line appeared.
    {
        // The sim (vitals drain, collapse/death) may ONLY tick while the player
        // is actually on the care side -- the gotchi home screen or the
        // hexboard. Everywhere else (title, options, credits, death,
        // toy_animation intro, merge/story scenes, debug scenes) it's frozen.
        // This is a WHITELIST, not a blacklist: a blacklist of "pre-game"
        // scenes kept missing new scenes (options/credits/death), which let the
        // sim run -- and you could literally die sitting on the main menu.
        // Also frozen during the tutorial and the sleep-collapse gate.
        bool onCareSide = (currentScene == "gotchi" || currentScene == "hexboard");
        bool tutorialActive = tutorialController && tutorialController->isActive();
        globalGameState.statsFrozen = !onCareSide || tutorialActive || globalGameState.sleepCollapsed;
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
    // Silence ALL console output. Three separate channels emit logs:
    //   1. raylib's TraceLog (RRES/TEXTURE/SHADER/IMAGE spam + the game's own
    //      TraceLog calls like GOTCHI_FRAME/HEXCAM/ASSETPACK) -> LOG_NONE.
    //   2. C stdio printf/fprintf(stderr) -> redirect stdout+stderr to null.
    //   3. C++ std::cout/cerr/clog (StorySequencer DEBUG_LOG, care-action
    //      debug, etc.) -> point their stream buffers at a null sink.
    // Must be first, before any resource loading or InitWindow.
#ifdef HEXA_DEBUG_LOG
    // Debug build: keep ALL logging on and route raylib's TraceLog to a file
    // next to the exe (hexa_log.txt) so a -mwindows (no-console) build can still
    // be diagnosed. Build with: build-alt.sh debug  (defines HEXA_DEBUG_LOG).
    SetTraceLogLevel(LOG_ALL);
    {
        static FILE* logFile = fopen("hexa_log.txt", "w");
        if (logFile) {
            // Mirror raylib TraceLog into the file.
            SetTraceLogCallback([](int logLevel, const char* text, va_list args) {
                static FILE* f = fopen("hexa_log.txt", "a");
                if (f) { vfprintf(f, text, args); fputc('\n', f); fflush(f); }
                (void)logLevel;
            });
        }
    }
#else
    // Release: silence ALL console output. Three channels emit logs:
    //   1. raylib's TraceLog -> LOG_NONE.
    //   2. C stdio printf/fprintf(stderr) -> redirect stdout+stderr to null.
    //   3. C++ std::cout/cerr/clog -> point their buffers at a null sink.
    // Must be first, before any resource loading or InitWindow.
    SetTraceLogLevel(LOG_NONE);
#if defined(_WIN32)
    freopen("NUL", "w", stdout);
    freopen("NUL", "w", stderr);
#else
    freopen("/dev/null", "w", stdout);
    freopen("/dev/null", "w", stderr);
#endif
    static std::ofstream nullSink;   // never opened -> a silently-failing sink
    std::cout.rdbuf(nullSink.rdbuf());
    std::cerr.rdbuf(nullSink.rdbuf());
    std::clog.rdbuf(nullSink.rdbuf());
#endif // HEXA_DEBUG_LOG

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
    InitWindow(720, 720, "Hexagotchi");
    SetTargetFPS(60);

    gameTarget = LoadRenderTexture(GAME_W, GAME_H);
    SetTextureFilter(gameTarget.texture, TEXTURE_FILTER_POINT);

    dialog = new DialogBox(
        {(float)GAME_W / 2.0f, (float)GAME_H - 20.0f},
        680.0f, 160.0f
    );

    // Initialize the asset pack system
    AssetPack::setPackFile("assets.rres");

    // Audio: opens the device and loads shared SFX (UI click) from the pack.
    // Must run after setPackFile so the click clip resolves.
    AudioManager::Get().init();

    // Load persisted user settings (volumes, dialog speed, tutorial_seen) from
    // localStorage (web) / settings.ini (desktop), applying them to the global
    // settings + globalGameState. Absent config -> defaults. Runs before scenes
    // register so the first frame already reflects saved prefs.
    Config::Load();

#if defined(PLATFORM_WEB)
    // Pause/resume music when the browser tab is hidden/shown so the streaming
    // buffer doesn't underrun and glitch while the game loop is parked.
    emscripten_set_visibilitychange_callback(nullptr, EM_FALSE, OnVisibilityChange);
#endif

    sceneManager = new SceneManager();
    sceneManager->registerScene("hexboard", new HexViewScene());
    sceneManager->registerScene("input_test", new InputTestScene());
    GotchiScene* gotchiScene = new GotchiScene();
    sceneManager->registerScene("gotchi", gotchiScene);
    sceneManager->registerScene("sprite_test", new SpriteTestScene());
    sceneManager->registerScene("pizza_parlor", new PizzaParlorScene(dialog));
    sceneManager->registerScene("apartment", new ApartmentScene(dialog));
    sceneManager->registerScene("kids_visit", new KidsVisitScene(dialog));
    sceneManager->registerScene("ending", new EndingScene(dialog));
    sceneManager->registerScene("credits", new CreditsScene());
    sceneManager->registerScene("therapist_office", new TherapistOfficeScene(dialog));
    sceneManager->registerScene("office", new OfficeScene(dialog));
    sceneManager->registerScene("school", new SchoolScene(dialog));
    sceneManager->registerScene("model3d_test", new Model3DTestScene());
    sceneManager->registerScene("merge", new MergeScene());
    sceneManager->registerScene("toy_animation", new ToyAnimationScene());
    sceneManager->registerScene("death", new DeathScene());
    sceneManager->registerScene("title", new TitleScene());
    sceneManager->registerScene("scene_select", new SceneSelectScene(sceneManager));
    sceneManager->registerScene("options", new OptionsScene(sceneManager));
    GotchiStatsScene* gotchiStatsScene = new GotchiStatsScene();
    sceneManager->registerScene("gotchi_stats", gotchiStatsScene);

    // Wire up shared GameState to scenes (vitals and mood are now shared)
    gotchiScene->setGameState(&globalGameState);
    gotchiStatsScene->setGameState(&globalGameState);

    // OfficeScene reads live vitals for Loraine's "reports" beat in Scenario A.
    OfficeScene* officeScene = static_cast<OfficeScene*>(sceneManager->getScene("office"));
    if (officeScene) officeScene->setGameState(&globalGameState);

    // Wire up event bus for hexboard scene
    HexViewScene* hexViewScene = static_cast<HexViewScene*>(sceneManager->getScene("hexboard"));
    if (hexViewScene) {
        hexViewScene->setEventBus(&globalEventBus);
        hexViewScene->setGameState(&globalGameState);  // Share vitals and mood
    }

    // Initialize save directory - DISABLED for game jam
    // saveManager.initSaveDir();

    // Wire up the merge controller
    mergeController = new MergeController(globalEventBus, globalGameState, *sceneManager);
    gotchiScene->setEventBus(&globalEventBus);
    gotchiScene->setMergeController(mergeController);

    // Wire up the tutorial controller
    tutorialController = new TutorialController(globalGameState);
    gotchiScene->setTutorialController(tutorialController);
    if (hexViewScene) hexViewScene->setTutorialController(tutorialController);

    // Wire up the story sequencer
    storySequencer = new StorySequencer(globalEventBus, globalGameState, *sceneManager, *dialog);

    // Wire up the death sequencer
    deathSequencer = new DeathSequencer(globalEventBus, globalGameState, *sceneManager);

    // Wire up the sim reducer (C-core)
    gotchiSim = new GotchiSim(globalEventBus, globalGameState);

    // Wire up the drivers controller (Box C: affection/mercy computation)
    driversController = new DriversController(globalEventBus, globalGameState);

    // Wire up SaveWiring for autosave on checkpoints - DISABLED for game jam
    // saveWiring = new SaveWiring(saveManager, globalEventBus);
    // saveWiring->setGameState(&globalGameState);

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

    // Shutdown autosave - DISABLED for game jam
    // if (saveManager.activeSlot() >= 0) {
    //     saveManager.autosave(globalGameState);
    //     std::cout << "[Shutdown] Autosaved GameState to slot " << saveManager.activeSlot() << std::endl << std::flush;
    // }

    UnloadRenderTexture(gameTarget);
    delete dialog;
    delete mergeController;
    delete tutorialController;
    delete storySequencer;
    delete deathSequencer;
    delete gotchiSim;
    delete driversController;
    // delete saveWiring;  // DISABLED: Save system shut off for game jam
    delete sceneManager;
    AudioManager::Get().shutdown();
    CloseWindow();
#endif
    return 0;
}
