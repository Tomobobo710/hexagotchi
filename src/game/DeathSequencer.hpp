#ifndef DEATH_SEQUENCER_HPP
#define DEATH_SEQUENCER_HPP

// DeathSequencer.hpp — drives the merge -> DeathScene handoff on death.
//
// Whichever scene the gotchi dies in (GotchiScene or HexViewScene) plays its
// own fallover-hold animation locally, then just leaves gotchi->isDead() set.
// It cannot also drive the scene switch itself: SceneManager::update() only
// calls the CURRENT scene's update(), so the instant that scene calls
// switchScene("merge"), its own update() stops running and can never poll
// MergeScene::isFinished() to know when to continue on to DeathScene. This
// mirrors StorySequencer, which exists for the exact same reason (see its
// header) -- a standalone object whose update() is called every frame from
// main.cpp regardless of which scene is current.
//
// Flow: GotchiSim emits PetCollapsed (once, when FITALITY hits 0) -> wait
// DEATH_HOLD_SECONDS for the fallover animation to hold on screen -> switch
// to "merge" playing MERGE_OUT (Tom bursting out of the portal, mirroring
// every other merge transition) -> once that finishes, switch to "death".

#include <string>

class EventBus;
class GameState;
class SceneManager;
struct Event;

class DeathSequencer {
public:
    DeathSequencer(EventBus& bus, GameState& state, SceneManager& scenes);

    // Drives the hold/merge/death state machine -- call every frame.
    void update(float dt);

private:
    enum class Phase {
        Idle,
        Holding,   // fallover animation holding on screen
        Merging,   // "merge" scene playing MERGE_OUT, waiting on isFinished()
    };

    void onPetCollapsed(const Event& e);

    EventBus&     bus_;
    GameState&    state_;
    SceneManager& scenes_;

    Phase phase_ = Phase::Idle;
    float holdTimer_ = 0.0f;
    static constexpr float DEATH_HOLD_SECONDS = 5.0f;
};

#endif // DEATH_SEQUENCER_HPP
