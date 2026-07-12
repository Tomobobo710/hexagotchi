#include "AudioManager.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "raylib.h"

namespace AudioManager {

// Maps a 0..1 slider level to a raylib linear-amplitude gain. Two shaping
// steps so audio isn't ear-splitting at moderate settings:
//   - perceptual curve: square the level, since perceived loudness ~ amplitude^2
//     -- makes low settings genuinely quiet and the steps feel even.
//   - MASTER_CEILING: cap the absolute maximum. Raw raylib volume 1.0 is
//     brutally loud relative to other PC audio; this pulls the top down.
// Applies to BOTH music and SFX so the whole game shares one loudness scale.
static float levelToGain(float norm01) {
    const float MASTER_CEILING = 0.55f;
    if (norm01 <= 0.0f) return 0.0f;
    return norm01 * norm01 * MASTER_CEILING;
}

// A streaming Music plus the packed byte buffer it reads from.
// LoadMusicStreamFromMemory keeps a pointer INTO `bytes`, so the buffer must
// live as long as the stream -- we hold both and free them together.
struct MusicTrack {
    Music          music = {0};
    unsigned char* bytes = nullptr;
    bool           ok    = false;
};

static MusicTrack loadTrack(const std::string& key) {
    MusicTrack t;
    unsigned int size = 0;
    t.bytes = AssetPack::loadRawBytes(key, &size);
    if (!t.bytes || size == 0) {
        TraceLog(LOG_WARNING, "AUDIO music failed to load: %s", key.c_str());
        return t;
    }
    // Decode by extension. The bytes must outlive the stream (kept in t.bytes).
    std::string ext = ".mp3";
    size_t dot = key.find_last_of('.');
    if (dot != std::string::npos) ext = key.substr(dot);
    t.music = LoadMusicStreamFromMemory(ext.c_str(), t.bytes, (int)size);
    if (t.music.stream.buffer == nullptr) {
        TraceLog(LOG_WARNING, "AUDIO music decode fail: %s", key.c_str());
        UnloadFileData(t.bytes);
        t.bytes = nullptr;
        return t;
    }
    t.music.looping = true;
    t.ok = true;
    return t;
}

static void unloadTrack(MusicTrack* t) {
    if (!t) return;
    if (t->ok) UnloadMusicStream(t->music);
    if (t->bytes) UnloadFileData(t->bytes);
    t->ok = false;
    t->bytes = nullptr;
}

void Manager::init() {
    if (initialized_) return;
    TraceLog(LOG_INFO, "AUDIO: init() called");
    InitAudioDevice();
    bool ready = IsAudioDeviceReady();
    TraceLog(LOG_INFO, "AUDIO: InitAudioDevice done, IsAudioDeviceReady=%d", (int)ready);
    if (!ready) {
        TraceLog(LOG_WARNING, "AUDIO device failed to init; sound disabled");
        return;
    }
    initialized_ = true;

    Sound* s = new Sound(AssetPack::loadSound("sfx/click.mp3"));
    TraceLog(LOG_INFO, "AUDIO: click.mp3 frameCount=%u", (unsigned)s->frameCount);
    if (s->frameCount == 0) {
        TraceLog(LOG_WARNING, "AUDIO click.mp3 failed to load (pack miss or decode fail)");
        delete s;
        clickSound_ = nullptr;
    } else {
        clickSound_ = s;
    }

    petMusic_       = new MusicTrack(loadTrack("music/pet_theme.mp3"));
    mergeMusic_     = new MusicTrack(loadTrack("music/merge_theme.mp3"));
    realWorldMusic_ = new MusicTrack(loadTrack("music/real_world_theme.mp3"));
    TraceLog(LOG_INFO, "AUDIO: music loaded pet=%d merge=%d real=%d",
             (int)static_cast<MusicTrack*>(petMusic_)->ok,
             (int)static_cast<MusicTrack*>(mergeMusic_)->ok,
             (int)static_cast<MusicTrack*>(realWorldMusic_)->ok);
}

void Manager::shutdown() {
    if (clickSound_) {
        UnloadSound(*static_cast<Sound*>(clickSound_));
        delete static_cast<Sound*>(clickSound_);
        clickSound_ = nullptr;
    }
    for (void** slot : { &petMusic_, &mergeMusic_, &realWorldMusic_ }) {
        if (*slot) {
            unloadTrack(static_cast<MusicTrack*>(*slot));
            delete static_cast<MusicTrack*>(*slot);
            *slot = nullptr;
        }
    }
    currentWorld_ = MusicWorld::None;
    if (initialized_) {
        CloseAudioDevice();
        initialized_ = false;
    }
}

void Manager::playClick() {
    if (!initialized_ || !clickSound_) return;
    Sound* s = static_cast<Sound*>(clickSound_);
    SetSoundVolume(*s, levelToGain(GetSfxVolume01()));   // curved + capped
    PlaySound(*s);
}

void* Manager::musicFor(MusicWorld w) const {
    switch (w) {
        case MusicWorld::Gotchi:    return petMusic_;
        case MusicWorld::Merge:     return mergeMusic_;
        case MusicWorld::RealWorld: return realWorldMusic_;
        default:                    return nullptr;   // None
    }
}

// The volume (0..1, raylib linear amplitude) to apply to the CURRENT world's
// music track. The MERGE theme is treated as an SFX -- it follows the SFX
// slider (Tom's call: the merge sting belongs to the sound-effects bus).
float Manager::musicGainForCurrentWorld() const {
    float norm = (currentWorld_ == MusicWorld::Merge) ? GetSfxVolume01() : GetMusicVolume01();
    return levelToGain(norm);
}

void Manager::setMusicWorld(MusicWorld w) {
    if (!initialized_) return;
    if (w == currentWorld_) return;   // idempotent -- no restart on same world

    // Stop the outgoing track (if it was a musical world).
    MusicTrack* prev = static_cast<MusicTrack*>(musicFor(currentWorld_));
    if (prev && prev->ok) StopMusicStream(prev->music);

    currentWorld_ = w;

    // Start the incoming track (if any). At volume 0 (music turned OFF in
    // Options) don't start it at all -- otherwise the stream plays and, on some
    // raylib builds, a per-frame SetMusicVolume(0) doesn't fully silence a
    // just-started stream. updateMusic() starts it later if the volume is
    // raised back above 0.
    MusicTrack* next = static_cast<MusicTrack*>(musicFor(w));
    if (next && next->ok && musicGainForCurrentWorld() > 0.0f) {
        SetMusicVolume(next->music, musicGainForCurrentWorld());
        PlayMusicStream(next->music);
    }
}

void Manager::updateMusic() {
    if (!initialized_) return;
    MusicTrack* t = static_cast<MusicTrack*>(musicFor(currentWorld_));
    if (!t || !t->ok) return;

    // Merge theme follows the SFX slider; everything else follows Music. Both
    // go through the perceptual curve + master ceiling (see levelToGain).
    float vol = musicGainForCurrentWorld();

    // Volume 0 == OFF: stop the stream entirely rather than playing it silently.
    // Guarantees true silence regardless of raylib's volume handling, and lets
    // the relevant slider act as an on/off at the bottom of its range.
    if (vol <= 0.0f) {
        if (IsMusicStreamPlaying(t->music)) StopMusicStream(t->music);
        return;
    }

    // Not playing (was stopped at vol 0, or paused for tab-blur via
    // pauseMusic). If it was stopped for volume, (re)start it now that volume
    // is up. A paused stream reports not-playing too, but resumeMusic() owns
    // un-pausing on tab focus -- so only (re)start here when it's genuinely
    // stopped, which after a StopMusicStream is the case. PlayMusicStream on an
    // already-playing stream is a no-op, so this is safe.
    if (!IsMusicStreamPlaying(t->music)) {
        // Only auto-(re)start for the volume case, not the tab-blur pause.
        // pauseMusic() sets pausedForBlur_; skip restart while that's set.
        if (pausedForBlur_) return;
        PlayMusicStream(t->music);
    }

    SetMusicVolume(t->music, vol);   // live-follow the slider (curved + capped)
    UpdateMusicStream(t->music);
}

void Manager::pauseMusic() {
    if (!initialized_) return;
    pausedForBlur_ = true;   // blocks updateMusic()'s volume-based auto-restart
    MusicTrack* t = static_cast<MusicTrack*>(musicFor(currentWorld_));
    if (t && t->ok && IsMusicStreamPlaying(t->music)) PauseMusicStream(t->music);
}

void Manager::resumeMusic() {
    if (!initialized_) return;
    pausedForBlur_ = false;
    MusicTrack* t = static_cast<MusicTrack*>(musicFor(currentWorld_));
    // Only resume if music is actually on (volume > 0); if it was off, leave it
    // stopped and let updateMusic() handle (re)start when volume is raised.
    if (t && t->ok && GetMusicVolume01() > 0.0f) ResumeMusicStream(t->music);
}

Manager& Get() {
    static Manager instance;
    return instance;
}

} // namespace AudioManager
