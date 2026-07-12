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
#include "game/SplashScreenScene.hpp"
#include "game/GotchiStatsScene.hpp"
#include "game/MergeController.hpp"
#include "game/TutorialController.hpp"
#include "game/StorySequencer.hpp"
#include "game/DeathSequencer.hpp"
#include "game/SkipSceneOverlay.hpp"
#include "game/PauseMenuOverlay.hpp"
#include "Button.hpp"
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

// Global skip-scene button/confirm overlay -- see comment at its use site in
// UpdateDrawFrame() for which scenes it's allowed to appear on.
SkipSceneOverlay* skipSceneOverlay = nullptr;

// Story-beat scenes reachable via StorySequencer's Sequence steps (see
// ScenarioDirector.cpp) -- the only scenes a "skip this scene" button is
// allowed to act on. Deliberately excludes gotchi, hexboard, merge,
// toy_animation, death, ending (terminal/credits scene), title, options,
// credits, scene_select, and the debug/test scenes.
static bool IsSkippableStoryScene(const std::string& sceneName) {
    return sceneName == "office" || sceneName == "pizza_parlor" ||
           sceneName == "kids_visit" || sceneName == "therapist_office" ||
           sceneName == "school" || sceneName == "apartment";
}

// Global pause button/overlay for Tom's story-world scenes (GotchiScene and
// HexViewScene already have their own per-scene pause menus -- see those
// classes -- since they're self-contained and drive their own gameplay loop
// in-scene). The Tom scenes instead share ONE overlay driven from here,
// because their "pause-worthy" state (dialog typing/auto-advance,
// StorySequencer's scripted steps) lives outside any single Scene subclass,
// in main.cpp's own globals -- see the dialog->update()/storySequencer->
// update() gating below. Includes the ending scene (still has a
// player-controlled Tom actor) but not credits (terminal, no gameplay).
Button* pauseButton = nullptr;
PauseMenuOverlay* tomPauseMenu = nullptr;
bool tomWorldPaused = false;

// True while OPTIONS is showing as an overlay on top of a paused Tom-world
// scene. Deliberately NOT a real SceneManager::switchScene("options") --
// that would call cleanup()/init() on the story scene twice (once leaving,
// once coming back), and every story scene's cleanup() unconditionally
// resets activeScenario/lineIndex to "not playing" (see e.g.
// OfficeScene::cleanup()), which made StorySequencer's PlayingStep phase
// see isPlayingScenario()==false and treat it as "the step legitimately
// finished" -- skipping straight to the next story beat the instant the
// player came back from Options. Drawing/updating the shared OptionsScene
// instance directly as an overlay (like tomPauseMenu/skipSceneOverlay
// already are) means currentSceneName never changes, so nothing about the
// story scene or StorySequencer is touched at all.
bool tomOptionsOpen = false;

// True for the remainder of the frame in which a click was consumed by the
// pause button/menu OR the skip-scene button/confirm modal -- checked by
// story scenes before treating a mouse-press as "advance the dialogue line"
// (see IsPauseUiClaimingClick() below). Needed because the click-to-advance
// check reads raylib's raw one-frame IsMouseButtonPressed() edge, which
// SceneInputHandler::clearAllInputs() cannot suppress.
bool g_pauseUiClaimedClick = false;
bool IsPauseUiClaimingClick() { return g_pauseUiClaimedClick; }

static bool IsTomWorldScene(const std::string& sceneName) {
    return sceneName == "office" || sceneName == "pizza_parlor" ||
           sceneName == "kids_visit" || sceneName == "therapist_office" ||
           sceneName == "school" || sceneName == "apartment" ||
           sceneName == "ending";
}

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

    // Debug trigger for a story scene's scripted scenario(s) (normally
    // invoked by the tomagotchi/stat side, not by a raw key). Gated to
    // scenes entered via the scene_select debug hub (key 7) --
    // Scene::getEntrySceneName() is stamped by SceneManager on every switch
    // -- so none of this ever fires when a scene is reached through the real
    // sequencer/merge flow.
    //
    // Z: cycle to the next scenario (wrapping) and play it from the top --
    // generic over every story scene via Scene::getScenarioCount()/
    // triggerScenario(), so a new scene gets this for free without another
    // per-scene-type block here (previously only 4 of 7 story scenes had a
    // hardcoded, scene-specific KEY_E single-scenario trigger).
    // X: toggle wide-view off/on -- each story scene's own update() already
    // gates its ambient idle camera drift and scripted per-line
    // followPosition() behind `!getCamera()->isWideViewEnabled()`, so
    // turning wide view OFF hands the camera back to the exact same logic
    // that runs during real gameplay, letting a 3D effect placed via the
    // I/J/K/L/U/O dial-in controls be checked framed as it would actually be
    // seen in-game, not just in the zoomed-out debug view.
    Scene* curScene = sceneManager->getCurrentScene();
    bool fromDebugHub = curScene && curScene->getEntrySceneName() == "scene_select";
    if (fromDebugHub && curScene) {
        static int debugScenarioIndex = 0;
        int count = curScene->getScenarioCount();
        if (count > 0 && IsKeyPressed(KEY_Z) && !curScene->isPlayingScenario()) {
            curScene->triggerScenario(debugScenarioIndex);
            debugScenarioIndex = (debugScenarioIndex + 1) % count;
        }
        if (curScene->getCamera() && IsKeyPressed(KEY_X)) {
            curScene->getCamera()->toggleWideView();
        }
    }

    // Reflect the global Tom-world pause flag onto the current scene before
    // it updates, so the isPaused() early-return each story scene's update()
    // added actually triggers (ambient sway/camera-focus/end-fade logic).
    bool tomWorldSceneEarly = IsTomWorldScene(currentScene);
    if (tomWorldSceneEarly) {
        if (Scene* curSceneForPause = sceneManager->getCurrentScene()) {
            curSceneForPause->setPaused(tomWorldPaused);
        }
    }

    // Update the pause button/menu AND the skip-scene button/confirm modal
    // BEFORE the scene itself updates, so a click on PAUSE/RESUME/OPTIONS or
    // SKIP/YES/NO is consumed here first. This can't be done via
    // clearAllInputs() -- the story scenes' click-to-advance check reads
    // ih->isMouseButtonPressed(), which forwards straight to raylib's own
    // IsMouseButtonPressed() one-frame edge flag, not through
    // SceneInputHandler's own (unrelated) tracked state, so it can't be
    // cleared out from under them. Instead g_pauseUiClaimedClick is a
    // frame-scoped flag the story scenes check (see IsPauseUiClaimingClick()
    // below) before treating a mouse-press as "advance the dialogue line".
    Scene* curSceneForUi = sceneManager->getCurrentScene();
    SceneInputHandler* ihForUi = curSceneForUi ? curSceneForUi->getInputHandler() : nullptr;

    if (tomWorldSceneEarly) {
        if (tomOptionsOpen) {
            // Options is on top -- it owns input this frame, not the pause
            // menu underneath it or the story scene. OptionsScene is a real
            // Scene with its own SceneInputHandler (reads raylib's live
            // mouse/key state directly, independent of the story scene's
            // handler), so its own update() is self-sufficient here.
            if (auto* options = static_cast<OptionsScene*>(sceneManager->getScene("options"))) {
                options->update(dt);
            }
        } else {
            if (pauseButton) pauseButton->update(ihForUi, dt);
            if (tomWorldPaused && tomPauseMenu) tomPauseMenu->update(dt, ihForUi);
        }
    }

    // Only a regular story beat in the sequencer is skippable -- not
    // gotchi/hexboard, merge/toy_animation transitions, the terminal
    // ending/credits scene, or any menu/debug scene. isPlayingStep() also
    // excludes the merge-in/merge-out transition windows, where there's no
    // active step to skip. Hidden while paused so it can't be clicked out
    // from under the pause overlay.
    bool skipVisible = !tomWorldPaused && IsSkippableStoryScene(currentScene) &&
                        storySequencer && storySequencer->isPlayingStep();
    if (skipSceneOverlay) {
        skipSceneOverlay->update(dt, ihForUi, skipVisible);
    }

    // Any click this frame belongs to a UI overlay, never to the scene behind
    // it, whenever: the pause button itself is hovered (covers opening
    // pause), the pause menu is already open (every click while paused is
    // either RESUME/OPTIONS or a miss-click on the dimmed backdrop), the skip
    // button is hovered, or the skip confirm modal is open.
    g_pauseUiClaimedClick =
        (tomWorldSceneEarly && ((pauseButton && pauseButton->isHovered()) || tomWorldPaused)) ||
        (skipSceneOverlay && skipVisible && skipSceneOverlay->isInteractive());

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

    // Leaving a Tom-world scene (via back button, tutorial, etc) forces the
    // pause back off so it can't get stuck open behind a scene that has no
    // pause button of its own to close it with. (pauseButton/tomPauseMenu and
    // skipSceneOverlay were already updated earlier, before sceneManager
    // updated the scene itself -- see the comment at that block.)
    if (!IsTomWorldScene(currentScene)) {
        tomWorldPaused = false;
        tomOptionsOpen = false;
    }

    if (mergeController) {
        mergeController->update(dt);
    }
    // Dialog typing/auto-advance and the story sequencer's scripted steps
    // both live outside any Scene subclass (shared globals), so pausing a
    // Tom-world scene has to skip these here too -- gating just the scene's
    // own update() (see the isPaused() early-returns added to each story
    // scene) isn't enough on its own.
    if (!tomWorldPaused) {
        if (storySequencer) {
            storySequencer->update(dt);
            // Handle skip action - skip current story step (for testing)
            // Key S works in any scene when story sequencer is active
            if (IsKeyPressed(KEY_S)) {
                storySequencer->skipCurrentStep();
            }
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
        // Also frozen during the tutorial, the sleep-collapse gate, and while
        // the current scene's own pause menu is open (Scene::isPaused()) --
        // otherwise vitals kept draining behind the pause overlay.
        bool onCareSide = (currentScene == "gotchi" || currentScene == "hexboard");
        bool tutorialActive = tutorialController && tutorialController->isActive();
        Scene* activeScene = sceneManager->getCurrentScene();
        bool scenePaused = activeScene && activeScene->isPaused();
        globalGameState.statsFrozen = !onCareSide || tutorialActive || globalGameState.sleepCollapsed || scenePaused;
    }
    if (gotchiSim) {
        gotchiSim->update(dt);
    }
    if (driversController) {
        driversController->update(dt);
    }

    if (!tomWorldPaused) {
        dialog->update(dt);
    }

    BeginTextureMode(gameTarget);
        ClearBackground(BLACK);
        sceneManager->draw();
        // Options fully covers the screen with its own opaque background
        // (see below) -- don't draw the dialog box underneath it or it peeks
        // through/overlaps the options UI.
        if (!tomOptionsOpen) {
            dialog->draw();
        }
        if (skipSceneOverlay) {
            const std::string& skipScene = sceneManager->getCurrentSceneName();
            bool skipVisible = !tomWorldPaused && IsSkippableStoryScene(skipScene) &&
                                storySequencer && storySequencer->isPlayingStep();
            skipSceneOverlay->draw(skipVisible);
        }
        if (pauseButton && tomWorldSceneEarly) {
            pauseButton->draw();
        }
        if (tomWorldPaused) {
            if (tomOptionsOpen) {
                // Options fully covers the screen with its own opaque
                // background (see OptionsScene's Scene(...) bg color), so it
                // replaces the pause menu rather than drawing on top of it.
                if (auto* options = static_cast<OptionsScene*>(sceneManager->getScene("options"))) {
                    options->draw();
                }
            } else if (tomPauseMenu) {
                tomPauseMenu->draw();
            }
        }
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
    SetExitKey(KEY_NULL);
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
    sceneManager->registerScene("splash", new SplashScreenScene(sceneManager));
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

    // Wire up the skip-scene overlay
    skipSceneOverlay = new SkipSceneOverlay();
    skipSceneOverlay->onConfirmSkip = []() {
        // Hide the skipped scene's in-progress line first -- skipCurrentStep()
        // jumps straight to the next step (or the merge-out transition)
        // without finishing the dialog naturally, so without this the old
        // line/portrait lingers on screen over whatever comes next.
        if (dialog) dialog->hide();
        if (storySequencer) storySequencer->skipCurrentStep();
    };

    // Wire up the global pause button/overlay for Tom's story-world scenes
    // (GotchiScene/HexViewScene have their own -- see IsTomWorldScene()'s
    // comment above for why this one has to live here instead).
    static const float PAUSE_BTN_W = 80.0f, PAUSE_BTN_H = 32.0f, PAUSE_BTN_PAD = 12.0f;
    pauseButton = new Button({(float)GAME_W - PAUSE_BTN_W - PAUSE_BTN_PAD, PAUSE_BTN_PAD},
                             PAUSE_BTN_W, PAUSE_BTN_H, "PAUSE");
    pauseButton->setAnchor("top-left");
    pauseButton->setFontSize(16);
    pauseButton->setBackgroundColor({60, 60, 100, 220});
    pauseButton->setHoverColor({100, 100, 160, 240});
    pauseButton->setBorderColor({150, 150, 200, 255});
    pauseButton->setOnClick([]() { tomWorldPaused = !tomWorldPaused; });

    tomPauseMenu = new PauseMenuOverlay();
    tomPauseMenu->onResume = []() {
        tomWorldPaused = false;
        // The click that closed the menu is still "pressed" on this same
        // frame -- without clearing it, the scene's own click-to-advance
        // dialogue check (ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT)) reads
        // that same click as a request to advance the current dialogue line
        // the instant the game unpauses. Matches GotchiScene/HexViewScene's
        // own onResume, which clears input for the same reason.
        if (Scene* s = sceneManager->getCurrentScene()) {
            if (SceneInputHandler* ih = s->getInputHandler()) ih->clearAllInputs();
        }
    };
    tomPauseMenu->onOptionsSelected = []() {
        // Opens OPTIONS as an overlay ON TOP of the still-current, still-
        // paused story scene -- see tomOptionsOpen's comment above for why
        // this deliberately does NOT go through sceneManager->switchScene().
        if (auto* options = static_cast<OptionsScene*>(sceneManager->getScene("options"))) {
            options->init();  // rebuilds buttons + refreshes volume/speed labels
            options->onBackOverride = []() { tomOptionsOpen = false; };
        }
        tomOptionsOpen = true;
    };

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
    // Screenshot tool jumps straight to its target scene (or title) -- never
    // through the splash sequence.
    sceneManager->switchSceneImmediate(
        (shotScene && shotScene[0]) ? shotScene : "title");
#else
    // Normal boot: the logo splash sequence, which then switches to the title.
    sceneManager->switchSceneImmediate("splash");
#endif

    dialog->setAnchor("bottom");
    dialog->setCharacterRevealSpeed(45.0f);

#if defined(PLATFORM_WEB)
    emscripten_set_main_loop(UpdateDrawFrame, 0, 1);
#else
#ifdef HEXA_SHOT_TOOL
    int shotFrame = 0;
#endif
    while (!WindowShouldClose() && !exitRequested) {
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
    delete skipSceneOverlay;
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
