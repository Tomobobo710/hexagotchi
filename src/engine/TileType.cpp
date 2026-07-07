#include "TileType.hpp"
#include <sstream>
#include <cmath>

TileType::TileType(const std::string& name, const std::string& biome, TileDepth depth)
    : name(name), biome(biome), depth(depth) {
    // Initialize allowed borders with reasonable defaults
    // By default, allow same biome on all sides
    for (int i = 0; i < 6; i++) {
        allowedBorders[i].push_back(biome);
    }
}

std::string TileType::getTexturePath() const {
    // Path format: single_tiles/{biome}/{name}.png (relative to assets/)
    // This matches the key format used by tools/pack_assets.cpp
    return "single_tiles/" + biome + "/" + name + ".png";
}

bool TileType::canBorderBiome(HexSide side, const std::string& otherBiome) const {
    int sideIndex = static_cast<int>(side);
    auto it = allowedBorders.find(sideIndex);
    if (it == allowedBorders.end()) return false;

    for (const auto& allowed : it->second) {
        if (allowed == otherBiome) return true;
    }
    return false;
}

void TileType::addAllowedBorder(HexSide side, const std::string& biome) {
    int sideIndex = static_cast<int>(side);
    allowedBorders[sideIndex].push_back(biome);
}

Vector2 TileType::getSideOffset(HexSide side, float hexSize) {
    // Offset from center to side, in pixel coordinates
    // Hex flat-top orientation
    float hexWidth = hexSize * 2.0f;
    float hexHeight = hexSize * std::sqrt(3.0f);

    switch (side) {
        case HexSide::TOP:
            return Vector2{0.0f, -hexHeight / 2.0f};
        case HexSide::TOP_RIGHT:
            return Vector2{hexWidth / 4.0f, -hexHeight / 4.0f};
        case HexSide::BOTTOM_RIGHT:
            return Vector2{hexWidth / 4.0f, hexHeight / 4.0f};
        case HexSide::BOTTOM:
            return Vector2{0.0f, hexHeight / 2.0f};
        case HexSide::BOTTOM_LEFT:
            return Vector2{-hexWidth / 4.0f, hexHeight / 4.0f};
        case HexSide::TOP_LEFT:
            return Vector2{-hexWidth / 4.0f, -hexHeight / 4.0f};
        default:
            return Vector2{0.0f, 0.0f};
    }
}
