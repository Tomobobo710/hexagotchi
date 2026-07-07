#ifndef HEX_TILE_HPP
#define HEX_TILE_HPP

#include "SceneActor.hpp"
#include "TileType.hpp"
#include <vector>

// Hex grid coordinates (axial q, r)
struct HexCoords {
    int q, r;

    HexCoords(int q_, int r_) : q(q_), r(r_) {}

    // Convert hex coordinates to pixel coordinates (flat-top hex)
    Vector2 toPixel(float hexSize) const;
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
