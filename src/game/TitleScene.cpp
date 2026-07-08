#include "TitleScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "SceneManager.hpp"
#include "GotchiScene.hpp"
#include "GameState.h"
#include <functional>
#include <fstream>
#include <sstream>
#include <iostream>

// Global scene manager pointer (extern'd in main.cpp)
extern SceneManager* sceneManager;

TitleScene::TitleScene()
    : Scene(720.0f, 720.0f, {0, 0, 0, 255}),
      showingLoadOptions_(false) {
    saveManager_.initSaveDir();
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

void TitleScene::onNewGame() {
    // Create a new GameState with default values
    GameState state;
    state.version = SAVE_VERSION;
    state.mode = Mode::Gotchi;

    // Save to slot 0 (default slot for new games)
    bool success = saveManager_.save(0, state);
    if (success) {
        saveManager_.setActiveSlot(0);
        std::cout << "New game saved to slot 0" << std::endl;
    } else {
        std::cerr << "Failed to save new game" << std::endl;
    }

    // Transition to gotchi scene
    if (sceneManager) {
        sceneManager->switchScene("gotchi", TransitionEffect::FADE, 0.5f);
    }
}

void TitleScene::onLoadGame() {
    // Switch to load selection mode
    showingLoadOptions_ = true;
}

void TitleScene::onLoadFromSlot(int slot) {
    // Try to load from the specified slot
    auto loadedState = saveManager_.load(slot);
    if (loadedState.has_value()) {
        GameState& state = loadedState.value();
        std::cout << "Game loaded from slot " << slot << ", version: " << state.version << std::endl;

        // Set as active slot for autosaves
        saveManager_.setActiveSlot(slot);

        // Transition to gotchi scene
        if (sceneManager) {
            sceneManager->switchScene("gotchi", TransitionEffect::FADE, 0.5f);
        }
    } else {
        std::cerr << "No save found in slot " << slot << std::endl;
    }
}

void TitleScene::goBackToMainMenu() {
    showingLoadOptions_ = false;
}

void TitleScene::init() {
    float centerX = (float)GAME_W / 2.0f;
    float centerY = (float)GAME_H / 2.0f;

    // Create main menu buttons
    float buttonWidth = 200.0f;
    float buttonHeight = 50.0f;
    float spacing = 20.0f;

    newGameButton_ = new Button({centerX, centerY - buttonHeight/2 - spacing/2}, buttonWidth, buttonHeight, "NEW GAME");
    newGameButton_->setAnchor("center");
    newGameButton_->setFontSize(20);
    newGameButton_->setOnClick([this]() { onNewGame(); });

    loadGameButton_ = new Button({centerX, centerY + buttonHeight/2 + spacing/2}, buttonWidth, buttonHeight, "LOAD GAME");
    loadGameButton_->setAnchor("center");
    loadGameButton_->setFontSize(20);
    loadGameButton_->setOnClick([this]() { onLoadGame(); });

    // Create load slot buttons (hidden by default)
    float slotButtonWidth = 180.0f;
    float slotButtonHeight = 36.0f;
    float slotSpacing = 8.0f;
    float slotStartY = centerY - (SaveManager::NUM_SLOTS * (slotButtonHeight + slotSpacing)) / 2 + slotButtonHeight/2;

    for (int i = 0; i < SaveManager::NUM_SLOTS; i++) {
        float yPos = slotStartY + i * (slotButtonHeight + slotSpacing);
        Button* btn = createButton("", centerX, yPos, slotButtonWidth, slotButtonHeight);
        int slot = i;
        btn->setOnClick([this, slot]() { onLoadFromSlot(slot); });
        saveButtons_.push_back(btn);
    }

    // Back button (hidden by default)
    Button* backBtn = createButton("BACK", centerX, centerY + 120, 120, 36);
    backBtn->setFontSize(14);
    backBtn->setOnClick([this]() { goBackToMainMenu(); });
    saveButtons_.push_back(backBtn);

    // Center camera
    getCamera()->setBoundary(0, 0, (float)GAME_W, (float)GAME_H);
}

void TitleScene::update(float deltaTime) {
    Scene::update(deltaTime);

    SceneInputHandler* input = getInputHandler();

    // Update main menu buttons
    if (newGameButton_) {
        newGameButton_->update(input, deltaTime);
    }
    if (loadGameButton_) {
        loadGameButton_->update(input, deltaTime);
    }

    // Update load slot buttons
    for (Button* btn : saveButtons_) {
        btn->update(input, deltaTime);
    }
}

void TitleScene::draw() {
    Scene::draw();

    // Draw title text
    const char* title = "HEXAGOTCHI";
    int titleWidth = MeasureText(title, 48);
    DrawText(title, (int)((GAME_W - titleWidth) / 2.0f), 100, 48, {200, 200, 255, 255});

    // Draw subtext
    const char* subtext = "Press 9 for Title Screen";
    int subtextWidth = MeasureText(subtext, 14);
    DrawText(subtext, (int)((GAME_W - subtextWidth) / 2.0f), GAME_H - 30, 14, {100, 100, 150, 200});

    // Draw instructions at top
    DrawText("1: Game  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  9: Title  ESC: Exit",
             16, 8, 12, {140, 140, 180, 255});

    // Draw "Save System" header
    DrawText("SAVE SYSTEM", 30, 20, 16, {140, 180, 255, 255});

    if (showingLoadOptions_) {
        // Draw slot selection UI
        for (int i = 0; i < SaveManager::NUM_SLOTS; i++) {
            SlotSummary summary = saveManager_.summary(i);
            std::string label;
            if (summary.exists) {
                label = "Slot " + std::to_string(i + 1) + ": Load (v" + std::to_string(summary.version) + ")";
            } else {
                label = "Slot " + std::to_string(i + 1) + ": Empty";
            }

            // Update button label
            if (i < (int)saveButtons_.size()) {
                saveButtons_[i]->setLabel(label);
                saveButtons_[i]->setVisible(true);
                saveButtons_[i]->setEnabled(summary.exists);

                // Draw button manually to show disabled state properly
                saveButtons_[i]->draw();
            }
        }

        // Draw back button
        if (!saveButtons_.empty()) {
            saveButtons_.back()->setVisible(true);
            saveButtons_.back()->draw();
        }
    } else {
        // Draw main menu buttons
        if (newGameButton_) {
            newGameButton_->draw();
        }
        if (loadGameButton_) {
            loadGameButton_->draw();
        }

        // Hide load slot buttons
        for (Button* btn : saveButtons_) {
            btn->setVisible(false);
        }
    }
}

void TitleScene::cleanup() {
    Scene::cleanup();

    if (newGameButton_) {
        delete newGameButton_;
        newGameButton_ = nullptr;
    }
    if (loadGameButton_) {
        delete loadGameButton_;
        loadGameButton_ = nullptr;
    }

    for (Button* btn : saveButtons_) {
        delete btn;
    }
    saveButtons_.clear();
    showingLoadOptions_ = false;
}
