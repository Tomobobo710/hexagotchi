#ifndef SPRITE_LOADER_HPP
#define SPRITE_LOADER_HPP

#include "raylib.h"
#include <string>
#include <vector>

// Loads a numbered PNG sequence like assets/gotchis/001/idle_00.png,
// idle_01.png, ... into a vector of Texture2D, in frame order, for use with
// SceneActor::setAnimationFrames(). Caller owns and must UnloadTexture() each
// returned texture (e.g. via SpriteLoader::unloadFrames()).
//
// dir:    folder containing the frames, e.g. "assets/gotchis/001"
// prefix: action name, e.g. "idle" -> matches "idle_00.png", "idle_01.png", ...
// digits: zero-padded index width (2 -> "_00", "_01", ...)
//
// Stops at the first missing index. Returns an empty vector if frame 0 is
// missing (nothing to animate).
namespace SpriteLoader {
    std::vector<Texture2D> loadFrames(const std::string& dir, const std::string& prefix, int digits = 2);
    void unloadFrames(std::vector<Texture2D>& frames);
}

#endif // SPRITE_LOADER_HPP
