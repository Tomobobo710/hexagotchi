#ifndef GAME_CONSTANTS_HPP
#define GAME_CONSTANTS_HPP

static const int GAME_W = 720;
static const int GAME_H = 720;

// Letterbox scaling mode for stretching the GAME_W x GAME_H render target to
// the window. Compile-time default below; flip at runtime with
// SetIntegerScaling() e.g. from a settings menu.
//
// true  (default): scale is snapped to the nearest whole number (1x, 2x, 3x,
//        ...) so every source pixel maps to an even NxN block on screen --
//        the standard "pixel perfect" look for pixel-art sprites. Leaves
//        unused letterbox space at window sizes that aren't an exact multiple
//        of GAME_W/GAME_H.
// false: scale is the exact fractional fit to the window (no wasted space,
//        but pixel-art edges can look uneven/aliased at non-integer scales).
static const bool GAME_INTEGER_SCALING_DEFAULT = false;

inline bool& IntegerScalingState() {
    static bool value = GAME_INTEGER_SCALING_DEFAULT;
    return value;
}

inline void SetIntegerScaling(bool enabled) {
    IntegerScalingState() = enabled;
}

inline bool IsIntegerScalingEnabled() {
    return IntegerScalingState();
}

#endif // GAME_CONSTANTS_HPP
