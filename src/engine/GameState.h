#ifndef GAME_STATE_H
#define GAME_STATE_H

#include <string>
#include <unordered_map>
#include <variant>
#include <optional>
#include "GameConstants.hpp"

// Save version for forward/backward compatibility
constexpr int SAVE_VERSION = 1;

// Mode enum for game flow
enum class Mode { Gotchi, HexBoard, Story };

// First merge bucket for branching story
enum class FirstMerge { NotYet, Immediate, AfterABit, AfterNeglect };

// Value type for the extensible narrative bag
using Value = std::variant<bool, int, float, std::string>;

// Forward declaration for SaveManager to access private slotPath
class SaveManager;

struct GameState {
    int version = SAVE_VERSION;

    // ---- Typed, hot-path sim state (read every frame) ----
    struct Needs {
        float hunger = 1.0f;
        float hygiene = 1.0f;
        float affection = 1.0f;
        float energy = 1.0f;
    } needs;

    float sleep        = 1.0f;    // Metronome: one-way drain, forces merge at 0
    bool  deathEnabled = false;   // Flips at climax; before this, cannot die

    // ---- Hidden branch drivers (NEVER shown to player as numbers) ----
    struct Drivers {
        float affection = 0.0f;
        float mercy = 0.0f;
        float survival = 1.0f;
    } drivers;

    // ---- Flow / merge ----
    Mode        mode              = Mode::Gotchi;
    bool        seenReality       = false;              // Drives merge-button label
    FirstMerge  firstMergeBucket  = FirstMerge::NotYet; // Picks beat-1 variant
    int         storyBeatIndex    = 0;                  // "Next beat" cursor, NOT a fixed count
    float       mergeLockTimer    = 0.0f;               // Post-beat cooldown before next merge
    double      playtimeSeconds   = 0.0;                // For slot summaries

    // --- batched up front so B/C/D/F don't each edit this header ---
    int   mergeCount       = 0;      // B writes on each merge; C reads it for the mercy driver
    float grime            = 0.0f;   // 0 = pristine, 1 = fully neglected. F tints drawGary from this; C/D drive it
    bool  collapsed        = false;  // C sets when he falls after deathEnabled; E reads it for ending 3
    bool  engagedCareSide  = false;  // set true once the player has used gotchi/hex care
    bool  engagedStorySide = false;  // set true once the player has completed a story beat

    // ---- Extensible narrative bag: choices, seen-flags, arbitrary story vars ----
    std::unordered_map<std::string, Value> flags;

    // Convenience accessors (never index the map raw at call sites)
    void  setFlag(const std::string& key, Value v) { flags[key] = std::move(v); }
    bool  getBool (const std::string& key, bool   def = false) const;
    int   getInt  (const std::string& key, int    def = 0)     const;
    float getFloat(const std::string& key, float  def = 0.f)   const;
    std::string getStr(const std::string& key, std::string def = "") const;
    bool  has(const std::string& key) const { return flags.count(key) != 0; }

    // Reset to initial state
    void reset() {
        *this = GameState{};
    }
};

#endif // GAME_STATE_H
