#include "DeathScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "GameState.h"
#include "SceneManager.hpp"
#include "ToyAnimationScene.hpp"
#include "SceneInputHandler.hpp"
#include "TutorialController.hpp"

DeathScene::DeathScene()
    : Scene((float)GAME_W, (float)GAME_H, BLACK) {
    // Scene's base ctor already centers the camera at (w/2, h/2) with zoom
    // 1.0 -- no explicit setPosition()/setZoom() needed for this screen.
}

void DeathScene::init() {
    getCamera()->setBoundary(0, 0, (float)GAME_W, (float)GAME_H);
    background_ = AssetPack::loadTexture("backgrounds/deathbg.png");

    float buttonWidth = 200.0f;
    float buttonHeight = 56.0f;
    Vector2 pos = { (GAME_W - buttonWidth) / 2.0f, (float)GAME_H / 2.0f + 90.0f };
    tryAgainButton_ = std::unique_ptr<Button>(new Button(pos, buttonWidth, buttonHeight, "TRY AGAIN"));
    tryAgainButton_->setAnchor("top-left");
    tryAgainButton_->setFontSize(22);
    tryAgainButton_->setBackgroundColor({60, 60, 100, 220});
    tryAgainButton_->setHoverColor({100, 100, 160, 240});
    tryAgainButton_->setBorderColor({150, 150, 200, 255});
    tryAgainButton_->setOnClick([this]() { onTryAgain(); });
}

void DeathScene::update(float deltaTime) {
    Scene::update(deltaTime);
    if (tryAgainButton_) {
        tryAgainButton_->update(getInputHandler(), deltaTime);
    }
}

void DeathScene::onTryAgain() {
    // Fresh-run reset that overwrites the shared global by value, so every
    // scene/controller holding a GameState* sees it immediately (they only ever
    // hold the pointer, never a copy). PRESERVES tutorial_seen -- a Try Again is
    // the same player restarting, so they shouldn't be re-walked through the
    // tutorial. Audio/dialog preferences are external (GameConstants globals),
    // so they survive this untouched.
    ResetRunKeepingTutorial(globalGameState);

    if (getSceneManager()) {
        SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
        mgr->switchScene("toy_animation");
        ToyAnimationScene* toyAnim = static_cast<ToyAnimationScene*>(mgr->getScene("toy_animation"));
        if (toyAnim) toyAnim->startIntro("gotchi");
    }
}

void DeathScene::draw() {
    Scene::draw();

    if (background_.id != 0) {
        // Background is wider than the GAME_W x GAME_H canvas. Panned left by
        // 400px from the fully-right-panned position so more of the left
        // side of the art is visible -- clamped so it never pans past the
        // texture's left edge (xOffset can't go above 0).
        int xOffset = GAME_W - background_.width + 400;  // shifts the visible window left
        if (xOffset > 0) xOffset = 0;
        DrawTexture(background_, xOffset, 0, WHITE);
    }

    const char* title = "GAME OVER";
    int titleSize = 56;
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GAME_W - titleWidth) / 2;
    int titleY = 40;

    int titlePillPaddingX = 24;
    int titlePillPaddingY = 14;
    Rectangle titlePill = {
        (float)(titleX - titlePillPaddingX), (float)(titleY - titlePillPaddingY),
        (float)(titleWidth + titlePillPaddingX * 2), (float)(titleSize + titlePillPaddingY * 2)
    };
    DrawRectangleRounded(titlePill, 0.3f, 8, {0, 0, 0, 170});

    DrawText(title, titleX, titleY, titleSize, RED);

    const char* tip = "Tip: Keep your gotchi happy to progress.";
    int tipSize = 20;
    int tipWidth = MeasureText(tip, tipSize);
    int tipX = (GAME_W - tipWidth) / 2;
    int tipY = GAME_H - 50;

    // Pill background behind the tip so it reads over the busy art.
    int pillPaddingX = 18;
    int pillPaddingY = 10;
    Rectangle pill = {
        (float)(tipX - pillPaddingX), (float)(tipY - pillPaddingY),
        (float)(tipWidth + pillPaddingX * 2), (float)(tipSize + pillPaddingY * 2)
    };
    DrawRectangleRounded(pill, 0.5f, 8, {0, 0, 0, 160});

    DrawText(tip, tipX, tipY, tipSize, {220, 220, 220, 255});

    if (tryAgainButton_) {
        tryAgainButton_->draw();
    }
}

void DeathScene::cleanup() {
    Scene::cleanup();
    if (background_.id != 0) {
        UnloadTexture(background_);
        background_ = {0};
    }
    tryAgainButton_.reset();
}
