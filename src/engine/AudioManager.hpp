#ifndef AUDIO_MANAGER_HPP
#define AUDIO_MANAGER_HPP

// AudioManager -- the game's single, minimal audio layer.
//
// The game shipped with no audio at all; this is the whole system. It owns the
// raylib audio device and the shared one-shot SFX (currently just the UI click)
// loaded out of assets.rres via AssetPack. Volume is pulled fresh from the
// global settings (GameConstants: GetSfxVolume01) on every play, so the Options
// menu takes effect immediately without any wiring.
//
// Usage:
//   AudioManager::Get().init();   // once, after InitWindow, at startup
//   AudioManager::Get().playClick();
//   AudioManager::Get().shutdown(); // once, at exit (before CloseWindow)
namespace AudioManager {

// Which "world" the player is in -- drives the single global music track.
// Music is NOT per-scene: the main loop maps the current scene name to one of
// these and calls setMusicWorld() every frame. Silence (None) everywhere that
// isn't one of the three musical worlds (title, options, credits, death, etc.).
enum class MusicWorld {
    None,       // silence -- stop any playing track
    Gotchi,     // gotchi / hexboard care side -> pet_theme
    Merge,      // merge transition -> merge_theme
    RealWorld,  // Tom's World story scenes -> real_world_theme
};

class Manager {
public:
    // Initializes the audio device and loads the shared SFX + music. Safe to
    // call once.
    void init();

    // Unloads sounds/music and closes the audio device. Safe to call once.
    void shutdown();

    // Plays the UI click one-shot at the current global SFX volume. No-op if
    // audio isn't initialized or the clip failed to load.
    void playClick();

    // Sets which world's music should be playing. IDEMPOTENT: if `w` is already
    // the active world, does nothing (the track keeps playing seamlessly). Only
    // a genuine change stops the old track and starts the new one. Safe to call
    // every frame. MusicWorld::None stops all music.
    void setMusicWorld(MusicWorld w);

    // Feeds the active music stream (raylib requires this each frame) and
    // applies the live global music volume. Call once per frame from the main
    // loop. No-op if nothing is playing.
    void updateMusic();

    // Pause / resume the active music stream. Used when the browser tab loses
    // or regains focus: while the tab is hidden the game loop (and thus
    // updateMusic) is paused by the browser, so the stream buffer underruns and
    // glitches -- pausing the stream on blur and resuming on focus avoids that.
    // No-op if nothing is playing. updateMusic() skips a paused stream.
    void pauseMusic();
    void resumeMusic();

private:
    bool  initialized_ = false;
    void* clickSound_  = nullptr;   // heap Sound; void* to keep raylib out of this header

    // Three long-lived Music streams (heap; void* to keep raylib.h out of the
    // header). nullptr if a track failed to load.
    void* petMusic_       = nullptr;
    void* mergeMusic_     = nullptr;
    void* realWorldMusic_ = nullptr;

    MusicWorld currentWorld_ = MusicWorld::None;

    // True while music is paused specifically because the browser tab is hidden
    // (see pauseMusic/resumeMusic). Distinguishes that from a stream stopped
    // because volume is 0, so updateMusic() doesn't auto-restart a tab-blur
    // pause but does restart when the volume is raised back up.
    bool pausedForBlur_ = false;

    // Returns the heap Music* for a world, or nullptr for None / unloaded.
    void* musicFor(MusicWorld w) const;
};

// Process-wide singleton.
Manager& Get();

} // namespace AudioManager

#endif // AUDIO_MANAGER_HPP
