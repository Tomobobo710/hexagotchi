#ifndef CREDITS_SCENE_HPP
#define CREDITS_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "CharacterRegistry.hpp"
#include "raylib.h"
#include <memory>
#include <vector>
#include <string>

// The end-of-game credits screen -- reached after either ending's apartment
// coda (see ScenarioDirector's mergeCount==5 sequences). Plays a cast parade
// (each character's happy pose walks on, name label shows above in their
// identity color, then walks off), followed by a slow rank reveal based on
// happyCheckpointsPassed(state) (0/3=D-, 1/3=C, 2/3=B, 3/3=A+), and finally
// unlocks the "PLAY AGAIN?" button.
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
    // overrides are needed; the "PLAY AGAIN?" button is the only way forward
    // (and is hidden until the cast parade + rank reveal finish).

public:
    // Cast-parade phase timings (seconds). Walk-in and walk-out move the pose
    // across a fixed screen-space distance; hold is how long it stands still
    // (fully on-screen, name label showing) before walking off. Public so the
    // file-scope castStepTransform() draw helper can share the same recipe.
    static constexpr float WALK_IN_DURATION  = 0.6f;
    static constexpr float HOLD_DURATION     = 1.1f;
    static constexpr float WALK_OUT_DURATION = 0.6f;
    static constexpr float STEP_DURATION = WALK_IN_DURATION + HOLD_DURATION + WALK_OUT_DURATION;

private:
    std::unique_ptr<Button> playAgainButton_;

    // One cast member's parade entry: which character, their pose texture
    // (loaded once in init(), unloaded in cleanup()), and a per-entry timing
    // recipe (walk-in / hold / walk-out) computed from CAST_STEP_DURATION.
    struct CastEntry {
        CharacterId id;
        Texture2D pose;
    };
    std::vector<CastEntry> cast_;
    int castIndex_ = 0;
    float castElapsed_ = 0.0f;

    enum class Phase { Cast, RankLabel, RankPause, RankReveal, Done };
    Phase phase_ = Phase::Cast;
    float phaseElapsed_ = 0.0f;

    // Rank computed once in init() from happyCheckpointsPassed(*state), where
    // 0/3->D-, 1/3->C, 2/3->B, 3/3->A+. Held here rather than recomputed every
    // frame since the checkpoints don't change once Credits is reached.
    std::string rankLetter_;
    Color rankColor_ = WHITE;

    // "Rank:" label appears first (RankLabel phase), then a pause with nothing
    // shown after it (RankPause), then the letter itself fades/pops in
    // (RankReveal) before advancing to Done, which reveals the button.
    static constexpr float RANK_LABEL_DURATION = 0.6f;
    static constexpr float RANK_PAUSE_DURATION = 1.0f;
    static constexpr float RANK_REVEAL_DURATION = 1.2f;

    // Same reset as DeathScene::onTryAgain() -- fresh default GameState, routed
    // back through the toy_animation intro into GotchiScene.
    void onPlayAgain();
};

#endif // CREDITS_SCENE_HPP
