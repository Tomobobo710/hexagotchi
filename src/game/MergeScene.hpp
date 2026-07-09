#ifndef MERGE_SCENE_HPP
#define MERGE_SCENE_HPP

#include "Scene.hpp"
#include "PortalEffect.hpp"

// Transition screen played when Tom crosses between his world and the
// Tomagotchi device -- just the office's portal effect, camera dashing
// through it, clear color fading to/from black. No background art, no
// actors: the portal ring/base/membrane (same PortalEffect used in
// OfficeScene) is the entire visual.
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

    PortalEffect* portal = nullptr;
    Mode mode = Mode::MERGE_IN;
    float elapsed = 0.0f;
};

#endif // MERGE_SCENE_HPP
