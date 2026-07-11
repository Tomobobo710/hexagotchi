#ifndef HAPPINESS_CHECKPOINTS_HPP
#define HAPPINESS_CHECKPOINTS_HPP

#include "GameState.h"

// ============================================================================
// Happiness Checkpoints -- the story's "was the gotchi cared for?" ledger
// ============================================================================
//
// THE SYSTEM (read this before touching it):
//
// At three fixed story moments -- "checkpoints" -- we snapshot ONE fact: was
// the gotchi HAPPY at that moment? Nothing else. Each checkpoint stores a
// single bool into GameState's persistent `flags` bag (which is serialized
// with the save, so these survive across sessions).
//
//   Checkpoint 1  -> flag "happy_check_1"   (OfficeScene Scenario A -- first merge)
//   Checkpoint 2  -> flag "happy_check_2"   (NOT WRITTEN YET -- future scene)
//   Checkpoint 3  -> flag "happy_check_3"   (NOT WRITTEN YET -- the last check)
//
// "Happy" is defined by ONE measurement: the gotchi's live Happiness stat
// (EmotionalStat::HAPPINESS) being at or above HAPPY_THRESHOLD at the instant
// the checkpoint fires. That's the whole criterion. All the OTHER numbers
// Loraine reads in the dialog (Health, Energy, Hunger, etc.) are FLAVOR ONLY
// -- they are not measured or stored here.
//
// HOW THE ENDING USES IT (future work):
//   The 3rd checkpoint is the gate. When check 3 is recorded, the tally of how
//   many of the three came back happy (0..3) decides which way the story ends.
//   That branching logic lives at checkpoint 3's site, NOT here -- this header
//   only records and reads the three bools. `happyCheckpointsPassed()` gives
//   the count so the ending gate can just ask for it.
//
// USAGE:
//   recordHappinessCheckpoint(state, 1);   // snapshot now, into happy_check_1
//   bool passed1 = getHappinessCheckpoint(state, 1);
//   int  score   = happyCheckpointsPassed(state);   // 0..3, for the ending gate
//
// A checkpoint is written ONCE per playthrough. Re-recording the same index
// overwrites it (harmless), but the intended flow is: each story checkpoint
// fires its record call exactly once when that scene reaches the beat.
// ============================================================================

namespace HappinessCheckpoint {

// The single measurement: was the gotchi's live MOOD literally "Happy" at merge
// time? Not a stat threshold -- the gotchi mood system (GotchiMood::updateMood)
// resolves to MOOD_00_HAPPY only when ALL FOUR care needs (hunger, thirst,
// hygiene, happiness) are at/above 75% good; otherwise it's whichever need is
// worst. So "mood == Happy" IS the holistic "all needs met" verdict the player
// sees on screen. Mood is updated every frame outside the vitals tick gate
// (GotchiSim), so it's current at merge time.

constexpr int COUNT = 3;

// The persistent flag key for checkpoint `index` (1..COUNT). Kept in one place
// so the three call sites can't drift on spelling.
inline std::string flagKey(int index) {
    return "happy_check_" + std::to_string(index);
}

} // namespace HappinessCheckpoint

// Snapshot "was the gotchi's mood Happy right now?" into checkpoint `index`
// (1..3). Reads the live mood off GameState; stores the resulting bool under
// the checkpoint's flag. Safe no-op on a bad index.
inline void recordHappinessCheckpoint(GameState& state, int index) {
    if (index < 1 || index > HappinessCheckpoint::COUNT) return;
    bool wasHappy =
        state.mood.getCurrentMood() == GotchiMoodType::MOOD_00_HAPPY;
    state.setFlag(HappinessCheckpoint::flagKey(index), wasHappy);
}

// Read back a recorded checkpoint. Defaults to false if never recorded.
inline bool getHappinessCheckpoint(const GameState& state, int index) {
    if (index < 1 || index > HappinessCheckpoint::COUNT) return false;
    return state.getBool(HappinessCheckpoint::flagKey(index), false);
}

// How many of the three checkpoints came back happy (0..COUNT). Unrecorded
// checkpoints count as not-happy. This is what the ending gate reads.
inline int happyCheckpointsPassed(const GameState& state) {
    int passed = 0;
    for (int i = 1; i <= HappinessCheckpoint::COUNT; i++) {
        if (getHappinessCheckpoint(state, i)) passed++;
    }
    return passed;
}

#endif // HAPPINESS_CHECKPOINTS_HPP
