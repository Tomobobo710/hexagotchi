#include "SplashScreenScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SceneManager.hpp"
#include "SceneInputHandler.hpp"

// The logo "world" is the same 1280x720 space the story scenes use. Each logo
// is a 720p image drawn to fill that world; the camera sits at the world center
// at zoom 1, so the 720-tall framebuffer shows the vertically-full, centered
// slice -- no letterbox bars.
static const float WORLD_W = 1280.0f;
static const float WORLD_H = 720.0f;

// Logo sequence, in display order.
static const char* kLogoKeys[] = {
    "logos/action_team_logo.png",
    "logos/pixelpocalypse_company_logo.png",
    "logos/game_jam_logo.png",
};

SplashScreenScene::SplashScreenScene(SceneManager* manager)
    : Scene(WORLD_W, WORLD_H, BLACK), sceneManager_(manager) {
}

void SplashScreenScene::init() {
    getCamera()->setBoundary(0, 0, WORLD_W, WORLD_H);
    // Center the camera on the world and hold zoom at 1.0 -- fills the 720-tall
    // view with the middle 720-wide slice of the 1280-wide world (no bars).
    getCamera()->setPosition(WORLD_W * 0.5f, WORLD_H * 0.5f);
    getCamera()->setZoom(1.0f);

    logos_.clear();
    for (const char* key : kLogoKeys) {
        logos_.push_back(AssetPack::loadTexture(key));   // id 0 on miss
    }

    index_   = 0;
    elapsed_ = 0.0f;
    done_    = false;
}

unsigned char SplashScreenScene::currentAlpha() const {
    if (elapsed_ < FADE_IN) {
        return (unsigned char)(255.0f * (elapsed_ / FADE_IN));
    }
    if (elapsed_ < FADE_IN + HOLD) {
        return 255;
    }
    if (elapsed_ < FADE_IN + HOLD + FADE_OUT) {
        float t = (elapsed_ - FADE_IN - HOLD) / FADE_OUT;
        return (unsigned char)(255.0f * (1.0f - t));
    }
    return 0;   // trailing black GAP
}

void SplashScreenScene::goToTitle() {
    if (done_) return;
    done_ = true;
    if (sceneManager_) sceneManager_->switchScene("title");
}

void SplashScreenScene::update(float deltaTime) {
    Scene::update(deltaTime);
    if (done_) return;

    // Skip: SPACE or left-click at any time jumps straight to the title.
    SceneInputHandler* ih = getInputHandler();
    bool skip = IsKeyPressed(KEY_SPACE)
                || (ih && ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT));
    if (skip) {
        goToTitle();
        return;
    }

    elapsed_ += deltaTime;

    // One full cycle per logo = fade in + hold + fade out + black gap.
    const float CYCLE = FADE_IN + HOLD + FADE_OUT + GAP;
    if (elapsed_ >= CYCLE) {
        elapsed_ -= CYCLE;
        index_++;
        if (index_ >= (int)logos_.size()) {
            goToTitle();
        }
    }
}

void SplashScreenScene::draw() {
    Scene::draw();   // clears to black

    if (done_ || index_ < 0 || index_ >= (int)logos_.size()) return;

    Texture2D tex = logos_[index_];
    if (tex.id == 0) return;

    // Draw the logo to fill the whole 1280x720 world, inside the camera's 2D
    // pass -- the centered camera then frames the middle 720-wide slice. Each
    // logo is a 720p image, so it maps to the world edge-to-edge.
    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
        Rectangle src  = { 0, 0, (float)tex.width, (float)tex.height };
        Rectangle dest = { 0, 0, WORLD_W, WORLD_H };
        Color tint = WHITE;
        tint.a = currentAlpha();
        DrawTexturePro(tex, src, dest, {0, 0}, 0.0f, tint);
    EndMode2D();
}

void SplashScreenScene::cleanup() {
    Scene::cleanup();
    for (Texture2D& t : logos_) {
        if (t.id != 0) UnloadTexture(t);
    }
    logos_.clear();
}
