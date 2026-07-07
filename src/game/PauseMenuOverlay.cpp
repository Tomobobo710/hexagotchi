#include "PauseMenuOverlay.hpp"
#include "GameConstants.hpp"

static const Color PAUSE_DIM_COLOR = {0, 0, 0, 160};
static const Color PAUSE_PANEL_COLOR = {20, 20, 40, 230};
static const Color PAUSE_PANEL_BORDER = {100, 100, 200, 255};
static const float PAUSE_PANEL_WIDTH = 280.0f;
static const float PAUSE_PANEL_HEIGHT = 260.0f;
static const float PAUSE_BUTTON_WIDTH = 200.0f;
static const float PAUSE_BUTTON_HEIGHT = 44.0f;
static const float PAUSE_BUTTON_SPACING = 16.0f;

PauseMenuOverlay::PauseMenuOverlay(Scene& s)
    : scene(s),
      resumeButton({(float)GAME_W / 2.0f, (float)GAME_H / 2.0f - 60.0f}, PAUSE_BUTTON_WIDTH, PAUSE_BUTTON_HEIGHT, "RESUME"),
      controlsButton({(float)GAME_W / 2.0f, (float)GAME_H / 2.0f}, PAUSE_BUTTON_WIDTH, PAUSE_BUTTON_HEIGHT, "CONTROLS"),
      exitButton({(float)GAME_W / 2.0f, (float)GAME_H / 2.0f + 60.0f}, PAUSE_BUTTON_WIDTH, PAUSE_BUTTON_HEIGHT, "EXIT GAME") {
    resumeButton.setAnchor("center");
    controlsButton.setAnchor("center");
    exitButton.setAnchor("center");

    resumeButton.setOnClick([this]() { if (onResume) onResume(); });
    controlsButton.setOnClick([this]() { if (onControlsSelected) onControlsSelected(); });
    exitButton.setOnClick([this]() { if (onExitSelected) onExitSelected(); });
}

void PauseMenuOverlay::open() {
    // Nothing stateful to reset currently, but kept as the hook GameScene
    // calls so future state (e.g. selected index for gamepad nav) has a home.
}

void PauseMenuOverlay::close() {
    if (onClose) onClose();
}

void PauseMenuOverlay::update(float deltaTime) {
    SceneInputHandler* input = scene.getInputHandler();
    resumeButton.update(input, deltaTime);
    controlsButton.update(input, deltaTime);
    exitButton.update(input, deltaTime);
}

void PauseMenuOverlay::draw() {
    DrawRectangle(0, 0, GAME_W, GAME_H, PAUSE_DIM_COLOR);

    Rectangle panel = {
        (GAME_W - PAUSE_PANEL_WIDTH) / 2.0f,
        (GAME_H - PAUSE_PANEL_HEIGHT) / 2.0f - 40.0f,
        PAUSE_PANEL_WIDTH, PAUSE_PANEL_HEIGHT
    };
    DrawRectangleRec(panel, PAUSE_PANEL_COLOR);
    DrawRectangleLinesEx(panel, 2.0f, PAUSE_PANEL_BORDER);

    int titleWidth = MeasureText("PAUSED", 28);
    DrawText("PAUSED", (int)(GAME_W / 2.0f - titleWidth / 2.0f), (int)(panel.y + 20.0f), 28, WHITE);

    resumeButton.draw();
    controlsButton.draw();
    exitButton.draw();
}
