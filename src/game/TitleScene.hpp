#ifndef TITLE_SCENE_HPP
#define TITLE_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "SaveManager.h"
#include <string>
#include <vector>

// Forward declare the global SaveManager from main.cpp
extern SaveManager saveManager;

// Scene 9 - Title screen scene
// Shows a black background with "New Game", "Load Game" buttons and 3 save slots
class TitleScene : public Scene {
public:
    TitleScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    // Button groups
    Button* newGameButton_ = nullptr;
    Button* loadGameButton_ = nullptr;
    Button* mainMenuSaveButton_ = nullptr;  // Save button for main menu
    std::vector<Button*> slotButtons_;      // Slot summary/load buttons
    std::vector<Button*> actionButtons_;    // Save/Delete/Back buttons

    // Use the global SaveManager from main.cpp to ensure one source of truth
    SaveManager& saveManager_;

    // Current mode: false = main menu, true = load/save selection
    bool showingLoadOptions_;

    // Currently selected slot for save/delete operations
    int selectedSlot_ = -1;

    // Button creation helper
    Button* createButton(const std::string& label, float x, float y, float width = 200.0f, float height = 40.0f);

    // Callbacks for buttons
    void onNewGame();
    void onLoadGame();
    void onLoadFromSlot(int slot);
    void onSaveToSlot(int slot);
    void onDeleteSlot(int slot);
    void goBackToMainMenu();
    void refreshSlotButtons();
};

#endif // TITLE_SCENE_HPP
