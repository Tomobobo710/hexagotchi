#ifndef MERGE_CONTROLLER_HPP
#define MERGE_CONTROLLER_HPP

// MergeController.hpp — Box B: merge controller / core loop
// HUMAN-OWNED: Constants and behavior are defined in GAME_DESIGN.md §11.
// Agents: implement the state machine, do not rewrite the design.

#include "GameState.h"
#include "EventType.h"
#include "SceneManager.hpp"

// Tunable constants (placeholders — the human tunes after a playthrough)
constexpr float FIRST_MERGE_IMMEDIATE_SEC = 20.0f;  // under this since gotchi start = Immediate
constexpr float FIRST_MERGE_ABIT_SEC      = 90.0f;  // under this = AfterABit, else AfterNeglect
constexpr float FIRST_MERGE_NEGLECT_NEED  = 0.40f;  // any visible need below this = AfterNeglect
constexpr float MERGE_LOCK_COOLDOWN_SEC   = 3.0f;   // button locked after each beat
constexpr float SLEEP_FULL                = 1.0f;   // sleep reset target on merge-back

class EventBus;

class MergeController {
public:
    MergeController(EventBus& bus, GameState& state, SceneManager& scenes);

    void        update(float dt);          // call every frame from the main loop
    bool        isMergeAvailable() const;  // UI: is the merge button pressable now
    const char* mergeButtonLabel() const;  // "Merge" if seenReality else "Give him a break"

private:
    void enterMerge(bool forced);
    void returnFromMerge();

    EventBus&    bus_;
    GameState&   state_;
    SceneManager& scenes_;

    // Runtime-only timer for first-merge timing bucket computation
    // (resets on save/load before first merge; need-based path still works after reload)
    float waitingTimer_ = 0.0f;
};

#endif // MERGE_CONTROLLER_HPP
