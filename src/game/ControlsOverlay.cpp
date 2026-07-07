#include "ControlsOverlay.hpp"
#include "GameConstants.hpp"
#include <algorithm>
#include <cstring>

// UI Constants
const int ControlsOverlay::MENU_X = 200;
const int ControlsOverlay::MENU_Y = 80;
const int ControlsOverlay::MENU_WIDTH = 400;
const int ControlsOverlay::MENU_HEIGHT = 500;
const int ControlsOverlay::ITEM_HEIGHT = 36;

ControlsOverlay::ControlsOverlay(SceneInputHandler* inputHandler)
    : inputHandler_(inputHandler),
      open_(false),
      selectedBindingIndex_(0),
      fadeAlpha_(1.0f),  // Start faded in
      waitingForInput_(false) {
    if (inputHandler_) {
        actionNames_ = inputHandler_->getAllActions();
    }
}

void ControlsOverlay::open() {
    if (!open_) {
        open_ = true;
        fadeAlpha_ = 0.0f;  // Fade in from transparent
        loadBindings();     // Reload bindings from file
    }
}

void ControlsOverlay::close() {
    if (open_) {
        open_ = false;
        fadeAlpha_ = 1.0f;  // Fully opaque when closed for next time
    }
}

void ControlsOverlay::update(float deltaTime) {
    if (!open_) return;

    // Fade in effect when opening
    if (fadeAlpha_ < 1.0f) {
        fadeAlpha_ += deltaTime * 2.0f;  // Fade in over 0.5 seconds
        if (fadeAlpha_ > 1.0f) fadeAlpha_ = 1.0f;
    }

    updateMenu(deltaTime);

    // Update message timer
    if (messageTimer_ > 0.0f) {
        messageTimer_ -= deltaTime;
        if (messageTimer_ <= 0.0f) {
            statusMessage_ = "";
        }
    }
}

void ControlsOverlay::updateMenu(float deltaTime) {
    if (!inputHandler_) return;

    if (waitingForInput_) {
        // First check for cancel keys before processing captured key
        // Note: inputHandler is updated in UpdateDrawFrame, so we check current state
        bool backspacePressed = inputHandler_->isKeyHeld(KEY_BACKSPACE);

        if (backspacePressed) {
            // Cancel the current binding selection, don't exit overlay
            waitingForInput_ = false;
            statusMessage_ = "Cancelled";
            messageTimer_ = 1.0f;
            inputHandler_->setWaitingForInput(false);
        } else {
            // If a key was captured, update the binding
            int capturedKey = inputHandler_->getCapturedKey();
            if (capturedKey != -1 && !actionNames_.empty() &&
                selectedBindingIndex_ >= 0 && selectedBindingIndex_ < static_cast<int>(actionNames_.size())) {
                std::string actionName = actionNames_[selectedBindingIndex_];
                inputHandler_->mapKey(actionName, capturedKey);
                saveBindings();

                statusMessage_ = "Bound!";
                messageTimer_ = 1.0f;
                waitingForInput_ = false;

                // Clear the captured key
                inputHandler_->setWaitingForInput(false);
            }
        }
    } else {
        updateNavigation(deltaTime);
    }
}

void ControlsOverlay::updateNavigation(float deltaTime) {
    static bool upKeyHeld = false;
    static bool downKeyHeld = false;
    static bool enterKeyHeld = false;  // Track Enter state to prevent repeated binding starts

    // Navigation with arrow keys or WASD
    bool upPressed = inputHandler_->isKeyHeld(KEY_UP) || inputHandler_->isKeyHeld(KEY_W);
    bool downPressed = inputHandler_->isKeyHeld(KEY_DOWN) || inputHandler_->isKeyHeld(KEY_S);

    // Up navigation - only trigger on initial press
    if (upPressed && !upKeyHeld) {
        selectedBindingIndex_--;
        if (selectedBindingIndex_ < 0) {
            selectedBindingIndex_ = static_cast<int>(actionNames_.size()) - 1;
        }
        upKeyHeld = true;
    } else if (!upPressed) {
        upKeyHeld = false;
    }

    // Down navigation - only trigger on initial press
    if (downPressed && !downKeyHeld) {
        selectedBindingIndex_++;
        if (selectedBindingIndex_ >= static_cast<int>(actionNames_.size())) {
            selectedBindingIndex_ = 0;
        }
        downKeyHeld = true;
    } else if (!downPressed) {
        downKeyHeld = false;
    }

    // Enter/Space to start binding
    bool enterPressed = inputHandler_->isKeyHeld(KEY_ENTER);
    if (enterPressed && !waitingForInput_ && !enterKeyHeld) {
        waitingForInput_ = true;
        statusMessage_ = "Press a key...";
        messageTimer_ = 2.0f;
        inputHandler_->setWaitingForInput(true);
        enterKeyHeld = true;
    } else if (!enterPressed) {
        enterKeyHeld = false;
    }

    // Check for return key (BACKSPACE only - ESC handled by GameScene::handleEscape)
    bool backspacePressed = inputHandler_->isKeyHeld(KEY_BACKSPACE);

    if (backspacePressed && !waitingForInput_) {
        // Close the overlay
        close();
        if (this->onClose) {
            onClose();
        }
    }

    // Mouse interaction (skip when waiting for input)
    if (!waitingForInput_) {
        handleMouseClick();
    }
}

void ControlsOverlay::draw() const {
    DrawRectangle(0, 0, GAME_W, GAME_H, {10, 10, 20, static_cast<unsigned char>(255 * fadeAlpha_)});

    // Title
    DrawText("CONTROLS SETTINGS", MENU_X + 80, MENU_Y - 40, 32,
             {180, 200, 255, static_cast<unsigned char>(255 * fadeAlpha_)});

    drawMenu();

    // Status message
    if (!statusMessage_.empty() && messageTimer_ > 0.0f) {
        DrawText(statusMessage_.c_str(), MENU_X + MENU_WIDTH - 60, MENU_Y - 40, 20,
                 {255, 255, 100, static_cast<unsigned char>(255 * fadeAlpha_)});
    }

    // Instructions at bottom
    DrawText("UP/DOWN: Select  ENTER: Bind  BACKSPACE: Return",
             GAME_W / 2 - 230, MENU_Y + MENU_HEIGHT + 60, 14,
             {100, 120, 160, static_cast<unsigned char>(255 * fadeAlpha_)});
}

void ControlsOverlay::drawMenu() const {
    if (actionNames_.empty()) return;

    int itemY = MENU_Y + 20;

    for (size_t i = 0; i < actionNames_.size(); i++) {
        bool isSelected = (i == static_cast<size_t>(selectedBindingIndex_));

        // Background for selected item
        if (isSelected && !waitingForInput_) {
            DrawRectangle(MENU_X - 10, itemY - 5, MENU_WIDTH + 20, ITEM_HEIGHT,
                          {60, 60, 120, static_cast<unsigned char>(150 * fadeAlpha_)});
        }

        // Action name (left side)
        std::string actionLabel = actionNames_[i];
        // Capitalize first letter
        if (!actionLabel.empty()) {
            actionLabel[0] = toupper(actionLabel[0]);
        }
        // Replace underscores with spaces
        for (char& c : actionLabel) {
            if (c == '_') c = ' ';
        }

        Color actionColor;
        if (isSelected) {
            actionColor = YELLOW;
        } else {
            actionColor = {180, 200, 255, static_cast<unsigned char>(255 * fadeAlpha_)};
        }
        DrawText(actionLabel.c_str(), MENU_X, itemY, 20, actionColor);

        // Current binding (right side)
        int keyCode = inputHandler_->getMappedKey(actionNames_[i]);
        std::string keyDisplay = ControlsOverlay::getBindingDisplayName(keyCode);

        if (waitingForInput_ && isSelected) {
            // Fix the color bug: YELLOW with faded alpha (not alpha in red channel)
            unsigned char alpha = static_cast<unsigned char>(255 * fadeAlpha_);
            DrawText("...", MENU_X + MENU_WIDTH - 80, itemY, 20,
                     {YELLOW.r, YELLOW.g, YELLOW.b, alpha});
        } else {
            Color bindingColor;
            if (isSelected) {
                bindingColor = WHITE;
            } else {
                bindingColor = {140, 160, 200, static_cast<unsigned char>(255 * fadeAlpha_)};
            }
            DrawText(keyDisplay.c_str(), MENU_X + MENU_WIDTH - 80, itemY, 20, bindingColor);
        }

        // Up/down indicator for selected
        if (isSelected && !waitingForInput_) {
            DrawText("^", MENU_X - 25, itemY - 8, 16,
                     {140, 160, 200, static_cast<unsigned char>(255 * fadeAlpha_)});
            DrawText("v", MENU_X - 25, itemY + ITEM_HEIGHT - 8, 16,
                     {140, 160, 200, static_cast<unsigned char>(255 * fadeAlpha_)});
        }

        itemY += ITEM_HEIGHT;
    }
}

std::string ControlsOverlay::getBindingDisplayName(int keyCode) {
    if (keyCode == -1) return "None";

    // Use SceneInputHandler utility methods
    if (keyCode >= MOUSE_BUTTON_LEFT && keyCode <= MOUSE_BUTTON_MIDDLE) {
        switch (keyCode) {
            case MOUSE_BUTTON_LEFT: return "Mouse Left";
            case MOUSE_BUTTON_RIGHT: return "Mouse Right";
            case MOUSE_BUTTON_MIDDLE: return "Mouse Middle";
            default: return "Mouse";
        }
    } else {
        // Simple key name mapping
        switch (keyCode) {
            case KEY_A: return "A"; case KEY_B: return "B"; case KEY_C: return "C";
            case KEY_D: return "D"; case KEY_E: return "E"; case KEY_F: return "F";
            case KEY_G: return "G"; case KEY_H: return "H"; case KEY_I: return "I";
            case KEY_J: return "J"; case KEY_K: return "K"; case KEY_L: return "L";
            case KEY_M: return "M"; case KEY_N: return "N"; case KEY_O: return "O";
            case KEY_P: return "P"; case KEY_Q: return "Q"; case KEY_R: return "R";
            case KEY_S: return "S"; case KEY_T: return "T"; case KEY_U: return "U";
            case KEY_V: return "V"; case KEY_W: return "W"; case KEY_X: return "X";
            case KEY_Y: return "Y"; case KEY_Z: return "Z";

            case KEY_ZERO: return "0"; case KEY_ONE: return "1"; case KEY_TWO: return "2";
            case KEY_THREE: return "3"; case KEY_FOUR: return "4"; case KEY_FIVE: return "5";
            case KEY_SIX: return "6"; case KEY_SEVEN: return "7"; case KEY_EIGHT: return "8";
            case KEY_NINE: return "9";

            case KEY_SPACE: return "Space"; case KEY_ENTER: return "Enter";
            case KEY_ESCAPE: return "Esc"; case KEY_BACKSPACE: return "Backspace";
            case KEY_TAB: return "Tab"; case KEY_DELETE: return "Del";

            case KEY_LEFT: return "Left"; case KEY_RIGHT: return "Right";
            case KEY_UP: return "Up"; case KEY_DOWN: return "Down";

            case KEY_LEFT_SHIFT: return "LShift"; case KEY_RIGHT_SHIFT: return "RShift";
            default: return getKeyName(keyCode);
        }
    }
}

std::string ControlsOverlay::getKeyName(int keyCode) {
    const char* name = GetKeyName(keyCode);
    if (name && strlen(name) > 0) {
        return std::string(name);
    }
    return "Unknown";
}

int ControlsOverlay::getHoveredItem() const {
    if (actionNames_.empty()) return -1;

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
    if (gameMouseX < MENU_X - 10 || gameMouseX > MENU_X + MENU_WIDTH + 10) {
        return -1;
    }

    int itemY = MENU_Y + 20;
    for (size_t i = 0; i < actionNames_.size(); i++) {
        if (gameMouseY >= itemY && gameMouseY <= itemY + ITEM_HEIGHT) {
            return static_cast<int>(i);
        }
        itemY += ITEM_HEIGHT;
    }

    return -1;
}

void ControlsOverlay::handleMouseClick() {
    bool mouseLeftPressed = IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
    static bool lastMouseLeftHeldLocal = false;

    // Only trigger on initial press
    if (mouseLeftPressed && !lastMouseLeftHeldLocal) {
        int hovered = getHoveredItem();
        if (hovered >= 0 && hovered < static_cast<int>(actionNames_.size())) {
            selectedBindingIndex_ = hovered;

            // If not already waiting for input, start binding mode
            if (!waitingForInput_) {
                waitingForInput_ = true;
                statusMessage_ = "Press a key...";
                messageTimer_ = 2.0f;
                if (inputHandler_) {
                    inputHandler_->setWaitingForInput(true);
                }
            }
        }
    }

    lastMouseLeftHeldLocal = mouseLeftPressed;
}

void ControlsOverlay::loadBindings() {
    if (!inputHandler_) return;

    // Load from canonical path
    std::string filepath = "config/controls.json";
    inputHandler_->loadBindingsFromFile(filepath);
}

void ControlsOverlay::saveBindings() {
    if (!inputHandler_) return;

    // Save to canonical path only
    std::string filepath = "config/controls.json";
    inputHandler_->saveBindingsToFile(filepath);
}
