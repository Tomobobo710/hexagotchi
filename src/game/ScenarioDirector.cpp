#include "ScenarioDirector.hpp"
#include "GameState.h"

Sequence ScenarioDirector::SelectSequence(const GameState& /*state*/) {
    // Scenario C (Office scenario 1, "The Promotion (Sort Of)") then
    // Scenario A (Pizza Parlor scenario 0, the wife needling Tom while the
    // Pokemon heckles from the sideline) -- a two-step sequence to prove out
    // multiple scene visits within one merge. Only sequence that exists
    // right now -- see docs/SCENARIO_SYSTEM.mx section 3 for where real
    // state-driven selection goes once there's more than one to pick.
    return {
        { "office", 1, false },
        { "pizza_parlor", 0, false },
    };
}
