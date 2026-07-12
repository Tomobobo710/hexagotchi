#ifndef OPTIONS_SCENE_HPP
#define OPTIONS_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "SceneManager.hpp"
#include <vector>
#include <memory>
#include <string>

// Global options menu, reachable from the title screen. Adjusts the
// header-only global settings in GameConstants.hpp (music/sfx volume, dialog
// auto-advance speed) -- see MusicVolumeState()/SfxVolumeState()/
// DialogSpeedState(). Every change is persisted immediately via Config::Save()
// (localStorage on web, settings.ini on desktop), so settings survive across
// launches. A "Reset to Defaults" button restores the default settings.
//
// Controls are composed from plain Buttons:
//   - a volume STEPPER is a "<" and ">" button pair; the current value is
//     drawn as a label between them (0.1 increments, 0..1).
//   - the dialog-speed SELECT is a single button that cycles
//     OFF -> NORMAL -> FAST -> OFF; its label shows the current option.
//   - a BACK button, top-left, returns to the title.
class OptionsScene : public Scene {
public:
    OptionsScene(SceneManager* manager);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    SceneManager* sceneManager;

    std::unique_ptr<Button> backButton_;

    // Music volume stepper
    std::unique_ptr<Button> musicDown_;
    std::unique_ptr<Button> musicUp_;

    // SFX volume stepper
    std::unique_ptr<Button> sfxDown_;
    std::unique_ptr<Button> sfxUp_;

    // Dialog-speed cycling select
    std::unique_ptr<Button> dialogSpeed_;

    // Reset-to-defaults button (restores default settings, keeps tutorial_seen).
    std::unique_ptr<Button> resetButton_;

    // Y positions of each row's centerline, set in init(), reused by draw()
    // to place the row labels/values next to their controls.
    float musicRowY_  = 0.0f;
    float sfxRowY_    = 0.0f;
    float dialogRowY_ = 0.0f;

    // Refreshes the dialog-speed button label from the current global setting.
    void refreshDialogSpeedLabel();

    // Shared styling for a control button.
    void styleControl(Button* b);
};

#endif // OPTIONS_SCENE_HPP
