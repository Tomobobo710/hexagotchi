#ifndef TUTORIAL_CONTROLLER_HPP
#define TUTORIAL_CONTROLLER_HPP

#include "GameState.h"
#include "GotchiDialogBox.hpp"
#include <string>
#include <vector>
#include <functional>

// ============================================================================
// TutorialController -- the new-game hand-holding walkthrough
// ============================================================================
//
// Runs once per playthrough (gated on GameState flag "tutorial_seen"), and
// spans two scenes: GotchiScene (care buttons), then HexViewScene (hexboard
// click-to-move). It owns the one GotchiDialogBox instance used throughout,
// since only one gotchi is ever talking at a time regardless of which scene
// is active -- unlike DialogBox, which is shared across many different Tom's
// World scenes/speakers, there's no need for each scene to own its own copy.
//
// STEP MODEL: a flat ordered list of TutorialStep. Each step shows one line
// of dialog. A step either:
//   - advances on space bar alone (pure narration), or
//   - requires the player to perform a specific action first (actionId
//     matches a string GotchiScene/HexViewScene report via reportAction()),
//     and only offers the space-bar advance once that action has fired.
// This is intentionally a flat vector, not a state machine per scene --
// scenes don't need to know tutorial step numbers, they just report which
// named actions were performed and ask isActionUnlocked(id) to decide what
// to enable.
//
// BUTTON LOCKING: GotchiScene/HexViewScene ask isActionUnlocked("wash") etc.
// each frame to decide Button::setEnabled(). The "Merge" button is a hard
// exception -- it stays locked (isActionUnlocked("merge") always returns
// false) even after the tutorial finishes; merge's own gating is a
// separate, later feature and must be explicitly unlocked outside this
// controller once that's designed.
// ============================================================================

struct TutorialStep {
    std::string text;          // Line shown in the GotchiDialogBox
    std::string actionId;      // Empty = pure narration (space advances immediately).
                                // Non-empty = must reportAction(actionId) once before
                                // the step will accept the space-bar advance.
    std::string scene;         // Which scene this step belongs to ("gotchi" or "hexboard") --
                                // used only to pick the step's dialog box screen position.
};

class TutorialController {
public:
    TutorialController(GameState& state);

    // True if the tutorial should run for this playthrough (i.e. hasn't been
    // completed before). Checked once, at new-game time.
    bool shouldRun() const;

    // Begins the step sequence; called once when entering GotchiScene on a
    // fresh new game (shouldRun() true).
    void start();

    // Call every frame while the tutorial is active (from whichever scene is
    // current). dt drives the dialog box's char reveal.
    void update(float deltaTime);
    void draw();

    bool isActive() const { return active_; }

    // The scene ("gotchi"/"hexboard") the current step belongs to -- lets a
    // scene's update()/draw() know whether the tutorial dialog is "theirs"
    // to advance/draw right now.
    const std::string& currentScene() const;

    // A scene calls this once when the player performs the named action
    // (e.g. GotchiScene::handleGotchiAction("Wash") -> reportAction("wash")).
    // No-op if the tutorial isn't active or the action doesn't match the
    // current step.
    void reportAction(const std::string& actionId);

    // Whether the named action's button should be enabled right now. Scenes
    // call this per-button every frame. "merge" always returns false while
    // the tutorial is active, per the hard lock above.
    bool isActionUnlocked(const std::string& actionId) const;

    // True once every step has been shown and advanced past -- the scene
    // that owns the final step should switch away (e.g. into free play) when
    // this flips true, and the caller should mark the flag persisted via
    // finish().
    bool isFinished() const;

    // Marks GameState's "tutorial_seen" flag so shouldRun() is false for the
    // rest of this (and future) playthroughs, and clears active_.
    void finish();

private:
    GameState& state_;
    GotchiDialogBox dialog_;
    std::vector<TutorialStep> steps_;
    int stepIndex_ = 0;
    bool active_ = false;
    bool currentActionDone_ = false;

    void showCurrentStep();
    void advanceStep();
};

#endif // TUTORIAL_CONTROLLER_HPP
