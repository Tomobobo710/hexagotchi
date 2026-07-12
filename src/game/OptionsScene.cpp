#include "OptionsScene.hpp"
#include "GameConstants.hpp"
#include "Config.hpp"
#include <cmath>
#include <cstdio>

static const Color OPTIONS_BG_COLOR = {18, 14, 24, 255};

// Layout constants (720x720 space).
static const float STEP_BTN_SIZE   = 48.0f;   // "<" / ">" square buttons
static const float SELECT_WIDTH    = 200.0f;  // dialog-speed cycling button
static const float SELECT_HEIGHT   = 48.0f;
static const float VALUE_GAP       = 90.0f;   // space reserved between "<" and ">" for the value readout
static const float ROW_LABEL_X     = 90.0f;   // left edge where a row's name is drawn
static const float CONTROLS_CENTER = 470.0f;  // x-center of the stepper/select cluster


OptionsScene::OptionsScene(SceneManager* manager)
    : Scene((float)GAME_W, (float)GAME_H, OPTIONS_BG_COLOR), sceneManager(manager) {
}

void OptionsScene::styleControl(Button* b) {
    b->setAnchor("center");
    b->setFontSize(20);
    b->setBackgroundColor({60, 60, 100, 220});
    b->setHoverColor({100, 100, 160, 240});
    b->setPressedColor({40, 40, 80, 240});
    b->setBorderColor({150, 150, 200, 255});
}

void OptionsScene::refreshDialogSpeedLabel() {
    if (!dialogSpeed_) return;
    switch (GetDialogSpeed()) {
        case DialogSpeed::Off:    dialogSpeed_->setLabel("OFF");    break;
        case DialogSpeed::Normal: dialogSpeed_->setLabel("NORMAL"); break;
        case DialogSpeed::Fast:   dialogSpeed_->setLabel("FAST");   break;
    }
}

void OptionsScene::init() {
    // --- Back button, top-left (matches the HexViewScene pattern) ---
    backButton_ = std::unique_ptr<Button>(new Button({20.0f, 40.0f}, 200.0f, 64.0f, "BACK"));
    backButton_->setAnchor("top-left");
    backButton_->setFontSize(28);
    backButton_->setBackgroundColor({60, 60, 100, 220});
    backButton_->setHoverColor({100, 100, 160, 240});
    backButton_->setBorderColor({150, 150, 200, 255});
    backButton_->setOnClick([this]() {
        if (sceneManager) sceneManager->switchScene("title");
    });

    // --- Rows ---
    float firstRowY = 260.0f;
    float rowSpacing = 110.0f;
    musicRowY_  = firstRowY;
    sfxRowY_    = firstRowY + rowSpacing;
    dialogRowY_ = firstRowY + rowSpacing * 2;

    float leftX  = CONTROLS_CENTER - (VALUE_GAP / 2.0f) - (STEP_BTN_SIZE / 2.0f);
    float rightX = CONTROLS_CENTER + (VALUE_GAP / 2.0f) + (STEP_BTN_SIZE / 2.0f);

    // Music stepper
    musicDown_ = std::unique_ptr<Button>(new Button({leftX,  musicRowY_}, STEP_BTN_SIZE, STEP_BTN_SIZE, "<"));
    musicUp_   = std::unique_ptr<Button>(new Button({rightX, musicRowY_}, STEP_BTN_SIZE, STEP_BTN_SIZE, ">"));
    styleControl(musicDown_.get());
    styleControl(musicUp_.get());
    musicDown_->setOnClick([]() { SetMusicVolume(GetMusicVolume() - 1); Config::Save(); });
    musicUp_->setOnClick([]()   { SetMusicVolume(GetMusicVolume() + 1); Config::Save(); });

    // SFX stepper
    sfxDown_ = std::unique_ptr<Button>(new Button({leftX,  sfxRowY_}, STEP_BTN_SIZE, STEP_BTN_SIZE, "<"));
    sfxUp_   = std::unique_ptr<Button>(new Button({rightX, sfxRowY_}, STEP_BTN_SIZE, STEP_BTN_SIZE, ">"));
    styleControl(sfxDown_.get());
    styleControl(sfxUp_.get());
    sfxDown_->setOnClick([]() { SetSfxVolume(GetSfxVolume() - 1); Config::Save(); });
    sfxUp_->setOnClick([]()   { SetSfxVolume(GetSfxVolume() + 1); Config::Save(); });

    // Dialog-speed cycling select
    dialogSpeed_ = std::unique_ptr<Button>(new Button({CONTROLS_CENTER, dialogRowY_}, SELECT_WIDTH, SELECT_HEIGHT, "NORMAL"));
    styleControl(dialogSpeed_.get());
    dialogSpeed_->setOnClick([this]() {
        switch (GetDialogSpeed()) {
            case DialogSpeed::Off:    SetDialogSpeed(DialogSpeed::Normal); break;
            case DialogSpeed::Normal: SetDialogSpeed(DialogSpeed::Fast);   break;
            case DialogSpeed::Fast:   SetDialogSpeed(DialogSpeed::Off);    break;
        }
        refreshDialogSpeedLabel();
        Config::Save();
    });
    refreshDialogSpeedLabel();

    // Reset-to-defaults button, centered below the three rows.
    float resetY = dialogRowY_ + 110.0f;
    resetButton_ = std::unique_ptr<Button>(new Button({(float)GAME_W / 2.0f, resetY}, 300.0f, SELECT_HEIGHT, "RESET TO DEFAULTS"));
    styleControl(resetButton_.get());
    resetButton_->setOnClick([this]() {
        Config::ResetToDefaults();   // restores default settings + saves
        refreshDialogSpeedLabel();   // pull the reset speed back into the label
    });
}

void OptionsScene::update(float deltaTime) {
    Scene::update(deltaTime);

    SceneInputHandler* input = getInputHandler();

    // Gray out volume steppers at their bounds (0 and max) so it's clear when
    // you can't step further.
    if (musicDown_) musicDown_->setEnabled(GetMusicVolume() > 0);
    if (musicUp_)   musicUp_->setEnabled(GetMusicVolume() < VOLUME_LEVEL_MAX);
    if (sfxDown_)   sfxDown_->setEnabled(GetSfxVolume() > 0);
    if (sfxUp_)     sfxUp_->setEnabled(GetSfxVolume() < VOLUME_LEVEL_MAX);

    if (backButton_)  backButton_->update(input, deltaTime);
    if (musicDown_)   musicDown_->update(input, deltaTime);
    if (musicUp_)     musicUp_->update(input, deltaTime);
    if (sfxDown_)     sfxDown_->update(input, deltaTime);
    if (sfxUp_)       sfxUp_->update(input, deltaTime);
    if (dialogSpeed_) dialogSpeed_->update(input, deltaTime);
    if (resetButton_) resetButton_->update(input, deltaTime);
}

void OptionsScene::draw() {
    Scene::draw();

    // Title
    const char* title = "OPTIONS";
    int titleWidth = MeasureText(title, 40);
    DrawText(title, (int)((float)GAME_W / 2.0f - titleWidth / 2.0f), 140, 40, {220, 220, 240, 255});

    Color labelColor = {200, 200, 220, 255};
    Color valueColor = {255, 255, 255, 255};

    // Row labels (left-aligned), vertically centered on each control row.
    DrawText("Music",       (int)ROW_LABEL_X, (int)(musicRowY_  - 12), 24, labelColor);
    DrawText("Sound FX",    (int)ROW_LABEL_X, (int)(sfxRowY_    - 12), 24, labelColor);
    DrawText("Auto Dialog", (int)ROW_LABEL_X, (int)(dialogRowY_ - 12), 24, labelColor);

    // Volume readouts as "N/10", centered between the arrows.
    char buf[16];
    std::snprintf(buf, sizeof(buf), "%d/%d", GetMusicVolume(), VOLUME_LEVEL_MAX);
    int w = MeasureText(buf, 24);
    DrawText(buf, (int)(CONTROLS_CENTER - w / 2.0f), (int)(musicRowY_ - 12), 24, valueColor);

    std::snprintf(buf, sizeof(buf), "%d/%d", GetSfxVolume(), VOLUME_LEVEL_MAX);
    w = MeasureText(buf, 24);
    DrawText(buf, (int)(CONTROLS_CENTER - w / 2.0f), (int)(sfxRowY_ - 12), 24, valueColor);

    if (backButton_)  backButton_->draw();
    if (musicDown_)   musicDown_->draw();
    if (musicUp_)     musicUp_->draw();
    if (sfxDown_)     sfxDown_->draw();
    if (sfxUp_)       sfxUp_->draw();
    if (dialogSpeed_) dialogSpeed_->draw();
    if (resetButton_) resetButton_->draw();
}

void OptionsScene::cleanup() {
    Scene::cleanup();
    backButton_.reset();
    musicDown_.reset();
    musicUp_.reset();
    sfxDown_.reset();
    sfxUp_.reset();
    dialogSpeed_.reset();
    resetButton_.reset();
}
