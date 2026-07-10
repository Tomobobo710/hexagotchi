#ifndef EXPLORE_GATE_H
#define EXPLORE_GATE_H

// ExploreGate.h - The ONLY place the Explore condition lives
// The predicate returns true when:
//   - confidence is high (>= threshold)
//   - excitement is low (<= threshold)
//
// Additional conditions are checked at the call site:
//   - seenReality must be true (player has visited the real world)
//
// This allows the player to "explore" the hexboard only when the gotchi
// is confident and not overly excited (calm state), and only after
// the player has visited the real world (same condition for Merge button).

// Thresholds - tunable for the human to adjust
// confidence >= 0.60 = "confidence high"
// excitement <= 0.40 = "excitement low"
inline constexpr float EXPLORE_CONFIDENCE_MIN = 0.60f;
inline constexpr float EXPLORE_EXCITEMENT_MAX = 0.40f;

// The gate predicate - returns true if Explore button should be visible
inline bool shouldShowExplore(float confidence, float excitement) {
    return confidence >= EXPLORE_CONFIDENCE_MIN
        && excitement <= EXPLORE_EXCITEMENT_MAX;
}

#endif // EXPLORE_GATE_H
