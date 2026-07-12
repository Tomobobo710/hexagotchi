#include "CreditsScene.hpp"
#include "GameConstants.hpp"
#include "GameState.h"
#include "SceneManager.hpp"
#include "ToyAnimationScene.hpp"
#include "SceneInputHandler.hpp"
#include "TutorialController.hpp"

CreditsScene::CreditsScene()
    : Scene((float)GAME_W, (float)GAME_H, BLACK) {
    // Base ctor centers the camera at (w/2, h/2), zoom 1.0 -- exactly what we
    // want; no background, black clear color.
}

void CreditsScene::init() {
    getCamera()->setBoundary(0, 0, (float)GAME_W, (float)GAME_H);

    // "PLAY AGAIN?" button -- same styling/size/placement as DeathScene's TRY
    // AGAIN, just a different label and reset entry point.
    float buttonWidth = 200.0f;
    float buttonHeight = 56.0f;
    Vector2 pos = { (GAME_W - buttonWidth) / 2.0f, (float)GAME_H / 2.0f + 90.0f };
    playAgainButton_ = std::unique_ptr<Button>(new Button(pos, buttonWidth, buttonHeight, "PLAY AGAIN?"));
    playAgainButton_->setAnchor("top-left");
    playAgainButton_->setFontSize(22);
    playAgainButton_->setBackgroundColor({60, 60, 100, 220});
    playAgainButton_->setHoverColor({100, 100, 160, 240});
    playAgainButton_->setBorderColor({150, 150, 200, 255});
    playAgainButton_->setOnClick([this]() { onPlayAgain(); });
}

void CreditsScene::update(float deltaTime) {
    Scene::update(deltaTime);
    if (playAgainButton_) {
        playAgainButton_->update(getInputHandler(), deltaTime);
    }
}

void CreditsScene::onPlayAgain() {
    // Fresh-run reset (see DeathScene::onTryAgain). Overwrites the shared global
    // by value so every GameState* holder sees it at once. PRESERVES
    // tutorial_seen -- someone who just finished the whole game replaying it
    // shouldn't be re-tutorialized. Audio/dialog preferences are external
    // globals, unaffected either way.
    ResetRunKeepingTutorial(globalGameState);

    if (getSceneManager()) {
        SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
        mgr->switchScene("toy_animation");
        ToyAnimationScene* toyAnim = static_cast<ToyAnimationScene*>(mgr->getScene("toy_animation"));
        if (toyAnim) toyAnim->startIntro("gotchi");
    }
}

void CreditsScene::draw() {
    Scene::draw();

    // "CAST" header, white, centered horizontally near the top.
    const char* title = "CAST";
    int titleSize = 56;
    int titleWidth = MeasureText(title, titleSize);
    int titleX = (GAME_W - titleWidth) / 2;
    int titleY = 40;
    DrawText(title, titleX, titleY, titleSize, WHITE);

    if (playAgainButton_) {
        playAgainButton_->draw();
    }
}

void CreditsScene::cleanup() {
    Scene::cleanup();
    playAgainButton_.reset();
}
