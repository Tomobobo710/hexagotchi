#ifndef SPRITE_LOADER_HPP
#define SPRITE_LOADER_HPP

#include "raylib.h"
#include <vector>
#include <string>

// Sprite loader utility - loads animation frames from assets
namespace SpriteLoader {
    // Load animation frames from packed assets
    // basePath is relative to assets/ directory
    // Returns vector of textures (caller must unload)
    std::vector<Texture2D> loadFrames(const std::string& basePath, const std::string& action);
}

#endif // SPRITE_LOADER_HPP
