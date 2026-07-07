#include "SceneManager.hpp"
#include "DialogBox.hpp"
#include "GameConstants.hpp"
#include "game/GameScene.hpp"
#include "game/BossScene.hpp"
#include "game/InputTestScene.hpp"
#include "game/HexViewScene.hpp"
#include "game/SpriteTestScene.hpp"
#include "game/GotchiScene.hpp"
#include "game/DialogSequences.hpp"
#include <string>
#include <vector>
#include <cstdlib>
#include <cmath>

// Global exit request flag
bool exitRequested = false;

#if defined(PLATFORM_WEB)
    #include <emscripten/emscripten.h>
#endif

SceneManager* sceneManager = nullptr;
DialogBox*    dialog       = nullptr;
RenderTexture2D gameTarget;

std::vector<DialogEntry> gameDialogs;
std::vector<DialogEntry> bossDialogs;
int         dialogIndex = 0;
std::string lastScene   = "game";

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

void showDialog(std::vector<DialogEntry>& seq, int idx) {
    if (idx < (int)seq.size()) {
        auto& d = seq[idx];
        dialog->setSpeakerName(d.speaker);
        dialog->setSpeakerColor(d.speakerColor);
        dialog->setPortraitColor(d.portraitColor);
        dialog->setText(d.text);
        dialog->show();
    }
}

void UpdateDrawFrame() {
    SyncMouseToGameSpace();

    float dt = GetFrameTime();
    std::string currentScene = sceneManager->getCurrentSceneName();

    if (currentScene != lastScene) {
        dialogIndex = 0;
        if (currentScene == "game") showDialog(gameDialogs, 0);
        if (currentScene == "boss") showDialog(bossDialogs, 0);
        lastScene = currentScene;
    }

    // Hide dialog on input test, hexboard, and gotchi scenes
    if (currentScene == "input_test" || currentScene == "hexboard" || currentScene == "sprite_test" || currentScene == "gotchi") {
        dialog->hide();
    }

    if (IsKeyPressed(KEY_ONE) && currentScene != "game")
        sceneManager->switchScene("game", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_TWO) && currentScene != "boss")
        sceneManager->switchScene("boss", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_THREE) && currentScene != "input_test")
        sceneManager->switchScene("input_test", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_FOUR) && currentScene != "sprite_test")
        sceneManager->switchScene("sprite_test", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_FIVE) && currentScene != "hexboard")
        sceneManager->switchScene("hexboard", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_EIGHT) && currentScene != "gotchi")
        sceneManager->switchScene("gotchi", TransitionEffect::FADE, 0.5f);

    if (IsKeyPressed(KEY_SPACE) && dialog->isVisible()) {
        if (!dialog->isFinished()) {
            dialog->skipCharacterReveal();
        } else {
            dialogIndex++;
            auto& seq = (currentScene == "boss") ? bossDialogs : gameDialogs;
            if (dialogIndex < (int)seq.size()) showDialog(seq, dialogIndex);
            else dialog->hide();
        }
    }

    if (IsKeyPressed(KEY_H)) {
        dialogIndex = 0;
        auto& seq = (currentScene == "boss") ? bossDialogs : gameDialogs;
        showDialog(seq, 0);
    }

    // Toggle pause menu with 0 key (only in game and boss scenes)
    if (IsKeyPressed(KEY_ZERO) && (currentScene == "game" || currentScene == "boss")) {
        Scene* currentSceneObj = sceneManager->getCurrentScene();
        if (currentSceneObj) {
            currentSceneObj->togglePause();
        }
    }

    sceneManager->update(dt);
    dialog->update(dt);

    BeginTextureMode(gameTarget);
        ClearBackground(BLACK);
        sceneManager->draw();
        dialog->draw();
        DrawRectangle(0, 0, GAME_W, 32, {0, 0, 0, 160});
        if (currentScene == "hexboard") {
            DrawText("HEXBOARD", 14, 8, 18, {180, 180, 255, 255});
            DrawText("1: World  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  ESC: Exit", GAME_W - 290, 8, 12, {140, 140, 180, 255});
        } else if (currentScene == "input_test") {
            DrawText("INPUT TEST", 14, 8, 18, {180, 180, 255, 255});
            DrawText("1: World  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  ESC: Exit", GAME_W - 290, 8, 12, {140, 140, 180, 255});
        } else if (currentScene == "sprite_test") {
            // SpriteTestScene draws its own "SPRITE TEST" title, skip the overlay title here.
            DrawText("1: World  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  ESC: Exit", GAME_W - 290, 8, 12, {140, 140, 180, 255});
        } else if (currentScene == "gotchi") {
            DrawText("GOTCHI", 14, 8, 18, {180, 180, 255, 255});
            DrawText("1: World  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  ESC: Exit", GAME_W - 290, 8, 12, {140, 140, 180, 255});
        } else {
            std::string sceneLabel = (currentScene == "boss") ? "BOSS ARENA" : "OVERWORLD";
            DrawText(sceneLabel.c_str(), 14, 8, 18, {180, 180, 255, 255});
            DrawText("1: World  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  ESC: Exit", GAME_W - 290, 8, 12, {140, 140, 180, 255});
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

    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(720, 720, "2D Engine Demo");
    SetTargetFPS(60);

    gameTarget = LoadRenderTexture(GAME_W, GAME_H);
    SetTextureFilter(gameTarget.texture, TEXTURE_FILTER_POINT);

    sceneManager = new SceneManager();
    sceneManager->registerScene("game", new GameScene());
    sceneManager->registerScene("boss", new BossScene());
    sceneManager->registerScene("hexboard", new HexViewScene());
    sceneManager->registerScene("input_test", new InputTestScene());
    sceneManager->registerScene("gotchi", new GotchiScene());
    sceneManager->switchSceneImmediate("hexboard");
    sceneManager->registerScene("sprite_test", new SpriteTestScene());
#ifdef HEXA_SHOT_TOOL
    sceneManager->switchSceneImmediate(
        (shotScene && shotScene[0]) ? shotScene : "game");
#else
    sceneManager->switchSceneImmediate("game");
#endif

    dialog = new DialogBox(
        {(float)GAME_W / 2.0f, (float)GAME_H - 20.0f},
        680.0f, 160.0f
    );
    dialog->setAnchor("bottom");
    dialog->setCharacterRevealSpeed(45.0f);

    gameDialogs = {
        { "Guide",  "Welcome, traveler. Use A/D or arrow keys to move. SPACE to jump.",
          {120, 220, 255, 255}, {30, 50, 100, 255} },
        { "Guide",  "Collect the golden gems. Watch out for the red creatures!",
          {120, 220, 255, 255}, {30, 50, 100, 255} },
        { "System", "Press 2 to enter the Boss Arena when you are ready. Good luck.",
          {200, 160, 255, 255}, {60, 20, 80, 255} },
    };
    bossDialogs = {
        { "???",    "So... you finally arrived. I have been waiting.",
          {255, 100, 120, 255}, {80, 20, 30, 255} },
        { "???",    "You think your little platforming skills will save you here?",
          {255, 100, 120, 255}, {80, 20, 30, 255} },
        { "System", "Press 1 to return to the overworld. Press H to replay dialog.",
          {200, 160, 255, 255}, {60, 20, 80, 255} },
    };

    showDialog(gameDialogs, 0);

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
    UnloadRenderTexture(gameTarget);
    delete dialog;
    delete sceneManager;
    CloseWindow();
#endif
    return 0;
}
