#ifndef FLAG_KEYS_H
#define FLAG_KEYS_H

// flags/Keys.h — HUMAN-OWNED. Agents APPEND via request; they do not rewrite this file.
//
// All flag keys are referenced through constexpr constants, never raw string literals
// at call sites. This is the single biggest defense against silent typo bugs when
// multiple agents write flags.
//
// Convention for choice keys: "choice.<scene>.<name>", value = bool or int (branch index).

namespace flag {
    // Story flags
    inline constexpr auto SEEN_REALITY        = "seen_reality";
    inline constexpr auto BOTH_SIDES_ENGAGED  = "both_sides_engaged";

    // Scene choices — convention: "choice.<scene>.<name>"
    inline constexpr auto CHOICE_OFFICE_CONFRONT = "choice.office.confront_boss";
    inline constexpr auto CHOICE_SCHOOL_DEFEND   = "choice.school.defend_zap";

    // Append new keys here (alphabetically preferred for readability)
    // Example: inline constexpr auto CHOICE_XXX = "choice.xxx.name";

} // namespace flag

#endif // FLAG_KEYS_H
