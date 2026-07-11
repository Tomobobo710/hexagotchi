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
        // Background is wider than the GAME_W x GAME_H canvas -- pan all the
        // way right (show the texture's rightmost GAME_W-wide slice) instead
        // of drawing it pinned to its top-left corner.
        int xOffset = GAME_W - background_.width;  // <= 0, shifts the image left
        DrawTexture(background_, xOffset, 0, WHITE);
    }

    const char* title = "GAME OVER";
    int titleSize = 56;
    int titleWidth = MeasureText(title, titleSize);
    DrawText(title, (GAME_W - titleWidth) / 2, GAME_H / 2 - 60, titleSize, RED);

    const char* tip = "Tip: Keep your gotchi happy to progress.";
    int tipSize = 20;
    int tipWidth = MeasureText(tip, tipSize);
    int tipX = (GAME_W - tipWidth) / 2;
    int tipY = GAME_H / 2 + 20;

    // Pill background behind the tip so it reads over the busy art.
    int pillPaddingX = 18;
    int pillPaddingY = 10;
    Rectangle pill = {
        (float)(tipX - pillPaddingX), (float)(tipY - pillPaddingY),
        (float)(tipWidth + pillPaddingX * 2), (float)(tipSize + pillPaddingY * 2)
    };
    DrawRectangleRounded(pill, 0.5f, 8, {0, 0, 0, 160});

    DrawText(tip, tipX, tipY, tipSize, {220, 220, 220, 255});
}

void DeathScene::cleanup() {
    Scene::cleanup();
    if (background_.id != 0) {
        UnloadTexture(background_);
        background_ = {0};
    }
}
