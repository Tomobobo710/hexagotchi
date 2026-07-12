#ifndef SPLASH_SCREEN_SCENE_HPP
#define SPLASH_SCREEN_SCENE_HPP

#include "Scene.hpp"
#include "raylib.h"
#include <vector>
#include <string>

class SceneManager;

// The very first scene the game boots into. Plays a sequence of company/jam
// logos, each fading in from black, holding, then fading out to black, with a
// black gap between them -- then switches to the title screen. Pressing SPACE
// or clicking at any point skips the whole sequence straight to the title.
//
// Uses the standard 1280x720 scene world with the camera centered at zoom 1
// (same framing as the story scenes) -- each 720p logo fills the world, and the
// centered camera shows the vertically-full middle slice with no letterbox
// bars. Black clear color, no music.
class SplashScreenScene : public Scene {
public:
    SplashScreenScene(SceneManager* manager);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    SceneManager* sceneManager_;

    // One logo's art. Loaded in init(), unloaded in cleanup().
    std::vector<Texture2D> logos_;

    // Per-logo timing (seconds): fade in -> hold -> fade out, then a black gap
    // before the next. Tuned per Tom: ~0.5 / 1.2 / 0.5, ~7s total for three.
    static constexpr float FADE_IN   = 0.5f;
    static constexpr float HOLD      = 1.2f;
    static constexpr float FADE_OUT  = 0.5f;
    static constexpr float GAP       = 0.3f;   // black between logos

    int   index_   = 0;      // which logo is showing
    float elapsed_ = 0.0f;   // seconds into the current logo's cycle
    bool  done_    = false;  // sequence finished / skipped -> switch pending

    // Skip / finish -> go to the title. Idempotent.
    void goToTitle();

    // Alpha (0..255) for the current logo at `elapsed_`, following the
    // fade-in/hold/fade-out envelope. Returns 0 during the trailing GAP.
    unsigned char currentAlpha() const;
};

#endif // SPLASH_SCREEN_SCENE_HPP
