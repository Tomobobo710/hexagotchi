#include "TitleScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "SceneManager.hpp"
#include "GotchiScene.hpp"
#include "ToyAnimationScene.hpp"
#include "GameState.h"
#include <functional>
#include <fstream>
#include <sstream>
#include <iostream>

// Global scene manager pointer (extern'd in main.cpp)
extern SceneManager* sceneManager;

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
    // Create a new GameState with default values
    GameState state;
    state.version = SAVE_VERSION;
    state.mode = Mode::Gotchi;
    state.storyBeatIndex = 0;
    state.mergeCount = 0;

    // DISABLED: Save system shut off for game jam
    // bool success = saveManager_.save(saveManager_.activeSlot(), state);
    // if (success) {
    //     std::cout << "[TitleScene] New game saved to slot " << saveManager_.activeSlot() << std::endl;
    // } else {
    //     std::cerr << "[TitleScene] Failed to save new game" << std::endl;
    // }

    // Update globalGameState so the new game actually starts
    globalGameState = state;

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

    // Create main menu buttons
    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;

    // Start Game button (was NEW GAME)
    newGameButton_ = new Button({centerX, centerY - buttonHeight/2}, buttonWidth, buttonHeight, "START GAME");
    newGameButton_->setAnchor("center");
    newGameButton_->setFontSize(20);
    newGameButton_->setOnClick([this]() { onNewGame(); });

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

    // Draw title text
    const char* title = "HEXAGOTCHI";
    int titleWidth = MeasureText(title, 48);
    DrawText(title, (int)((GAME_W - titleWidth) / 2.0f), 100, 48, {200, 200, 255, 255});

    // Draw credit text under Start Game button
    const char* credits = "Game by\nTomobobo and Bazola";
    int creditsHeight = 20;
    float centerY = (float)GAME_H / 2.0f;
    float buttonHeight = 50.0f;
    DrawText(credits, (int)((GAME_W - MeasureText(credits, 20)) / 2.0f), (int)(centerY + buttonHeight/2 + 30), 20, {180, 180, 220, 200});

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
