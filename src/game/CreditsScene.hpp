#ifndef CREDITS_SCENE_HPP
#define CREDITS_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "raylib.h"
#include <memory>

// The end-of-game credits screen -- reached after either ending's apartment
// coda (see ScenarioDirector's mergeCount==5 sequences). Minimal for now: a
// black screen with a "CAST" header and a single "PLAY AGAIN?" button that
// fully resets globalGameState and restarts, exactly like DeathScene's TRY
// AGAIN. To be fleshed out later with real scrolling credits and cast art.
class CreditsScene : public Scene {
public:
    CreditsScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    // Credits is reached as a TERMINAL sequence step (SequenceStep::isTerminal)
    // -- StorySequencer enters this scene and then stops, so the game parks
    // here with no merge-out. No isPlayingScenario()/triggerStoryEvent()
    // overrides are needed; the "PLAY AGAIN?" button is the only way forward.

private:
    std::unique_ptr<Button> playAgainButton_;

    // Same reset as DeathScene::onTryAgain() -- fresh default GameState, routed
    // back through the toy_animation intro into GotchiScene.
    void onPlayAgain();
};

#endif // CREDITS_SCENE_HPP
