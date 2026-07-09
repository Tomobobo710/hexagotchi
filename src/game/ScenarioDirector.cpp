#include "ScenarioDirector.hpp"
#include "GameState.h"

Sequence ScenarioDirector::SelectSequence(const GameState& /*state*/) {
    // Scenario A: the Pizza Parlor's existing event 0 (wife needling Tom
    // while the Pokemon heckles from the sideline). Only sequence that
    // exists right now -- see docs/SCENARIO_SYSTEM.mx section 3 for where
    // real state-driven selection goes once there's more than one to pick.
    return { { "pizza_parlor", 0, false } };
}
