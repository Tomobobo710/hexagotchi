#include "MergeController.hpp"
#include "EventBus.h"
#include "Keys.h"
#include "HappinessCheckpoints.hpp"

MergeController::MergeController(EventBus& bus, GameState& state, SceneManager& scenes)
    : bus_(bus), state_(state), scenes_(scenes) {
    // Subscribe to events we consume
    bus_.subscribe(EventType::MergeRequested, [this](const Event&) {
        // Merge policy:
        // - First merge (!seenReality): always allowed
        // - After unlock: allowed only if merge is available AND flag is set
        // - Request while mode != Gotchi, or while locked: ignored
        if (state_.mode != Mode::Gotchi) {
            return;  // Ignore if not in Gotchi mode
        }
        if (!isMergeAvailable()) {
            return;  // Ignore if merge is locked
        }
        if (state_.seenReality) {
            // After first merge, Merge is only unlocked by the sleep-collapse
            // gate (see GotchiScene::applySleepCollapseGate() /
            // HexViewScene::update()) -- the player wobbling at SLEEP_DEBT
            // 100. There is no other way to voluntarily merge post-tutorial.
            if (!state_.sleepCollapsed) {
                return;  // Ignore manual request when not sleep-collapsed
            }
        }
        // Request is valid — enter the merge
        enterMerge(false);
    });

    bus_.subscribe(EventType::StoryBeatCompleted, [this](const Event&) {
        returnFromMerge();
    });
}

void MergeController::update(float dt) {
    // 1. If mode == Gotchi and first merge hasn't happened, accumulate waiting timer
    if (state_.mode == Mode::Gotchi && !state_.seenReality) {
        waitingTimer_ += dt;
    }

    // 2. If mergeLockTimer > 0, decrement by dt, clamp at 0
    if (state_.mergeLockTimer > 0.0f) {
        state_.mergeLockTimer -= dt;
        if (state_.mergeLockTimer < 0.0f) {
            state_.mergeLockTimer = 0.0f;
        }
    }

    // Sleep hitting its cap NEVER auto-merges -- it only locks every button
    // except Merge and holds the gotchi in the wobble pose (see
    // GotchiScene::applySleepCollapseGate()). The player must click Merge
    // themselves. The one and only auto-transition is death (see
    // GotchiScene/HexViewScene's death-timer handling), which is unrelated
    // to MergeController entirely.
}

bool MergeController::isMergeAvailable() const {
    // Merge is available if lock timer has expired and mode is Gotchi
    return state_.mergeLockTimer <= 0.0f && state_.mode == Mode::Gotchi;
}

const char* MergeController::mergeButtonLabel() const {
    return "Merge";
}

void MergeController::enterMerge(bool forced) {
    // 3.4 Entering a beat (shared by first / voluntary / forced merge)
    state_.mode = Mode::Story;

    // Clear the sleep-collapse gate -- it was the only thing that unlocked
    // Merge in the first place, and it should not still read "collapsed"
    // once the player has actually merged.
    state_.sleepCollapsed = false;

    // --- Happiness Checkpoints (of 3): snapshot "is the gotchi happy right
    // now?" at merge time, keyed off how many merges have happened. This is
    // the single choke point every merge passes through, so it's where all
    // three checks live. Each records ONE bool into the persistent story
    // ledger; the tally (0..3) eventually gates the ending. See
    // HappinessCheckpoints.hpp. Recorded here at merge-IN, before the story
    // scene loads, off live vitals.
    //
    // mergeCount is only incremented on the way BACK (returnFromMerge), so at
    // enter-time it still reads the count BEFORE this merge:
    //   1st merge -> mergeCount == 0 (also !seenReality) -> check 1
    //   3rd merge -> mergeCount == 2                      -> check 2
    //   5th merge -> mergeCount == 4                      -> check 3 (final)
    // Check 3 is the FINAL check -- it's the one the ending gate reads
    // alongside 1 and 2 (see HappinessCheckpoints.hpp / happyCheckpointsPassed).
    if (state_.mergeCount == 0) {
        recordHappinessCheckpoint(state_, 1);   // Check 1 -- the first merge
    } else if (state_.mergeCount == 2) {
        recordHappinessCheckpoint(state_, 2);   // Check 2 -- the third merge
    } else if (state_.mergeCount == 4) {
        recordHappinessCheckpoint(state_, 3);   // Check 3 -- the fifth merge (final)
    }

    // 3.3 First-merge timing bucket (computed only on first merge)
    if (!state_.seenReality) {
        // Compute timing bucket for first merge
        // Check if any need is below neglect threshold (neglect beats the clock)
        bool anyNeedNeglect = (state_.needs.hunger < FIRST_MERGE_NEGLECT_NEED) ||
                              (state_.needs.hygiene < FIRST_MERGE_NEGLECT_NEED) ||
                              (state_.needs.affection < FIRST_MERGE_NEGLECT_NEED) ||
                              (state_.needs.energy < FIRST_MERGE_NEGLECT_NEED);

        if (anyNeedNeglect) {
            state_.firstMergeBucket = FirstMerge::AfterNeglect;
        } else if (waitingTimer_ < FIRST_MERGE_IMMEDIATE_SEC) {
            state_.firstMergeBucket = FirstMerge::Immediate;
        } else if (waitingTimer_ < FIRST_MERGE_ABIT_SEC) {
            state_.firstMergeBucket = FirstMerge::AfterABit;
        } else {
            state_.firstMergeBucket = FirstMerge::AfterNeglect;
        }

        // Mark as seen reality
        state_.seenReality = true;
    }

    // Emit MergeCompleted
    bus_.emit(Event::mergeCompleted());

    // Emit StoryBeatStarted with current beat index
    bus_.emit(Event::storyBeatStarted(state_.storyBeatIndex));

    // Note: Box D subscribes to StoryBeatStarted and loads the story scene
    // We do not touch the scene here
}

void MergeController::returnFromMerge() {
    // 3.5 Completing a beat
    // If deathEnabled is true (climax reached), do nothing — defer to ending box
    if (state_.deathEnabled) {
        return;
    }


    // Set merge lock cooldown
    state_.mergeLockTimer = MERGE_LOCK_COOLDOWN_SEC;

    // Reset sleep to full so the next drain cycle can begin
    state_.sleep = SLEEP_FULL;

    // Reset the visible Sleep bar too (0 = rested, 100 = exhausted) -- this
    // is the stat GotchiScene's vitals HUD actually shows the player;
    // state_.sleep above is a separate hidden 0..1 metronome that drives the
    // merge-forcing/collapse gate, not what's on screen.
    state_.vitals.setStat(SecondaryStat::SLEEP_DEBT, 0.0f);

    // Set engagedStorySide
    state_.engagedStorySide = true;

    // Increment mergeCount
    state_.mergeCount++;

    // Increment storyBeatIndex for next time
    state_.storyBeatIndex++;

    // Switch scene back to GotchiScene with a fade transition
    scenes_.switchScene("gotchi");

    // Set mode back to Gotchi
    state_.mode = Mode::Gotchi;

    // Emit MergeBackCompleted
    bus_.emit(Event::mergeBackCompleted());
}
