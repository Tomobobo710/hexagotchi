#ifndef TITLE_SCENE_HPP
#define TITLE_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "SaveManager.h"
#include <string>
#include <vector>

// Scene 9 - Title screen scene
// Shows a black background with "New Game" and "Load Game" buttons
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
    std::vector<Button*> saveButtons_;

    SaveManager saveManager_;

    // Current mode: false = main menu, true = load selection
    bool showingLoadOptions_;

    // Button creation helper
    Button* createButton(const std::string& label, float x, float y, float width = 200.0f, float height = 40.0f);

    // Callbacks for buttons
    void onNewGame();
    void onLoadGame();
    void onLoadFromSlot(int slot);
    void goBackToMainMenu();
};

#endif // TITLE_SCENE_HPP
