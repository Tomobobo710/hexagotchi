#ifndef CONFIG_HPP
#define CONFIG_HPP

// Config -- persists user PREFERENCES across launches. Two backends behind one
// interface, chosen at compile time:
//   - Web (PLATFORM_WEB): browser localStorage (via emscripten).
//   - Desktop: a "settings.ini" file next to the exe (simple key=value lines).
//
// What persists (NOT save-game state -- that's SaveManager, which is off):
//   - music volume, sfx volume  (0..10)           -- GameConstants globals
//   - dialog auto-advance speed  (OFF/NORMAL/FAST) -- GameConstants global
//   - tutorial_seen              (bool)            -- so the tutorial only ever
//     runs on a machine's first play, not every fresh launch.
//
// Flow:
//   Config::Load()      -- once at startup, applies stored values to the
//                          GameConstants globals + globalGameState's
//                          tutorial_seen flag. Missing/absent config -> defaults.
//   Config::Save()      -- writes the CURRENT global values out. Call after any
//                          settings change (Options menu) or when tutorial_seen
//                          flips. Cheap; safe to call on every change.
//   Config::ResetToDefaults() -- restore default settings (does NOT clear
//                          tutorial_seen) and Save(). Wired to the Options
//                          "Reset to Defaults" button.
namespace Config {
    void Load();
    void Save();
    void ResetToDefaults();
}

#endif // CONFIG_HPP
