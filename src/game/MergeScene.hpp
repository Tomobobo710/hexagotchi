#ifndef MERGE_SCENE_HPP
#define MERGE_SCENE_HPP

#include "Scene.hpp"
#include "PortalEffect.hpp"
#include "SceneActor.hpp"

// Transition screen played when Tom crosses between his world and the
// Tomagotchi device -- the office's portal effect, camera dashing through
// it, clear color fading to/from black, and Tom himself spiralling in/out
// of the portal on top of it. The portal ring/base/membrane (same
// PortalEffect used in OfficeScene) is the backdrop; Tom (a pose sprite on
// a SceneActor) corkscrews toward/away from the portal center, spinning and
// scaling as he goes.
//
// MERGE_IN (going into the device): camera starts far back (10) and pushes
// in to point-blank (0.1) over MERGE_DURATION seconds while the clear color
// fades black -> white.
// MERGE_OUT (coming back out): mirrored -- camera starts at 0.1 and pulls
// back to 10 while the clear color fades white -> black.
//
// Standalone scene, not tied to the office scene's own PortalEffect
// instance -- this one is entirely self-contained so it can be switched to
// from anywhere (tomagotchi side or Tom's-world side) without either scene
// needing to know about it.
class MergeScene : public Scene {
public:
    enum class Mode { MERGE_IN, MERGE_OUT };

    MergeScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    // Call after switching into this scene to pick which direction plays;
    // resets the timer back to 0. Defaults to MERGE_IN if never called.
    void startMerge(Mode mode);

    bool isFinished() const { return elapsed >= MERGE_DURATION; }

private:
    static constexpr float MERGE_DURATION = 7.0f;
    static constexpr float CAM_DIST_NEAR = 0.1f;
    static constexpr float CAM_DIST_FAR = 10.0f;

    // Spiral tuning for Tom's corkscrew into/out of the portal center.
    // SPIRAL_RADIUS_MAX is how far out (screen pixels) the spiral starts when
    // Tom is fully "out"; it lerps to ~0 as he reaches the portal. TURNS is
    // how many full loops he makes across the whole merge. SPIN_TURNS is how
    // many times his own sprite tumbles end-over-end on top of the orbit.
    // SCALE_MAX is his sprite scale when fully "out"; it rides down to 0 at
    // the portal center so he disappears entirely into it.
    // Outer radius of the spiral (px from screen center). Pushed past the
    // 720x720 screen's half-diagonal (~509px) so the wide end of the spiral
    // carries Tom fully offscreen -- that's where MERGE_OUT settles, so his
    // final upright rest frame lands out of view instead of on screen.
    static constexpr float SPIRAL_RADIUS_MAX = 700.0f;
    // Radius easing exponent: >1 keeps the coil tight near the center for most
    // of the run and only flares wide toward the outer end. Higher = tighter
    // for longer, sharper flare at the end.
    static constexpr float RADIUS_POW = 2.5f;
    static constexpr float SPIRAL_TURNS = 6.0f;
    static constexpr float SPIN_TURNS = 5.0f;
    static constexpr float SCALE_MAX = 1.6f;

    PortalEffect* portal = nullptr;
    SceneActor* tom = nullptr;
    Texture2D tomPose = {0};
    Mode mode = Mode::MERGE_IN;
    float elapsed = 0.0f;

    // Places `tom` on the spiral for progress p in [0,1], where p=0 is fully
    // out (far, big) and p=1 is fully merged (center, tiny). Shared by both
    // modes so MERGE_OUT is just this played with p reversed.
    void placeTomOnSpiral(float p);
};

#endif // MERGE_SCENE_HPP
