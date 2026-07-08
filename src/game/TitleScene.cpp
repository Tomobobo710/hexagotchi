#include "TitleScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "SceneManager.hpp"
#include <functional>

// Global scene manager pointer (extern'd in main.cpp)
extern SceneManager* sceneManager;

TitleScene::TitleScene()
    : Scene(720.0f, 720.0f, {0, 0, 0, 255}) {
    // Black background is set via backgroundColor in constructor
}

void TitleScene::init() {
    // Center the button on screen
    float centerX = (float)GAME_W / 2.0f;
    float centerY = (float)GAME_H / 2.0f;

    // Create Start Game button
    startButton = new Button({centerX, centerY}, 240.0f, 60.0f, "START GAME");
    startButton->setAnchor("center");
    startButton->setFontSize(24);

    // Set up click callback to transition to GotchiScene
    startButton->setOnClick([this]() {
        if (sceneManager) {
            sceneManager->switchScene("gotchi", TransitionEffect::FADE, 0.5f);
        }
    });

    // Center camera
    getCamera()->setBoundary(0, 0, (float)GAME_W, (float)GAME_H);
}

void TitleScene::update(float deltaTime) {
    Scene::update(deltaTime);

    // Update the button
    if (startButton) {
        startButton->update(getInputHandler(), deltaTime);
    }
}

void TitleScene::draw() {
    Scene::draw();

    // Draw title text
    const char* title = "HEXAGOTCHI";
    int titleWidth = MeasureText(title, 48);
    DrawText(title, (int)((GAME_W - titleWidth) / 2.0f), 200, 48, {200, 200, 255, 255});

    // Draw subtext
    const char* subtext = "Press 9 for Title Screen";
    int subtextWidth = MeasureText(subtext, 14);
    DrawText(subtext, (int)((GAME_W - subtextWidth) / 2.0f), GAME_H - 30, 14, {100, 100, 150, 200});

    // Draw instructions at top
    DrawText("1: Game  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  9: Title  ESC: Exit",
             16, 8, 12, {140, 140, 180, 255});

    // Draw button
    if (startButton) {
        startButton->draw();
    }
}

void TitleScene::cleanup() {
    Scene::cleanup();
    if (startButton) {
        delete startButton;
        startButton = nullptr;
    }
}
