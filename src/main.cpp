#include "SceneManager.hpp"
#include "DialogBox.hpp"
#include "GameConstants.hpp"
#include "game/GameScene.hpp"
#include "game/BossScene.hpp"
#include "game/InputTestScene.hpp"
#include "game/HexBoard.hpp"
#include "game/DialogSequences.hpp"
#include <string>
#include <vector>

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
    float drawW  = GAME_W * scale;
    float drawH  = GAME_H * scale;
    return { (sw - drawW) * 0.5f, (sh - drawH) * 0.5f, drawW, drawH };
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
    float dt = GetFrameTime();
    std::string currentScene = sceneManager->getCurrentSceneName();

    if (currentScene != lastScene) {
        dialogIndex = 0;
        if (currentScene == "game") showDialog(gameDialogs, 0);
        if (currentScene == "boss") showDialog(bossDialogs, 0);
        lastScene = currentScene;
    }

    // Hide dialog on input test and hexboard scenes
    if (currentScene == "input_test" || currentScene == "hexboard") {
        dialog->hide();
    }

    if (IsKeyPressed(KEY_ONE) && currentScene != "game")
        sceneManager->switchScene("game", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_TWO) && currentScene != "boss")
        sceneManager->switchScene("boss", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_THREE) && currentScene != "hexboard")
        sceneManager->switchScene("hexboard", TransitionEffect::FADE, 0.5f);
    if (IsKeyPressed(KEY_FOUR) && currentScene != "input_test")
        sceneManager->switchScene("input_test", TransitionEffect::FADE, 0.5f);

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

    sceneManager->update(dt);
    dialog->update(dt);

    BeginTextureMode(gameTarget);
        ClearBackground(BLACK);
        sceneManager->draw();
        dialog->draw();
        DrawRectangle(0, 0, GAME_W, 32, {0, 0, 0, 160});
        if (currentScene == "hexboard") {
            DrawText("HEXBOARD", 14, 8, 18, {180, 180, 255, 255});
            DrawText("1: World  2: Boss  3: Hexboard  4: Input Test  ESC: Exit", GAME_W - 310, 8, 12, {140, 140, 180, 255});
        } else if (currentScene == "input_test") {
            DrawText("INPUT TEST", 14, 8, 18, {180, 180, 255, 255});
            DrawText("1: World  2: Boss  3: Hexboard  4: Input Test  ESC: Exit", GAME_W - 310, 8, 12, {140, 140, 180, 255});
        } else {
            std::string sceneLabel = (currentScene == "boss") ? "BOSS ARENA" : "OVERWORLD";
            DrawText(sceneLabel.c_str(), 14, 8, 18, {180, 180, 255, 255});
            DrawText("1: World  2: Boss  3: Hexboard  H: Dialog  ESC: Exit", GAME_W - 290, 8, 12, {140, 140, 180, 255});
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
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(720, 720, "2D Engine Demo");
    SetTargetFPS(60);

    gameTarget = LoadRenderTexture(GAME_W, GAME_H);
    SetTextureFilter(gameTarget.texture, TEXTURE_FILTER_BILINEAR);

    sceneManager = new SceneManager();
    sceneManager->registerScene("game", new GameScene());
    sceneManager->registerScene("boss", new BossScene());
    sceneManager->registerScene("hexboard", new HexBoard());
    sceneManager->registerScene("input_test", new InputTestScene());
    sceneManager->switchSceneImmediate("hexboard");

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
    while (!WindowShouldClose() && !IsKeyPressed(KEY_ESCAPE)) {
        UpdateDrawFrame();
    }
    UnloadRenderTexture(gameTarget);
    delete dialog;
    delete sceneManager;
    CloseWindow();
#endif
    return 0;
}
