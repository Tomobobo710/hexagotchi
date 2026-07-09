#ifndef STORY_SEQUENCER_HPP
#define STORY_SEQUENCER_HPP

// StorySequencer.hpp — Box D: story sequencer
// Glues StoryBeatStarted events to actual story scene playback.
// See docs/SCENARIO_SYSTEM.mx for the event/scenario/sequence vocabulary and
// the merge/story contract this must preserve.
//
// On StoryBeatStarted, asks ScenarioDirector for this merge's Sequence, then
// walks: merge-in transition -> each {scene, scenarioId} step in order,
// waiting for that step's dialog to finish before advancing -> merge-out
// transition -> emit StoryBeatCompleted. MergeController only ever sees one
// StoryBeatStarted in, one StoryBeatCompleted out, no matter how many steps
// the sequence actually contains.

#include "ScenarioDirector.hpp"
#include <string>

// Forward declarations
class EventBus;
class GameState;
class SceneManager;
class DialogBox;
class Scene;
struct Event;

class StorySequencer {
public:
    // Constructor wires up the sequencer to the event bus
    // Takes non-owning pointers to all collaborators
    StorySequencer(EventBus& bus, GameState& state, SceneManager& scenes, DialogBox& dialog);

    // Destructor: unsubscribes from event bus
    ~StorySequencer();

    // Drives the merge-in/step/merge-out state machine -- polls MergeScene's
    // isFinished() during the transition phases, otherwise idle (dialog
    // completion is callback-driven via onDialogFinished()).
    void update(float dt);

    // Public for tests to simulate dialog completion
    // Call this when the active step's dialog finishes playing
    void onDialogFinished();

private:
    // Phases of one sequence playthrough, one merge's worth of content --
    // position in the sequence (start vs end), NOT which MergeScene::Mode
    // visual plays (see startMergeTransition()'s comment for why those are
    // kept separate).
    enum class Phase {
        Idle,
        MergeIn,     // sequence-start transition, before the first step
        PlayingStep, // current sequence step's scene/dialog is active
        MergeOut,    // sequence-end transition, after the last step
    };

    // Event handlers
    void onStoryBeatStarted(const Event& e);

    // Internal helpers
    void startSequence();
    void startMergeTransition(Phase phase); // phase: MergeIn (sequence start) or MergeOut (sequence end)
    void startStep(int stepIndex);
    void finishSequence();

    // State
    EventBus&    bus_;
    GameState&   state_;
    SceneManager& scenes_;
    DialogBox&   dialog_;

    // Runtime state
    Sequence sequence_;          // This merge's steps, picked once at startSequence()
    int      activeStepIndex_ = -1;
    Phase    phase_ = Phase::Idle;
    bool     sequenceInProgress_ = false; // Guard against overlapping sequences
};

#endif // STORY_SEQUENCER_HPP
