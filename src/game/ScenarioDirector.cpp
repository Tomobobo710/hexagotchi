#include "ScenarioDirector.hpp"
#include "GameState.h"

Sequence ScenarioDirector::SelectSequence(const GameState& state) {
    // Selection is keyed off mergeCount -- how many merges have COMPLETED.
    // This runs at merge-IN time (StoryBeatStarted), and mergeCount is only
    // incremented on the way back out (MergeController::returnFromMerge), so
    // here it reads the count BEFORE this merge:
    //   mergeCount == 0  -> this is the 1st merge
    //   mergeCount == 1  -> this is the 2nd merge
    // StorySequencer wraps whatever we return in its own merge-in / merge-out
    // transitions, so a sequence is just the story steps between them.

    // --- 2nd merge: the pizza parlor beat ---
    //   pizza_parlor 0  -- Karen needling Tom, Ronzer heckling
    // Merge in -> pizza -> merge back out to the gotchi scene.
    if (state.mergeCount == 1) {
        return {
            { "pizza_parlor", 0, false },
        };
    }

    // --- 1st merge (default): the office/apartment intro run ---
    //   Office A (office 0)     -- Larry greets Tom, three E's, Early Bird
    //   Apartment 0             -- "The Heating Situation" with Mark
    //   Office B (office 1)     -- Tom's 2 min late, Larry hammers him out
    return {
        { "office", 0, false },
        { "apartment", 0, false },
        { "office", 1, false },
    };
}
