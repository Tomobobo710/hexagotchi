#ifndef TITLE_SCENE_HPP
#define TITLE_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
// SaveManager.h - DISABLED: Save system shut off for game jam
// #include "SaveManager.h"
#include <string>
#include <vector>

// SaveManager - DISABLED: Save system shut off for game jam
// extern SaveManager saveManager;

// Scene 9 - Title screen scene
// Shows a black background with "Start Game" button (save/load disabled for game jam)
class TitleScene : public Scene {
public:
    TitleScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    // Button groups - SAVE SYSTEM DISABLED: Save system shut off for game jam
    Button* newGameButton_ = nullptr;
    // Button* loadGameButton_ = nullptr;
    // Button* mainMenuSaveButton_ = nullptr;  // Save button for main menu
    // std::vector<Button*> slotButtons_;      // Slot summary/load buttons
    // std::vector<Button*> actionButtons_;    // Save/Delete/Back buttons

    // Use the global SaveManager from main.cpp to ensure one source of truth
    // SaveManager& saveManager_;

    // Current mode: false = main menu, true = load/save selection
    // bool showingLoadOptions_;

    // Currently selected slot for save/delete operations
    // int selectedSlot_ = -1;

    // Button creation helper
    Button* createButton(const std::string& label, float x, float y, float width = 200.0f, float height = 40.0f);

    // Callbacks for buttons
    void onNewGame();
    // void onLoadGame();  // DISABLED: Save system shut off for game jam
    // void onLoadFromSlot(int slot);  // DISABLED
    // void onSaveToSlot(int slot);  // DISABLED
    // void onDeleteSlot(int slot);  // DISABLED
    void goBackToMainMenu();
    // void refreshSlotButtons();  // DISABLED
};

#endif // TITLE_SCENE_HPP
