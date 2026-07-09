#ifndef SCENARIO_DIRECTOR_HPP
#define SCENARIO_DIRECTOR_HPP

// See docs/SCENARIO_SYSTEM.mx for the event/scenario/sequence vocabulary.
//
// ScenarioDirector is the "future state machine" that doc keeps referring
// to: given the current GameState, it decides which scenario(s) play this
// merge and in which scene(s). StorySequencer owns none of that selection
// logic -- it just walks whatever Sequence comes back and plays it out.
//
// Deliberately trivial today (always the same one-step sequence). The seam
// exists so growing this into real state-driven selection later is a change
// inside SelectSequence(), not a rewire of StorySequencer's step-walking.

#include <string>
#include <vector>

struct GameState;

// One scene visit within a merge's sequence -- which scene, and which of
// that scene's scenarios (today: triggerStoryEvent()'s index) to play.
struct SequenceStep {
    std::string sceneName;
    int scenarioId = 0;
    // True if this step is the sequence's climax beat: sets deathEnabled
    // and emits DeathEnabled once this step's dialog finishes (see
    // StorySequencer). At most one step per sequence should set this.
    bool isClimax = false;
};

using Sequence = std::vector<SequenceStep>;

namespace ScenarioDirector {
    // Picks this merge's sequence of scene/scenario steps from the current
    // GameState. Today: always [{"office", 1}, {"pizza_parlor", 0}]
    // (Scenario C then Scenario A -- see SelectSequence()'s own comment for
    // what those letters mean) -- GameState is unused for now but threaded
    // through so callers/signature don't need to change once real selection
    // logic lands here.
    Sequence SelectSequence(const GameState& state);
}

#endif // SCENARIO_DIRECTOR_HPP
