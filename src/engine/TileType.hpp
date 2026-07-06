#ifndef TILE_TYPE_HPP
#define TILE_TYPE_HPP

#include <string>
#include <map>
#include <vector>
#include "raylib.h"

// Depth levels for water tiles
enum class TileDepth {
    SHALLOW = 0,
    MIDDLE = 1,
    DEEP = 2
};

// Direction sides of a hexagon (clockwise from top)
enum class HexSide {
    TOP = 0,
    TOP_RIGHT = 1,
    BOTTOM_RIGHT = 2,
    BOTTOM = 3,
    BOTTOM_LEFT = 4,
    TOP_LEFT = 5
};

// Position on a hex tile - center or one of the six sides
struct TilePosition {
    enum class Type {
        CENTER,
        SIDE_TOP,
        SIDE_TOP_RIGHT,
        SIDE_BOTTOM_RIGHT,
        SIDE_BOTTOM,
        SIDE_BOTTOM_LEFT,
        SIDE_TOP_LEFT
    } type;

    // For side positions, the angle in radians (0 = right, increasing counter-clockwise)
    float angle;

    TilePosition(Type t, float a = 0.0f) : type(t), angle(a) {}

    static TilePosition center() { return TilePosition(Type::CENTER); }

    static TilePosition sideTop() { return TilePosition(Type::SIDE_TOP, PI / 2.0f); }
    static TilePosition sideTopRight() { return TilePosition(Type::SIDE_TOP_RIGHT, PI / 6.0f); }
    static TilePosition sideBottomRight() { return TilePosition(Type::SIDE_BOTTOM_RIGHT, -PI / 6.0f); }
    static TilePosition sideBottom() { return TilePosition(Type::SIDE_BOTTOM, -PI / 2.0f); }
    static TilePosition sideBottomLeft() { return TilePosition(Type::SIDE_BOTTOM_LEFT, -5.0f * PI / 6.0f); }
    static TilePosition sideTopLeft() { return TilePosition(Type::SIDE_TOP_LEFT, 5.0f * PI / 6.0f); }
};

class TileType {
public:
    // Constructor
    TileType(const std::string& name, const std::string& biome, TileDepth depth = TileDepth::SHALLOW);

    // Getters
    std::string getName() const { return name; }
    std::string getBiome() const { return biome; }
    TileDepth getDepth() const { return depth; }

    // Texture path based on name and biome
    std::string getTexturePath() const;

    // Side compatibility - which biomes can border this tile safely
    bool canBorderBiome(HexSide side, const std::string& otherBiome) const;
    void addAllowedBorder(HexSide side, const std::string& biome);

    // Position offsets for hex grid layout
    static Vector2 getSideOffset(HexSide side, float hexSize);

private:
    std::string name;
    std::string biome;
    TileDepth depth;

    // Map: side -> list of allowed biomes
    std::map<int, std::vector<std::string>> allowedBorders;
};

#endif // TILE_TYPE_HPP
