#include "ScenarioDirector.hpp"
#include "GameState.h"

Sequence ScenarioDirector::SelectSequence(const GameState& /*state*/) {
    // First-merge sequence:
    //   Office A (office 0)     -- Larry greets Tom, three E's, Early Bird
    //   Apartment 0             -- "The Heating Situation" with Mark
    //   Office B (office 1)     -- Tom's 2 min late, Larry hammers him out
    // Then the sequence ends and Tom merges back out. State-driven selection
    // goes here later -- see docs/SCENARIO_SYSTEM.mx sec 3.
    return {
        { "office", 0, false },
        { "apartment", 0, false },
        { "office", 1, false },
    };
}
