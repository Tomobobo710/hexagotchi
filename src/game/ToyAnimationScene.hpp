#ifndef TOY_ANIMATION_SCENE_HPP
#define TOY_ANIMATION_SCENE_HPP

#include "Scene.hpp"
#include "raylib.h"
#include <string>

// Transition screen played when Tom's Gotchi/HexBoard is framed as "inside
// the toy" -- orbits/zooms the camera in on the Tomagotchi toy model until
// its screen face fills the whole frame edge-to-edge (no bezel visible), at
// which point it switches to whichever real scene was requested. That scene
// then just draws completely normally in its own 2D screen space -- there's
// no compositing/render-texture trickery, the toy's screen face IS the
// black background right at the cut, same "play an animation, then hand off"
// shape as MergeScene.
//
// Call startIntro(nextSceneName) right after switching into this scene
// (SceneManager::switchScene("toy_animation") then getScene() + this call),
// same pattern StorySequencer::startMergeTransition uses for MergeScene.
class ToyAnimationScene : public Scene {
public:
    ToyAnimationScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    // Picks which scene to switch to once the zoom-in finishes, and resets
    // the animation timer. Defaults to "gotchi" if never called (e.g. when
    // entered directly from the scene-select debug hub).
    void startIntro(const std::string& nextSceneName);

private:
    // 2x the original pacing, split across three phases so the whole thing
    // reads as a spectacle instead of a quick cut: SLIDE (camera tracks in
    // from off to the side), then RISE (orbit up and over the top), then
    // ZOOM (pull in tight on the screen at the apex). See applyCamera()'s
    // phase-fraction split for how these sequence.
    static constexpr float INTRO_DURATION = 7.2f;
    static constexpr float SLIDE_FRACTION = 0.25f;
    static constexpr float RISE_FRACTION = 0.65f;  // of the time AFTER the slide

    // Lateral start position for the slide-in -- camera begins well off to
    // the side (X) at the same dead-on distance/framing as before, and
    // tracks to dead-center (X=0) before the orbit/zoom begins.
    static constexpr float SLIDE_X_START = -5.0f;

    // Single-axis orbit (pitch only, no yaw at all -- camera stays on the
    // X=0 plane once the slide finishes) then a zoom once at the apex. End
    // values are tight enough that the toy's screen face (see
    // TomagotchiToyMesh, the black primitive) fills the entire 720x720 frame
    // with no edge/bezel visible, so the cut to the next scene reads as
    // seamless.
    static constexpr float PITCH_START_DEG = 0.0f;   // dead-on the front
    static constexpr float PITCH_END_DEG = 90.0f;    // straight down at the screen
    static constexpr float DIST_START = 4.0f;
    static constexpr float DIST_END = 0.55f;
    static constexpr float FOVY_START = 45.0f;
    static constexpr float FOVY_END = 30.0f;

    Shader unlitShader;
    Model toyModel;
    Texture2D whiteTex = {0};

    std::string nextSceneName = "gotchi";
    float elapsed = 0.0f;
    bool switched = false;

    void applyCamera(Camera3D& cam) const;
};

#endif // TOY_ANIMATION_SCENE_HPP
