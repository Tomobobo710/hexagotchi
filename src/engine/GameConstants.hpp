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

// ============================================================================
// Global user settings (Options menu)
// ============================================================================
// Header-only inline-static accessors, mirroring IntegerScalingState() above,
// so any scene OR the engine (DialogBox etc.) can read them without pulling in
// GameState. NOT persisted to disk yet -- SaveManager is #if 0'd for the jam,
// so these reset to their defaults on each launch. When save is re-enabled,
// serialize these three values there.

// --- Volumes: integer levels 0 .. 10, default 6. The Options menu steps them
//     by 1. Audio code reads the 0..1 float form (level / 10) and applies it
//     via SetSoundVolume/SetMusicVolume per clip.
static const int VOLUME_LEVEL_MAX     = 10;
static const int VOLUME_LEVEL_DEFAULT = 6;

inline int ClampVolumeLevel(int v) { return (v < 0) ? 0 : (v > VOLUME_LEVEL_MAX ? VOLUME_LEVEL_MAX : v); }

inline int& MusicVolumeState() { static int v = VOLUME_LEVEL_DEFAULT; return v; }
inline int   GetMusicVolume()  { return MusicVolumeState(); }              // 0..10
inline float GetMusicVolume01(){ return MusicVolumeState() / (float)VOLUME_LEVEL_MAX; }
inline void  SetMusicVolume(int level) { MusicVolumeState() = ClampVolumeLevel(level); }

inline int& SfxVolumeState() { static int v = VOLUME_LEVEL_DEFAULT; return v; }
inline int   GetSfxVolume()  { return SfxVolumeState(); }                  // 0..10
inline float GetSfxVolume01(){ return SfxVolumeState() / (float)VOLUME_LEVEL_MAX; }
inline void  SetSfxVolume(int level) { SfxVolumeState() = ClampVolumeLevel(level); }

// --- Dialog auto-advance speed. OFF disables the feature entirely (no timer,
//     no progress bar -- dialog only advances on tap/space). NORMAL is the
//     designed pace; FAST is 1.5x quicker. Read by DialogBox: OFF suppresses
//     setAutoContinueEnabled(true), and the speed scales the computed
//     auto-continue duration (see DialogBox::setText).
enum class DialogSpeed { Off, Normal, Fast };
inline DialogSpeed& DialogSpeedState() { static DialogSpeed s = DialogSpeed::Normal; return s; }
inline DialogSpeed  GetDialogSpeed()   { return DialogSpeedState(); }
inline void         SetDialogSpeed(DialogSpeed s) { DialogSpeedState() = s; }
// Multiplier applied to auto-continue duration: smaller = faster. FAST = 1/1.5.
inline float DialogSpeedDurationScale() {
    switch (DialogSpeedState()) {
        case DialogSpeed::Fast: return 1.0f / 1.5f;
        default:                return 1.0f;   // Normal (Off never auto-advances)
    }
}

// Gotchi world scale - fixed sprite scale in world space (64→128px)
// The gotchi's native frame is 64px; this scales it to 128px in the world
inline constexpr float GOTCHI_WORLD_SCALE = 2.0f;

// Gotchi screen fraction - target fraction of framebuffer height to fill with the gotchi
// 0.30 means the gotchi will fill ~30% of the 720px framebuffer height (~216px) --
// halved from 0.60 (too much screen real estate). The action-shader overlay
// reads the gotchi's own screen rect, so it shrinks along with the sprite.
inline constexpr float GOTCHI_SCREEN_FRAC = 0.30f;

#endif // GAME_CONSTANTS_HPP
