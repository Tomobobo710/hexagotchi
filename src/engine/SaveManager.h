#ifndef SAVE_MANAGER_H
#define SAVE_MANAGER_H

// DISABLED: Save system shut off for game jam
// All save/load functionality is disabled. Do not use until further notice.
#if 0

#include "GameState.h"
#include <optional>
#include <string>
#include <vector>

// Forward declaration
struct SlotSummary;

class SaveManager {
public:
    static constexpr int NUM_SLOTS = 3;

    // Save/load operations
    bool save(int slot, const GameState& s);          // Atomic write (temp + rename)
    std::optional<GameState> load(int slot);          // nullopt if missing/corrupt
    bool deleteSlot(int slot);
    bool slotExists(int slot) const;
    SlotSummary summary(int slot) const;              // Cheap read for the menu

    // Active-slot autosave: pick a slot on New/Continue, then every autosave targets it.
    void setActiveSlot(int slot) { active_ = slot; }
    int  activeSlot() const { return active_; }
    void autosave(const GameState& s);                // Called at checkpoints + shutdown

    // Initialize save directory (call once on startup)
    static void initSaveDir();

    // Set save directory (for testing purposes)
    static void setSaveDir(const std::string& dir);

private:
    int active_ = -1;

    std::string slotPath(int slot) const;             // <savedir>/save_<slot>.json
    std::string slotTempPath(int slot) const;         // <savedir>/save_<slot>.json.tmp
    std::string saveDir() const;                      // Desktop: user dir; Web: IDBFS mount
};

// Slot summary shown on main menu load list
struct SlotSummary {
    bool        exists = false;
    int         version = 0;
    int         storyBeatIndex = 0;
    double      playtimeSeconds = 0;
    std::string label;        // e.g. pet name or beat title, for a non-blank slot
};

#endif // DISABLED

#endif // SAVE_MANAGER_H
