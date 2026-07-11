#include "TutorialController.hpp"
#include "GameConstants.hpp"
#include "raylib.h"
#include "SceneInputHandler.hpp"

namespace {
    const std::string TUTORIAL_SEEN_FLAG = "tutorial_seen";

    // Bubble sized/positioned per scene: GotchiScene is a 720x720 tight shot
    // on the gotchi, HexViewScene is a wide world view, so the bubble sits
    // near the top-center in both rather than trying to track the gotchi's
    // (often off-screen, panning) world position.
    Vector2 bubblePosition() {
        float w = 460.0f;
        return { ((float)GAME_W - w) / 2.0f, 60.0f };
    }
}

TutorialController::TutorialController(GameState& state)
    : state_(state), dialog_(bubblePosition(), 460.0f, 150.0f) {

    steps_ = {
        { "Hi! Welcome to HexLand!", "", "gotchi" },
        { "This is me, your gotchi -- I live here on the HexMap just outside.", "", "gotchi" },
        { "Let's try the explore the HexMap first. Press SPACE to head out there.", "", "gotchi" },

        { "This is the HexMap! Click any tile to send me walking there.", "walk", "hexboard" },
        { "Nice! You can drag food or water from the palette below onto a tile, and I'll go eat or drink it.", "", "hexboard" },
        { "That's the HexMap basics. Let's head back so I can show you my home screen.", "", "hexboard" },

        { "This is my home screen. These buttons take care of me directly.", "", "gotchi" },
        { "Try Feed -- it fills me up when I'm hungry.", "Feed", "gotchi" },
        { "Try Pet -- it makes me happy.", "Pet", "gotchi" },
        { "Try Wash -- keeps me clean.", "Wash", "gotchi" },
        { "Explore, you've already seen. It'll take us to HexLand's HexMap!", "", "gotchi" },
        { "That button.. hmm.. I'm not sure what that one does.", "", "gotchi" },
        { "Ok I'll unlock all the buttons now!", "", "gotchi" },
        { "Hmm, one of them isn't working.", "", "gotchi" },
        { "Oh that button..", "", "gotchi" },
        { "Oh well, I'll unlock what I can.", "", "gotchi" },
    };
}

bool TutorialController::shouldRun() const {
    return !state_.getBool(TUTORIAL_SEEN_FLAG, false);
}

void TutorialController::start() {
    active_ = true;
    stepIndex_ = 0;
    currentActionDone_ = false;
    showCurrentStep();
}

void TutorialController::showCurrentStep() {
    if (stepIndex_ < 0 || stepIndex_ >= (int)steps_.size()) return;
    dialog_.setPosition(bubblePosition());
    dialog_.setText(steps_[stepIndex_].text);
    dialog_.show();
    currentActionDone_ = steps_[stepIndex_].actionId.empty();
}

const std::string& TutorialController::currentScene() const {
    static const std::string none = "";
    if (stepIndex_ < 0 || stepIndex_ >= (int)steps_.size()) return none;
    return steps_[stepIndex_].scene;
}

void TutorialController::reportAction(const std::string& actionId) {
    if (!active_) return;
    if (stepIndex_ < 0 || stepIndex_ >= (int)steps_.size()) return;
    if (steps_[stepIndex_].actionId == actionId) {
        currentActionDone_ = true;
    }
}

bool TutorialController::isActionUnlocked(const std::string& actionId) const {
    if (actionId == "merge") return false;  // hard-locked throughout the tutorial
    if (!active_) return true;
    if (stepIndex_ < 0 || stepIndex_ >= (int)steps_.size()) return true;
    return steps_[stepIndex_].actionId == actionId;
}

void TutorialController::advanceStep() {
    stepIndex_++;
    if (stepIndex_ >= (int)steps_.size()) {
        dialog_.hide();
        return;
    }
    showCurrentStep();
}

bool TutorialController::isFinished() const {
    return active_ && stepIndex_ >= (int)steps_.size();
}

void TutorialController::finish() {
    state_.setFlag(TUTORIAL_SEEN_FLAG, true);
    active_ = false;
}

void TutorialController::update(float deltaTime) {
    if (!active_) return;
    dialog_.update(deltaTime);

    if (isFinished()) return;

    bool acceptPressed = IsKeyPressed(KEY_SPACE);
    if (!acceptPressed) return;

    if (!dialog_.isFinished()) {
        dialog_.skipReveal();
        return;
    }

    // Only pure-narration steps or steps whose action already fired can
    // advance -- a step gated on an unperformed action (e.g. "Feed") simply
    // eats the space press until the player actually presses that button.
    if (currentActionDone_) {
        advanceStep();
    }
}

void TutorialController::draw() {
    if (!active_) return;
    dialog_.draw();
}
