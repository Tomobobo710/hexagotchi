#ifndef HEX_TILE_HPP
#define HEX_TILE_HPP

#include "raylib.h"
#include "SceneActor.hpp"
#include "TileType.hpp"
#include <vector>

// Hex grid constants
static const float HEX_SQRT3 = 1.7320508f;
static const float HEX_FILL = 1.10f; // >1 covers transparent art margins

// Hex grid coordinates (axial q, r)
struct HexCoords {
    int q, r;

    HexCoords(int q_, int r_) : q(q_), r(r_) {}

    // Convert hex coordinates to pixel coordinates (pointy-top hex, odd-r offset)
    Vector2 toPixel(float hexSize) const;

    // Inverse: convert pixel coordinates to hex coordinates
    static HexCoords fromPixel(Vector2 p, float hexSize);

    // Get bounding rectangle in world space
    Rectangle getBounds(float hexSize) const;
};

class HexTile : public SceneActor {
public:
    // Constructor
    HexTile(HexCoords coords, TileType* tileType, float hexSize = 64.0f);

    // Getters
    HexCoords getCoords() const { return coords; }
    TileType* getTileType() const { return tileType; }
    float getHexSize() const { return hexSize; }

    // Position in world space
    Vector2 getWorldPosition() const;

    // Set hex grid position
    void setHexPosition(HexCoords hexCoords);

    // Set/get texture from tile type
    void loadTexture();

    // Draw the hex tile
    void draw() override;

    // Get tile bounds for culling
    Rectangle getBounds() const;

    // Check if a point is inside this hex
    bool containsPoint(Vector2 point) const;

private:
    HexCoords coords;
    TileType* tileType;
    float hexSize;

    // Hex vertices for rendering and collision
    std::vector<Vector2> vertices;

    void updateVertices();
};

#endif // HEX_TILE_HPP
