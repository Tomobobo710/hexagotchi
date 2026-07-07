#include "PauseMenuOverlay.hpp"
#include "GameScene.hpp"
#include "../engine/GameConstants.hpp"
#include "../engine/SceneInputHandler.hpp"
#include <algorithm>

// Raylib includes for raw input access
#include "raylib.h"

// UI Constants
const int PauseMenuOverlay::MENU_X = 260;
const int PauseMenuOverlay::MENU_Y = 200;
const int PauseMenuOverlay::MENU_WIDTH = 400;
const int PauseMenuOverlay::BUTTON_HEIGHT = 50;
const int PauseMenuOverlay::FONT_SIZE = 28;

PauseMenuOverlay::PauseMenuOverlay(GameScene& parent)
    : parent_(parent),
      open_(false),
      selectedOptionIndex_(0),
      fadeAlpha_(0.0f) {
}

void PauseMenuOverlay::open() {
    if (!open_) {
        open_ = true;
        fadeAlpha_ = 0.0f;
    }
}

void PauseMenuOverlay::close() {
    if (open_) {
        open_ = false;
        fadeAlpha_ = 1.0f;  // Start fully faded out for next time
    }
}

void PauseMenuOverlay::update(float deltaTime, SceneInputHandler* handler) {
    if (!open_) return;

    // Fade in effect when opening
    if (fadeAlpha_ < 1.0f) {
        fadeAlpha_ += deltaTime * 2.0f;  // Fade in over 0.5 seconds
        if (fadeAlpha_ > 1.0f) fadeAlpha_ = 1.0f;
    }

    updateMenu(deltaTime, handler);
}

void PauseMenuOverlay::updateMenu(float deltaTime, SceneInputHandler* handler) {
    if (!handler) return;

    static bool upKeyHeld = false;
    static bool downKeyHeld = false;
    static bool enterKeyHeld = false;

    // Navigation with arrow keys or WASD
    bool upPressed = handler->isKeyHeld(KEY_UP) || handler->isKeyHeld(KEY_W);
    bool downPressed = handler->isKeyHeld(KEY_DOWN) || handler->isKeyHeld(KEY_S);

    // Up navigation - only trigger on initial press
    if (upPressed && !upKeyHeld) {
        selectedOptionIndex_--;
        if (selectedOptionIndex_ < 0) {
            selectedOptionIndex_ = static_cast<int>(menuOptions.size()) - 1;
        }
        upKeyHeld = true;
    } else if (!upPressed) {
        upKeyHeld = false;
    }

    // Down navigation - only trigger on initial press
    if (downPressed && !downKeyHeld) {
        selectedOptionIndex_++;
        if (selectedOptionIndex_ >= static_cast<int>(menuOptions.size())) {
            selectedOptionIndex_ = 0;
        }
        downKeyHeld = true;
    } else if (!downPressed) {
        downKeyHeld = false;
    }

    // Enter/Space to select
    bool enterPressed = handler->isKeyHeld(KEY_ENTER) || handler->isKeyHeld(KEY_SPACE);
    if (enterPressed && !enterKeyHeld) {
        if (selectedOptionIndex_ == 0) {
            // Resume Game
            close();
            if (onResume) {
                onResume();
            }
        } else if (selectedOptionIndex_ == 1) {
            // Controls
            close();
            if (onControlsSelected) {
                onControlsSelected();
            }
        } else if (selectedOptionIndex_ == 2) {
            // Exit Game
            close();
            if (onExitSelected) {
                onExitSelected();
            }
        }
        enterKeyHeld = true;
    } else if (!enterPressed) {
        enterKeyHeld = false;
    }

    // ESC to close menu (edge-triggered via IsKeyDown in handler)
    bool escapePressed = handler->isKeyHeld(KEY_ESCAPE);
    if (escapePressed) {
        // Close menu on ESC - single callback path via onClose
        close();
        if (onClose) {
            onClose();
        }
        if (onResume) {
            onResume();
        }
    }

    // Mouse interaction
    handleMouseClick(handler);
}

void PauseMenuOverlay::draw() const {
    if (!open_) return;

    DrawRectangle(0, 0, GAME_W, GAME_H, {10, 10, 20, static_cast<unsigned char>(255 * fadeAlpha_)});

    // Title
    std::string title = "PAUSE MENU";
    int titleWidth = MeasureText(title.c_str(), 48);
    DrawText(title.c_str(), (GAME_W - titleWidth) / 2, MENU_Y - 60, 48,
             {180, 200, 255, static_cast<unsigned char>(255 * fadeAlpha_)});

    drawMenu();

    // Instructions at bottom
    DrawText("UP/DOWN: Select  ENTER/SPACE: Confirm  ESC: Resume",
             GAME_W / 2 - 220, MENU_Y + BUTTON_HEIGHT * 3 + 70, 16,
             {100, 120, 160, static_cast<unsigned char>(255 * fadeAlpha_)});
}

void PauseMenuOverlay::drawMenu() const {
    int itemY = MENU_Y;

    for (size_t i = 0; i < menuOptions.size(); i++) {
        bool isSelected = (i == static_cast<size_t>(selectedOptionIndex_));

        // Button background
        int bgX = MENU_X;
        int bgY = itemY;
        int bgW = MENU_WIDTH;
        int bgH = BUTTON_HEIGHT;

        // Background color based on selection
        Color bgColor;
        if (isSelected) {
            bgColor.r = 60; bgColor.g = 80; bgColor.b = 140; bgColor.a = 255;  // Highlighted blue
        } else {
            bgColor.r = 30; bgColor.g = 30; bgColor.b = 50; bgColor.a = 255;  // Dimmed gray-blue
        }
        DrawRectangle(bgX, bgY, bgW, bgH, bgColor);

        // Border
        Color borderColor;
        if (isSelected) {
            borderColor = YELLOW;
        } else {
            borderColor.r = 80; borderColor.g = 80; borderColor.b = 120; borderColor.a = 255;
        }
        DrawRectangleLines(bgX, bgY, bgW, bgH, borderColor);

        // Text
        std::string text = menuOptions[i];
        int textWidth = MeasureText(text.c_str(), FONT_SIZE);
        Color textColor;
        if (isSelected) {
            textColor = YELLOW;
        } else {
            textColor.r = 180; textColor.g = 200; textColor.b = 255; textColor.a = 255;
        }
        DrawText(text.c_str(), MENU_X + (MENU_WIDTH - textWidth) / 2, itemY + 12, FONT_SIZE, textColor);

        // Up/down indicators for selected
        if (isSelected) {
            DrawText("^", MENU_X - 20, itemY - 5, 20,
                     {140, 160, 200, 255});
            DrawText("v", MENU_X - 20, itemY + bgH + 5, 20,
                     {140, 160, 200, 255});
        }

        itemY += BUTTON_HEIGHT + 10;  // Spacing between buttons
    }
}

int PauseMenuOverlay::getHoveredOption() const {
    Vector2 mousePos = GetMousePosition();

    // Calculate the letterbox transform to convert screen coordinates to game coordinates
    float scaleX = static_cast<float>(GetScreenWidth()) / static_cast<float>(GAME_W);
    float scaleY = static_cast<float>(GetScreenHeight()) / static_cast<float>(GAME_H);
    float scale  = (scaleX < scaleY) ? scaleX : scaleY;

    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    float drawW  = GAME_W * scale;
    float drawH  = GAME_H * scale;
    float offsetX = (sw - drawW) * 0.5f;
    float offsetY = (sh - drawH) * 0.5f;

    float gameMouseX = (mousePos.x - offsetX) / scale;
    float gameMouseY = (mousePos.y - offsetY) / scale;

    // Check if mouse is within menu area
    if (gameMouseX < MENU_X || gameMouseX > MENU_X + MENU_WIDTH) {
        return -1;
    }

    for (size_t i = 0; i < menuOptions.size(); i++) {
        int itemY = MENU_Y + i * (BUTTON_HEIGHT + 10);
        if (gameMouseY >= itemY && gameMouseY <= itemY + BUTTON_HEIGHT) {
            return static_cast<int>(i);
        }
    }

    return -1;
}

void PauseMenuOverlay::handleMouseClick(SceneInputHandler* handler) {
    // Use raylib directly for mouse state - not through action mappings
    bool mouseLeftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    static bool lastMouseLeftHeldLocal = false;

    // Only trigger on initial press
    if (mouseLeftPressed && !lastMouseLeftHeldLocal) {
        int hovered = getHoveredOption();
        if (hovered >= 0 && hovered < static_cast<int>(menuOptions.size())) {
            selectedOptionIndex_ = hovered;

            // Trigger the selection callback
            if (selectedOptionIndex_ == 0) {
                close();
                if (onResume) {
                    onResume();
                }
            } else if (selectedOptionIndex_ == 1) {
                close();
                if (onControlsSelected) {
                    onControlsSelected();
                }
            } else if (selectedOptionIndex_ == 2) {
                close();
                if (onExitSelected) {
                    onExitSelected();
                }
            }
        }
    }

    lastMouseLeftHeldLocal = mouseLeftPressed;
}
