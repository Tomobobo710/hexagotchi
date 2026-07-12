#include "TitleScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "SceneManager.hpp"
#include "GotchiScene.hpp"
#include "ToyAnimationScene.hpp"
#include "GameState.h"
#include "TutorialController.hpp"
#include <functional>
#include <fstream>
#include <sstream>
#include <iostream>

// Global scene manager pointer (extern'd in main.cpp)
extern SceneManager* sceneManager;

// Draws a horizontally-elongated hexagon (flat top/bottom, pointed left/right
// ends) filling a rect -- an on-theme "hex pill". `endFrac` is how far the
// pointed ends reach in from each side, as a fraction of the height. A filled
// convex fan from the center covers it cleanly at any alpha.
static void DrawHexPill(Rectangle r, float endFrac, Color color) {
    float ptInset = r.height * endFrac;   // horizontal reach of each pointed end
    float midY = r.y + r.height * 0.5f;
    // 6 vertices, clockwise from the left point.
    Vector2 v[6] = {
        { r.x,                    midY },                 // left point
        { r.x + ptInset,          r.y },                  // top-left
        { r.x + r.width - ptInset, r.y },                 // top-right
        { r.x + r.width,          midY },                 // right point
        { r.x + r.width - ptInset, r.y + r.height },      // bottom-right
        { r.x + ptInset,          r.y + r.height },       // bottom-left
    };
    Vector2 center = { r.x + r.width * 0.5f, midY };
    // Triangle fan (DrawTriangle winding: v[i], center, v[i+1] to stay CCW in
    // raylib's y-down space so the fill isn't culled).
    for (int i = 0; i < 6; i++) {
        Vector2 a = v[i];
        Vector2 b = v[(i + 1) % 6];
        DrawTriangle(a, center, b, color);
    }
}

// SaveManager - DISABLED: Save system shut off for game jam
// extern SaveManager saveManager;

// Global GameState (extern'd in GameState.h, defined in main.cpp)
extern GameState globalGameState;

TitleScene::TitleScene()
    : Scene(720.0f, 720.0f, {0, 0, 0, 255})
    // DISABLED: Save system shut off for game jam
    // , saveManager_(saveManager)
    // , showingLoadOptions_(false)
{
    // DISABLED: Save system shut off for game jam
}

Button* TitleScene::createButton(const std::string& label, float x, float y, float width, float height) {
    Button* btn = new Button({x, y}, width, height, label);
    btn->setAnchor("top-left");
    btn->setFontSize(16);
    btn->setBackgroundColor({60, 60, 100, 220});
    btn->setHoverColor({100, 100, 160, 240});
    btn->setBorderColor({150, 150, 200, 255});
    return btn;
}

// DISABLED: Save system shut off for game jam
#if 0
void TitleScene::refreshSlotButtons() {
    // Update all slot buttons with current summaries
    for (int i = 0; i < SaveManager::NUM_SLOTS; i++) {
        SlotSummary summary = saveManager_.summary(i);
        std::string label;

        if (summary.exists) {
            // Show summary info: beat index and playtime
            int minutes = static_cast<int>(summary.playtimeSeconds) / 60;
            label = "Slot " + std::to_string(i + 1) + ": " + summary.label;
            if (summary.label.empty()) {
                label = "Slot " + std::to_string(i + 1) + " (v" + std::to_string(summary.version) + ", beat " + std::to_string(summary.storyBeatIndex) + ")";
            }
            if (minutes > 0) {
                label += " (" + std::to_string(minutes) + "m)";
            }
        } else {
            label = "Slot " + std::to_string(i + 1) + ": Empty";
        }

        // Update button label
        if (i < (int)slotButtons_.size()) {
            slotButtons_[i]->setLabel(label);
            // Enable all slots - empty slots can be saved to, occupied slots can be loaded
            slotButtons_[i]->setEnabled(true);
        }
    }
}

void TitleScene::onLoadGame() {
    // Switch to load selection mode
    showingLoadOptions_ = true;
    selectedSlot_ = -1;
    refreshSlotButtons();
}

void TitleScene::onLoadFromSlot(int slot) {
    // Try to load from the specified slot
    auto loadedState = saveManager_.load(slot);
    if (loadedState.has_value()) {
        GameState& state = loadedState.value();
        std::cout << "[TitleScene] Game loaded from slot " << slot
                  << ", version: " << state.version
                  << ", storyBeatIndex: " << state.storyBeatIndex << std::endl;

        // Set as active slot for autosaves
        saveManager_.setActiveSlot(slot);

        // Update globalGameState so the loaded game actually starts
        globalGameState = state;

        // Transition through the toy intro animation on the way to the
        // gotchi scene -- see onNewGame() for the pattern.
        if (sceneManager) {
            sceneManager->switchScene("toy_animation");
            ToyAnimationScene* toyAnim = static_cast<ToyAnimationScene*>(sceneManager->getScene("toy_animation"));
            if (toyAnim) toyAnim->startIntro("gotchi");
        }
    } else {
        std::cerr << "[TitleScene] No save found in slot " << slot << std::endl;
    }
}

void TitleScene::onSaveToSlot(int slot) {
    // Save current globalGameState to the selected slot

    bool success = saveManager_.save(slot, globalGameState);
    if (success) {
        saveManager_.setActiveSlot(slot);
        std::cout << "[TitleScene] Game saved to slot " << slot << std::endl;
        refreshSlotButtons();  // Update display
    } else {
        std::cerr << "[TitleScene] Failed to save to slot " << slot << std::endl;
    }
}

void TitleScene::onDeleteSlot(int slot) {
    // Delete the specified slot
    bool success = saveManager_.deleteSlot(slot);
    if (success) {
        std::cout << "[TitleScene] Deleted slot " << slot << std::endl;
        refreshSlotButtons();  // Update display
    } else {
        std::cerr << "[TitleScene] Failed to delete slot " << slot << std::endl;
    }
}

void TitleScene::goBackToMainMenu() {
    showingLoadOptions_ = false;
    selectedSlot_ = -1;
}
#endif

void TitleScene::onNewGame() {
    // Fresh-run reset that PRESERVES tutorial_seen (see ResetRunKeepingTutorial)
    // -- consistent with death Try Again / credits Play Again, so the tutorial
    // only ever plays on the very first run of the session. Audio/dialog
    // preferences are external globals and survive regardless.
    ResetRunKeepingTutorial(globalGameState);

    // Transition through the toy intro animation on the way to the gotchi
    // scene -- same "switch, then configure before init() actually runs"
    // pattern StorySequencer::startMergeTransition uses for MergeScene.
    if (sceneManager) {
        sceneManager->switchScene("toy_animation");
        ToyAnimationScene* toyAnim = static_cast<ToyAnimationScene*>(sceneManager->getScene("toy_animation"));
        if (toyAnim) toyAnim->startIntro("gotchi");
    }
}

void TitleScene::init() {
    float centerX = (float)GAME_W / 2.0f;
    float centerY = (float)GAME_H / 2.0f;

    // Initialize title scroll shader effect
    titleShader_ = new TitleScrollShader();
    addEffect(titleShader_);

    // Create main menu buttons
    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;

    // Button cluster sits at ~1/3 up from the bottom of the screen. The pair is
    // centered on buttonsCenterY; the gap between them is (buttonHeight + 14).
    float buttonsCenterY = (float)GAME_H * 2.0f / 3.0f;
    float buttonGap = buttonHeight + 14.0f;

    // Start Game button (was NEW GAME) -- top of the pair.
    newGameButton_ = new Button({centerX, buttonsCenterY - buttonGap / 2.0f}, buttonWidth, buttonHeight, "START GAME");
    newGameButton_->setAnchor("center");
    newGameButton_->setFontSize(20);
    newGameButton_->setOnClick([this]() { onNewGame(); });

    // Options button, directly below Start Game.
    optionsButton_ = new Button({centerX, buttonsCenterY + buttonGap / 2.0f}, buttonWidth, buttonHeight, "OPTIONS");
    optionsButton_->setAnchor("center");
    optionsButton_->setFontSize(20);
    optionsButton_->setOnClick([this]() {
        if (sceneManager) sceneManager->switchScene("options");
    });

    // DISABLED: Save system shut off for game jam
    // loadGameButton_ = new Button({centerX, centerY + buttonHeight/2}, buttonWidth, buttonHeight, "LOAD GAME");
    // loadGameButton_->setAnchor("center");
    // loadGameButton_->setFontSize(20);
    // loadGameButton_->setOnClick([this]() { onLoadGame(); });

    // Main menu SAVE button - DISABLED
    // mainMenuSaveButton_ = new Button({centerX, centerY + 100}, 120, 36, "SAVE");
    // mainMenuSaveButton_->setAnchor("center");
    // mainMenuSaveButton_->setFontSize(16);
    // mainMenuSaveButton_->setBackgroundColor({50, 100, 50, 220});
    // mainMenuSaveButton_->setHoverColor({80, 140, 80, 240});
    // mainMenuSaveButton_->setOnClick([this]() {
    //     if (saveManager_.activeSlot() >= 0) {
    //         saveManager_.save(saveManager_.activeSlot(), globalGameState);
    //         std::cout << "[TitleScene] Game saved to active slot " << saveManager_.activeSlot() << std::endl;
    //     } else {
    //         std::cerr << "[TitleScene] No active slot set for save" << std::endl;
    //     }
    // });

    // DISABLED: Slot selection buttons
    // Create slot selection buttons (for load/save)
    // float slotButtonWidth = 320.0f;
    // float slotButtonHeight = 32.0f;
    // float slotSpacing = 6.0f;
    // float slotStartY = centerY - (SaveManager::NUM_SLOTS * (slotButtonHeight + slotSpacing)) / 2 + slotButtonHeight/2;

    // for (int i = 0; i < SaveManager::NUM_SLOTS; i++) {
    //     float yPos = slotStartY + i * (slotButtonHeight + slotSpacing);
    //     Button* btn = createButton("Empty", centerX, yPos, slotButtonWidth, slotButtonHeight);
    //     int slot = i;
    //     btn->setOnClick([this, slot]() { onLoadFromSlot(slot); });
    //     slotButtons_.push_back(btn);
    // }

    // DISABLED: Action buttons (Save, Delete, Back)
    // Create action buttons (Save, Delete, Back)
    // float actionButtonWidth = 100.0f;
    // float actionButtonHeight = 28.0f;
    // float actionSpacing = 10.0f;

    // // Save button
    // Button* saveBtn = createButton("SAVE", centerX - actionButtonWidth/2 - actionSpacing/2, centerY + 80, actionButtonWidth, actionButtonHeight);
    // saveBtn->setFontSize(14);
    // saveBtn->setOnClick([this]() {
    //     if (selectedSlot_ >= 0) {
    //         onSaveToSlot(selectedSlot_);
    //     }
    // });
    // actionButtons_.push_back(saveBtn);

    // // Delete button
    // Button* deleteBtn = createButton("DELETE", centerX + actionSpacing/2, centerY + 80, actionButtonWidth, actionButtonHeight);
    // deleteBtn->setFontSize(14);
    // deleteBtn->setBackgroundColor({100, 50, 50, 220});
    // deleteBtn->setHoverColor({160, 80, 80, 240});
    // deleteBtn->setOnClick([this]() {
    //     if (selectedSlot_ >= 0) {
    //         onDeleteSlot(selectedSlot_);
    //     }
    // });
    // actionButtons_.push_back(deleteBtn);

    // // Back button
    // Button* backBtn = createButton("BACK", centerX, centerY + 120, 100, 28);
    // backBtn->setFontSize(14);
    // backBtn->setOnClick([this]() { goBackToMainMenu(); });
    // actionButtons_.push_back(backBtn);

    // Center camera
    getCamera()->setBoundary(0, 0, (float)GAME_W, (float)GAME_H);
}

void TitleScene::update(float deltaTime) {
    Scene::update(deltaTime);

    SceneInputHandler* input = getInputHandler();

    // DISABLED: Save system shut off for game jam
    // if (showingLoadOptions_) {
    //     // Update slot selection buttons
    //     for (int i = 0; i < (int)slotButtons_.size(); i++) {
    //         slotButtons_[i]->update(input, deltaTime);
    //
    //         // Set selected slot when button is clicked (pressed and released)
    //         if (slotButtons_[i]->isPressed()) {
    //             selectedSlot_ = i;
    //             std::cout << "[TitleScene] Selected slot " << i << std::endl;
    //         }
    //     }
    //
    //     // Update action buttons (Save, Delete, Back)
    //     for (Button* btn : actionButtons_) {
    //         btn->update(input, deltaTime);
    //     }
    // } else {
        // Update main menu buttons
        if (newGameButton_) {
            newGameButton_->update(input, deltaTime);
        }
        if (optionsButton_) {
            optionsButton_->update(input, deltaTime);
        }
        // if (loadGameButton_) {
        //     loadGameButton_->update(input, deltaTime);
        // }
        // if (mainMenuSaveButton_) {
        //     mainMenuSaveButton_->update(input, deltaTime);
        // }
    // }
}

void TitleScene::draw() {
    Scene::draw();

    // A translucent grey HEX pill behind the title, so it reads over the busy
    // hex background (and stays on-theme).
    Color pillColor = {60, 60, 70, 150};   // grey, see-through
    {
        const char* title = "HEXAGOTCHI";
        int titleSize = 48;
        int titleWidth = MeasureText(title, titleSize);
        int titleY = 100;
        // Extra horizontal pad so the text clears the hex's pointed ends.
        float padX = 60.0f, padY = 16.0f;
        Rectangle pill = {
            (GAME_W - titleWidth) / 2.0f - padX,
            (float)titleY - padY,
            titleWidth + padX * 2.0f,
            titleSize + padY * 2.0f
        };
        DrawHexPill(pill, 0.5f, pillColor);
        DrawText(title, (int)((GAME_W - titleWidth) / 2.0f), titleY, titleSize, {200, 200, 255, 255});
    }

    // Credits pill: a single grey capsule near the bottom of the screen holding
    // both credit lines, text centered inside it, with padding from the edge.
    {
        Color creditColor = {200, 200, 220, 220};
        const char* creditLine1 = "A Game by";
        const char* creditLine2 = "Tomobobo and Bazola";
        int cSize = 20;
        int w1 = MeasureText(creditLine1, cSize);
        int w2 = MeasureText(creditLine2, cSize);
        int textW = (w1 > w2) ? w1 : w2;
        float lineGap = 24.0f;
        // Extra horizontal pad so text clears the hex's pointed ends.
        float padX = 60.0f, padY = 14.0f;

        float pillW = textW + padX * 2.0f;
        // Content = line1 (cSize tall) + gap down to line2's top + line2 (cSize).
        float pillH = (float)cSize + lineGap + padY * 2.0f;

        float edgePad = 24.0f;                       // padding from the bottom edge
        float pillY = (float)GAME_H - edgePad - pillH;
        float pillX = (GAME_W - pillW) / 2.0f;
        Rectangle pill = { pillX, pillY, pillW, pillH };
        DrawHexPill(pill, 0.5f, pillColor);

        // Center each line horizontally within the pill.
        float line1Y = pillY + padY;
        float line2Y = line1Y + lineGap;
        DrawText(creditLine1, (int)(pillX + (pillW - w1) / 2.0f), (int)line1Y, cSize, creditColor);
        DrawText(creditLine2, (int)(pillX + (pillW - w2) / 2.0f), (int)line2Y, cSize, creditColor);
    }

    // DISABLED: Save system shut off for game jam
    // if (showingLoadOptions_) {
    //     // Draw slot selection UI
    //     for (int i = 0; i < SaveManager::NUM_SLOTS; i++) {
    //         if (i < (int)slotButtons_.size()) {
    //             // Highlight selected slot
    //             if (i == selectedSlot_) {
    //                 Rectangle bounds = slotButtons_[i]->getBounds();
    //                 DrawRectangle((int)bounds.x - 5, (int)bounds.y - 5, (int)bounds.width + 10, (int)bounds.height + 10,
    //                               {80, 80, 120, 60});
    //             }
    //             slotButtons_[i]->draw();
    //         }
    //     }
    //
    //     // Draw action buttons
    //     for (Button* btn : actionButtons_) {
    //         btn->draw();
    //     }
    //
    //     // Draw instructions
    //     DrawText("Click a slot to select | SAVE: save to selected | DELETE: remove selected", 30, 480, 14, {140, 180, 255, 255});
    // } else {
        // Draw main menu buttons
        if (newGameButton_) {
            newGameButton_->draw();
        }
        if (optionsButton_) {
            optionsButton_->draw();
        }
        // if (loadGameButton_) {
        //     loadGameButton_->draw();
        // }
        // if (mainMenuSaveButton_) {
        //     mainMenuSaveButton_->draw();
        // }
    // }
}

void TitleScene::cleanup() {
    Scene::cleanup();

    if (newGameButton_) {
        delete newGameButton_;
        newGameButton_ = nullptr;
    }
    if (optionsButton_) {
        delete optionsButton_;
        optionsButton_ = nullptr;
    }

    titleShader_ = nullptr;
    // if (loadGameButton_) {
    //     delete loadGameButton_;
    //     loadGameButton_ = nullptr;
    // }
    // if (mainMenuSaveButton_) {
    //     delete mainMenuSaveButton_;
    //     mainMenuSaveButton_ = nullptr;
    // }

    // for (Button* btn : slotButtons_) {
    //     delete btn;
    // }
    // slotButtons_.clear();

    // for (Button* btn : actionButtons_) {
    //     delete btn;
    // }
    // actionButtons_.clear();

    // showingLoadOptions_ = false;
    // selectedSlot_ = -1;
}

