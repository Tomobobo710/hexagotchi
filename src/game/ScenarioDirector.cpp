#include "ScenarioDirector.hpp"
#include "GameState.h"
#include "HappinessCheckpoints.hpp"

Sequence ScenarioDirector::SelectSequence(const GameState& state) {
    // Selection is keyed off mergeCount -- how many merges have COMPLETED.
    // This runs at merge-IN time (StoryBeatStarted), and mergeCount is only
    // incremented on the way back out (MergeController::returnFromMerge), so
    // here it reads the count BEFORE this merge:
    //   mergeCount == 0  -> this is the 1st merge
    //   mergeCount == 1  -> this is the 2nd merge
    // StorySequencer wraps whatever we return in its own merge-in / merge-out
    // transitions, so a sequence is just the story steps between them.

    // --- 2nd merge: pizza -> kids at the apartment -> office endcap ---
    //   pizza_parlor 0  -- Karen needling Tom, Ronzer heckling
    //   kids_visit   0  -- Bimmy & Jimmy at Tom's cold apartment
    //   office       2  -- Scenario Z endcap: Tom mutters, then merge out
    // Merge in -> these three -> merge back out to the gotchi scene.
    if (state.mergeCount == 1) {
        return {
            { "pizza_parlor", 0, false },
            { "kids_visit", 0, false },
            { "office", 2, false },
        };
    }

    // --- 3rd merge: what does Tom even DO -> therapy -> office endcap ---
    //   office            3  -- Scenario D: Larry takes the "job" gravely,
    //                            Tom questions why anyone cares, Loraine reads
    //                            live stats as the player's clue
    //   therapist_office  0  -- "The Last Session": the digital-pet metaphor
    //   office            2  -- Scenario Z endcap, then merge out
    if (state.mergeCount == 2) {
        return {
            { "office", 3, false },
            { "therapist_office", 0, false },
            { "office", 2, false },
        };
    }

    // --- 4th merge: school pickup -> Mark's philosophy -> office endcap ---
    //   school       0  -- "The School Pickup Incident": Karen needles Tom
    //   apartment    1  -- "The Drain / What Does It All Mean": Mark the
    //                       accidental philosopher + boundary-violating creep
    //   office       2  -- Scenario Z endcap, then merge out
    if (state.mergeCount == 3) {
        return {
            { "school", 0, false },
            { "apartment", 1, false },
            { "office", 2, false },
        };
    }

    // --- 5th merge (==4): the FINALE -- one big sequence, no return trip ----
    // This merge records happiness Checkpoint 3 (the final check) in
    // MergeController::enterMerge() BEFORE StoryBeatStarted fires, so by the
    // time we get here the whole ledger is complete and the ending gate can
    // read it. There is NO 6th merge -- everything from here to the credits is
    // one continuous sequence and never endcaps back to the gotchi side.
    //
    //   office        4  -- Scenario E: Larry's "how'd we do", final Loraine
    //                       projection readout.
    //   pizza_parlor  1  -- Jimmy's birthday party.
    //   ending        0/1 -- parking-lot ending, GATED on the ledger:
    //                       scenario 0 (GOOD) if all 3 checkpoints passed,
    //                       scenario 1 (BAD) otherwise. Two independently
    //                       written scenarios (Tom the hero vs Mark the hero).
    //   apartment     3/2 -- coda: GOOD = Loraine comes home (scenario 3),
    //                       BAD = "morning after" with Mark (scenario 2).
    //   credits       0  -- terminal step: the sequencer parks here (no
    //                       merge-out); the "PLAY AGAIN?" button restarts.
    if (state.mergeCount == 4) {
        bool allHappy = happyCheckpointsPassed(state) == 3;
        return {
            { "office", 4, false },
            { "pizza_parlor", 1, false },
            { "ending", allHappy ? 0 : 1, false },
            { "apartment", allHappy ? 3 : 2, false },
            { "credits", 0, false, /*isTerminal*/ true },
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
