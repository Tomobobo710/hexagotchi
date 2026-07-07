#include "GotchiScene.hpp"
#include "AssetPack.hpp"
#include "SpriteLoader.hpp"
#include "GameConstants.hpp"
#include <iostream>

GotchiScene::GotchiScene()
    : Scene(720.0f, 720.0f, {40, 40, 60, 255}) {
    gotchiDir = "gotchis/001";
}

void GotchiScene::init() {
    // Center camera
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);

    // Create Gotchi at center of screen
    gotchi = new Gotchi({360.0f, 360.0f});
    gotchi->setTag("gotchi");
    gotchi->setScale({4.0f, 4.0f});  // Scale up for visibility
    addActor(gotchi);

    // Initialize the Gotchi
    gotchi->init();

    // Load animation frames using the Gotchi's loader
    // This populates the Gotchi's internal frame arrays for each action
    gotchi->loadAnimationFrames(gotchiDir);

    // Set initial action (will use the loaded frames)
    gotchi->setAction("idle");
}

void GotchiScene::update(float deltaTime) {
    Scene::update(deltaTime);

    // Update the Gotchi every frame
    if (gotchi) {
        gotchi->update(deltaTime);
    }
}

void GotchiScene::draw() {
    Scene::draw();

    // Draw scene title and instructions
    DrawText("GOTCHI SCENE", 16, 8, 18, {180, 180, 255, 255});
    DrawText("1: Game  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  ESC: Exit", GAME_W - 320, 8, 12, {140, 140, 180, 255});

    // Draw stats in bottom left
    if (gotchi) {
        int ly = 100;
        DrawText("GOTCHI STATS", 16, ly, 14, {100, 200, 255, 255});
        ly += 18;

        DrawText(("Hunger: " + std::to_string(static_cast<int>(100 - gotchi->getStats().getNormalizedStat(SecondaryStat::FOOD_LEVEL) * 100))).c_str(), 16, ly, 12, {200, 200, 200, 255});
        ly += 16;
        DrawText(("Energy: " + std::to_string(static_cast<int>(gotchi->getStats().getNormalizedStat(SecondaryStat::ENERGY) * 100))).c_str(), 16, ly, 12, {200, 200, 200, 255});
        ly += 16;
        DrawText(("Happiness: " + std::to_string(static_cast<int>(gotchi->getStats().getNormalizedStat(EmotionalStat::HAPPINESS) * 100))).c_str(), 16, ly, 12, {200, 200, 200, 255});
        ly += 16;
        DrawText(("Health: " + std::to_string(static_cast<int>(gotchi->getStats().getNormalizedStat(SecondaryStat::FITALITY) * 100))).c_str(), 16, ly, 12, {200, 200, 200, 255});
        ly += 16;
        DrawText(("Mood: " + gotchi->getMood().getMoodName()).c_str(), 16, ly, 12, {200, 200, 200, 255});
    }
}

void GotchiScene::cleanup() {
    Scene::cleanup();
    gotchi = nullptr;
}
