#ifndef SKIP_SCENE_OVERLAY_HPP
#define SKIP_SCENE_OVERLAY_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include <functional>

// A small always-present "skip" button (top-left, half the height of
// GotchiScene's main action buttons) that lets the player fast-forward past
// a story beat while in Tom's world. Clicking it opens a confirm modal
// (yes/no) rather than skipping immediately -- mirrors PauseMenuOverlay's
// dim-rect + bordered-panel draw pattern, but this overlay is global
// (main.cpp owns one instance) since skippability is a cross-scene concern,
// not something any single Scene subclass decides on its own.
class SkipSceneOverlay {
public:
    SkipSceneOverlay();

    // input: the currently-active scene's SceneInputHandler (already updated
    // this frame by that scene's own update()), same convention
    // PauseMenuOverlay uses via Scene::getInputHandler().
    // visible: true only while the active scene is a skippable story beat --
    // caller (main.cpp) decides this from scene name + StorySequencer phase.
    void update(float deltaTime, SceneInputHandler* input, bool visible);
    void draw(bool visible);

    // True if a click this frame landed on the skip button or the confirm
    // modal (either open, or the skip button itself hovered) -- callers
    // check this before letting the same click also count as a "advance the
    // dialogue line" elsewhere (main.cpp's IsPauseUiClaimingClick()).
    bool isInteractive() const { return confirmOpen || skipButton.isHovered(); }

    // Fired when the player confirms the skip. Caller wires this to
    // StorySequencer::skipCurrentStep().
    std::function<void()> onConfirmSkip;

private:
    Button skipButton;
    Button confirmYesButton;
    Button confirmNoButton;
    bool confirmOpen;
};

#endif // SKIP_SCENE_OVERLAY_HPP
