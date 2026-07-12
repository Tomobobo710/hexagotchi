#include "SkipSceneOverlay.hpp"
#include "GameConstants.hpp"

// GotchiScene's main action buttons are 64.0f tall (GotchiScene.cpp's
// addButton()); this button is deliberately half that.
static const float SKIP_BUTTON_WIDTH = 90.0f;
static const float SKIP_BUTTON_HEIGHT = 32.0f;
static const float SKIP_BUTTON_PAD = 12.0f;

static const Color SKIP_DIM_COLOR = {0, 0, 0, 160};
static const Color SKIP_PANEL_COLOR = {20, 20, 40, 230};
static const Color SKIP_PANEL_BORDER = {100, 100, 200, 255};
static const float SKIP_PANEL_WIDTH = 320.0f;
static const float SKIP_PANEL_HEIGHT = 160.0f;
static const float SKIP_CONFIRM_BUTTON_WIDTH = 120.0f;
static const float SKIP_CONFIRM_BUTTON_HEIGHT = 44.0f;

SkipSceneOverlay::SkipSceneOverlay()
    : skipButton({SKIP_BUTTON_PAD, SKIP_BUTTON_PAD}, SKIP_BUTTON_WIDTH, SKIP_BUTTON_HEIGHT, "SKIP"),
      confirmYesButton({(float)GAME_W / 2.0f - 70.0f, (float)GAME_H / 2.0f + 30.0f}, SKIP_CONFIRM_BUTTON_WIDTH, SKIP_CONFIRM_BUTTON_HEIGHT, "YES"),
      confirmNoButton({(float)GAME_W / 2.0f + 70.0f, (float)GAME_H / 2.0f + 30.0f}, SKIP_CONFIRM_BUTTON_WIDTH, SKIP_CONFIRM_BUTTON_HEIGHT, "NO"),
      confirmOpen(false) {
    skipButton.setAnchor("top-left");
    skipButton.setFontSize(14);
    skipButton.setBackgroundColor({60, 60, 100, 220});
    skipButton.setHoverColor({100, 100, 160, 240});
    skipButton.setBorderColor({150, 150, 200, 255});
    skipButton.setOnClick([this]() { confirmOpen = true; });

    confirmYesButton.setAnchor("center");
    confirmNoButton.setAnchor("center");
    confirmYesButton.setOnClick([this]() {
        confirmOpen = false;
        if (onConfirmSkip) onConfirmSkip();
    });
    confirmNoButton.setOnClick([this]() { confirmOpen = false; });
}

void SkipSceneOverlay::update(float deltaTime, SceneInputHandler* input, bool visible) {
    if (!visible) {
        confirmOpen = false;
        return;
    }

    if (confirmOpen) {
        confirmYesButton.update(input, deltaTime);
        confirmNoButton.update(input, deltaTime);
    } else {
        skipButton.update(input, deltaTime);
    }
}

void SkipSceneOverlay::draw(bool visible) {
    if (!visible) return;

    skipButton.draw();

    if (confirmOpen) {
        DrawRectangle(0, 0, GAME_W, GAME_H, SKIP_DIM_COLOR);

        Rectangle panel = {
            (GAME_W - SKIP_PANEL_WIDTH) / 2.0f,
            (GAME_H - SKIP_PANEL_HEIGHT) / 2.0f,
            SKIP_PANEL_WIDTH, SKIP_PANEL_HEIGHT
        };
        DrawRectangleRec(panel, SKIP_PANEL_COLOR);
        DrawRectangleLinesEx(panel, 2.0f, SKIP_PANEL_BORDER);

        const char* msg = "Skip this scene?";
        int msgWidth = MeasureText(msg, 22);
        DrawText(msg, (int)(GAME_W / 2.0f - msgWidth / 2.0f), (int)(panel.y + 24.0f), 22, WHITE);

        confirmYesButton.draw();
        confirmNoButton.draw();
    }
}
