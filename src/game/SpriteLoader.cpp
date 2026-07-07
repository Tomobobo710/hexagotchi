#include "SpriteLoader.hpp"
#include "AssetPack.hpp"

namespace SpriteLoader {

std::vector<Texture2D> loadFrames(const std::string& basePath, const std::string& action) {
    // Load animation frames from packed assets using AssetPack
    return AssetPack::loadFrames(basePath, action);
}

} // namespace SpriteLoader
