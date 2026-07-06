#include "HexTile.hpp"
#include <cmath>

// Convert hex coordinates to pixel coordinates (flat-top hex)
Vector2 HexCoords::toPixel(float hexSize) const {
    float x = hexSize * (3.0f / 2.0f * q);
    float y = hexSize * (std::sqrt(3.0f) * q / 2.0f + std::sqrt(3.0f) * r);
    return Vector2{x, y};
}

HexTile::HexTile(HexCoords coords, TileType* tileType, float hexSize)
    : SceneActor({0, 0}, hexSize * 2.0f, hexSize * std::sqrt(3.0f)),
      coords(coords), tileType(tileType), hexSize(hexSize) {
    // Position will be updated by setHexPosition
    setHexPosition(coords);

    // Generate hex vertices (flat-top)
    updateVertices();
}

void HexTile::setHexPosition(HexCoords hexCoords) {
    coords = hexCoords;
    Vector2 pixelPos = coords.toPixel(hexSize);
    // Center the tile on the pixel position
    position.x = pixelPos.x;
    position.y = pixelPos.y;
}

Vector2 HexTile::getWorldPosition() const {
    return position;
}

void HexTile::loadTexture() {
    std::string path = tileType->getTexturePath();
    Texture2D texture = LoadTexture(path.c_str());
    setTexture(texture);
}

void HexTile::updateVertices() {
    vertices.clear();

    // Flat-top hexagon vertices (6 points)
    // Starting from top and going clockwise
    float h = hexSize * std::sqrt(3.0f);
    float w = hexSize * 2.0f;

    // Vertices relative to center
    float halfWidth = hexSize;
    float halfHeight = h / 2.0f;

    // Top, Top-Right, Bottom-Right, Bottom, Bottom-Left, Top-Left
    vertices.push_back(Vector2{0, -halfHeight});                 // Top
    vertices.push_back(Vector2{halfWidth, -halfHeight / 2.0f});  // Top-Right
    vertices.push_back(Vector2{halfWidth, halfHeight / 2.0f});   // Bottom-Right
    vertices.push_back(Vector2{0, halfHeight});                  // Bottom
    vertices.push_back(Vector2{-halfWidth, halfHeight / 2.0f});  // Bottom-Left
    vertices.push_back(Vector2{-halfWidth, -halfHeight / 2.0f}); // Top-Left
}

void HexTile::draw() {
    if (!visible) return;

    // Draw the tile texture if loaded
    if (texture.id != 0) {
        DrawTextureV(texture, Vector2{position.x - hexSize, position.y - hexSize * std::sqrt(3.0f) / 2.0f}, color);
    } else {
        // Draw hexagon outline/shape
        Color hexColor = tileType->getBiome() == "ocean" ? BLUE :
                        tileType->getBiome() == "sand" ? YELLOW :
                        tileType->getBiome() == "grass" ? GREEN :
                        tileType->getBiome() == "dirt" ? BROWN :
                        tileType->getBiome() == "ice" ? WHITE :
                        tileType->getBiome() == "lava" ? RED :
                        tileType->getBiome() == "space" ? PURPLE :
                        tileType->getBiome() == "swamp" ? DARKGREEN :
                        tileType->getBiome() == "autumn" ? ORANGE :
                        LIGHTGRAY;

        // Draw hexagon outline with line thickness
        DrawPolyLinesEx(position, 6, hexSize, 0.0f, 2.0f, hexColor);
        // Draw filled polygon using triangle fan approximation
        DrawPoly(position, 6, hexSize, 0.0f, hexColor);
    }
}

bool HexTile::containsPoint(Vector2 point) const {
    // Simple hex collision using polygon
    return CheckCollisionPointPoly(point, vertices.data(), (int)vertices.size());
}
