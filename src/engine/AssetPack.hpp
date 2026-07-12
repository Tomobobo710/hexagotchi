#ifndef ASSET_PACK_HPP
#define ASSET_PACK_HPP

#include "raylib.h"
#include <string>
#include <vector>

// Thin wrapper around rres for loading textures out of a packed assets.rres
// file instead of loose files on disk. Fixes the desktop bug where game.exe
// couldn't find assets/ unless launched from the repo root -- assets.rres
// ships as one file next to game.exe (or is preloaded into the web build's
// virtual filesystem the same way loose assets were before), so there is no
// directory-relative lookup at all.
//
// Keys match the relative path used when packing (see tools/pack_assets.cpp),
// e.g. "gotchis/001/idle_00.png". This is the only texture-loading path in the
// engine now -- nothing reads loose files under assets/ directly.
namespace AssetPack {
    // Point at the .rres file to read from. Call once at startup before any
    // loadTexture() call. Defaults to "assets.rres" if never called.
    void setPackFile(const std::string& path);

    // Returns a texture with id == 0 if the key isn't found in the pack.
    // Caller owns the result and must UnloadTexture() it, same convention as
    // every other texture loader in the engine.
    Texture2D loadTexture(const std::string& key);

    // Convenience for numbered frame sequences packed under a shared prefix,
    // e.g. loadFrames("gotchis/001", "idle") looks up
    // "gotchis/001/idle_00.png", "gotchis/001/idle_01.png", ... stopping at
    // the first missing index.
    std::vector<Texture2D> loadFrames(const std::string& dir, const std::string& prefix, int digits = 2);

    // Unloads every texture in a vector returned by loadFrames(), then clears it.
    void unloadFrames(std::vector<Texture2D>& frames);

    // Loads an audio clip packed under `key` (e.g. "sfx/click.mp3"). The
    // packed bytes are the original file verbatim; this decodes them in memory
    // via LoadWaveFromMemory using the key's extension. Requires an audio
    // device (InitAudioDevice) to already be initialized. Returns a Sound with
    // frameCount == 0 if the key is missing or fails to decode. Caller owns the
    // result and must UnloadSound() it.
    Sound loadSound(const std::string& key);

    // Reads the raw packed bytes for `key` into a freshly malloc'd buffer.
    // Returns nullptr on miss. On success, *outSize gets the byte count and the
    // caller OWNS the buffer -- free it with UnloadFileData() (raylib's
    // MemFree). Used for streaming music: LoadMusicStreamFromMemory keeps a
    // pointer INTO this buffer, so it must outlive the Music stream (don't free
    // it until after UnloadMusicStream()).
    unsigned char* loadRawBytes(const std::string& key, unsigned int* outSize);
}

#endif // ASSET_PACK_HPP
