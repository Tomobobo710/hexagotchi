// DriversController.hpp — Box C: affection/mercy drivers computation
// Subscribes to CareAction events and updates GameState drivers
// HUMAN-OWNED: Behavior is defined in GAME_DESIGN.md §11.
// Agents: implement the event flow, do not rewrite the design.

#ifndef DRIVERS_CONTROLLER_HPP
#define DRIVERS_CONTROLLER_HPP

#include "EventBus.h"
#include "GameState.h"
#include "MergeController.hpp"

// Warmth actions drive affection: pet/play only (action code 3 = Pet)
// Hygiene actions drive mercy: wash/groom only (action codes 0, 1)
inline bool isWarmthAction(int actionCode) {
    return actionCode == 3;  // Pet
}

inline bool isHygieneAction(int actionCode) {
    return actionCode == 0 || actionCode == 1;  // Wash or Groom
}

// Gain constants are defined in MergeController.hpp:
//   AFFECTION_GAIN = 0.08f (per warmth action)
//   MERCY_GAIN = 0.12f (per hygiene action)

class DriversController {
public:
    // Constructor wires up to event bus
    DriversController(EventBus& bus, GameState& state);

    // Destructor unsubscribes
    ~DriversController();

    // Update is called each frame - handles grime creep and decay
    void update(float dt);

    // Accessors for debug/readout
    float getAffectionAccumulator() const { return affectionAccum_; }
    float getHygieneAccumulator() const { return hygieneAccum_; }
    float getAffection() const { return state_.drivers.affection; }
    float getMercy() const { return state_.drivers.mercy; }

private:
    // Event handlers
    void onCareAction(const Event& e);

    // State
    EventBus&    bus_;
    GameState&   state_;

    // Runtime accumulators for warmth/hygiene actions
    float affectionAccum_ = 0.0f;
    float hygieneAccum_ = 0.0f;

    // Event subscription token
    int careActionToken_ = 0;
};

#endif // DRIVERS_CONTROLLER_HPP
