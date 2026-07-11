#include "DeathScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"

DeathScene::DeathScene()
    : Scene((float)GAME_W, (float)GAME_H, BLACK) {
    // Scene's base ctor already centers the camera at (w/2, h/2) with zoom
    // 1.0 -- no explicit setPosition()/setZoom() needed for this screen.
}

void DeathScene::init() {
    getCamera()->setBoundary(0, 0, (float)GAME_W, (float)GAME_H);
    background_ = AssetPack::loadTexture("backgrounds/deathbg.png");
}

void DeathScene::draw() {
    Scene::draw();

    if (background_.id != 0) {
        DrawTexture(background_, 0, 0, WHITE);
    }

    const char* title = "GAME OVER";
    int titleSize = 56;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, (GAME_W - titleWidth) / 2, GAME_H / 2 - 60, titleSize, RED);

    const char* tip = "Tip: Keep your gotchi happy to progress.";
    int tipSize = 20;
    int tipWidth = MeasureText(tip, tipSize);
    DrawText(tip, (GAME_W - tipWidth) / 2, GAME_H / 2 + 20, tipSize, {220, 220, 220, 255});
}

void DeathScene::cleanup() {
    Scene::cleanup();
    if (background_.id != 0) {
        UnloadTexture(background_);
        background_ = {0};
    }
}
