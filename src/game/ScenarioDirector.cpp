#include "ScenarioDirector.hpp"
#include "GameState.h"

Sequence ScenarioDirector::SelectSequence(const GameState& /*state*/) {
    // Scenario A (Office scenario 0): the first merge into Tom's world --
    // Tom arrives feeling sick, Larry greets him about the "three E's". This
    // is the current work-in-progress beat we test in the full flow. Only
    // sequence that exists right now -- see docs/SCENARIO_SYSTEM.mx section 3
    // for where real state-driven selection goes once there's more than one.
    return {
        { "office", 0, false },
    };
}
