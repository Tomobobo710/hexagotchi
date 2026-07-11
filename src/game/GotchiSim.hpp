#ifndef GOTCHI_SIM_HPP
#define GOTCHI_SIM_HPP

// GotchiSim.hpp — C-core: simulation reducer
// HUMAN-OWNED: Constants and behavior are defined in docs/C_CORE_SIM_REDUCER.md.
// Agents: implement the reducer, do not rewrite the design.

#include "GameState.h"
#include "EventType.h"

class EventBus;

class GotchiSim {
public:
    GotchiSim(EventBus& bus, GameState& state);
    ~GotchiSim();

    void update(float dt);  // call every frame from the main loop

private:
    // Event handlers
    void onCareAction(const Event& e);
    void onMergeCompleted(const Event& e);

    // Tick the vitals every GOTCHI_TICK_RATE seconds
    void tickVitals(float dt);

    // Update mood every frame (mood overlays need precise timing)
    void updateMood(float dt);

    // Constants (from plan)
    static constexpr float SLEEP_DRAIN_RATE       = 0.02f;  // per second of care time; ~50s of care -> forced merge
    static constexpr float AFFECTION_GAIN         = 0.05f;  // per warmth action
    static constexpr float MERCY_GAIN             = 0.10f;  // base per merge, scaled by earliness
    static constexpr float GRIME_CREEP_RATE       = 0.01f;  // per second of neglect
    static constexpr float GRIME_CARE_REDUCE      = 0.08f;  // per care action
    static constexpr float COLLAPSE_NEED_THRESHOLD= 0.05f;  // a need at/under this collapses (only if deathEnabled)

    // Action type identifiers (from Event::ia in CareAction)
    static constexpr int CARE_ACTION_FEED   = 0;
    static constexpr int CARE_ACTION_PET    = 1;
    static constexpr int CARE_ACTION_WASH   = 2;
    static constexpr int CARE_ACTION_GROOM  = 3;
    static constexpr int CARE_ACTION_WATER  = 4;
    static constexpr int CARE_ACTION_BREAK  = 5;

    // Warmth actions (pet/play/ball) - these build affection
    bool isWarmthAction(int actionType) const;

    EventBus&   bus_;
    GameState&  state_;

    // Subscription tokens
    int careActionToken_ = 0;
    int mergeCompletedToken_ = 0;

    // Track if we've already emitted PetCollapsed (only once)
    bool collapsedEmitted_ = false;

    // Tick timer for vitals/mood updates (separate from other metronomes)
    static constexpr float GOTCHI_TICK_RATE = 10.0f;  // Base tick rate in seconds
    float vitalsTickTimer_ = 0.0f;
};

#endif // GOTCHI_SIM_HPP
