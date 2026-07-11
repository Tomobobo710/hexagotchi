#include "HexTile.hpp"
#include <cmath>
#include "AssetPack.hpp"

// Convert hex coordinates to pixel coordinates (pointy-top hex, odd-r offset)
// For pointy-top hexes, odd rows (r % 2 == 1) are offset by half a hex width
Vector2 HexCoords::toPixel(float hexSize) const {
    // Pointy-top: width = sqrt(3) * S (across flats, horizontal)
    //              height = 2 * S (corner to corner, vertical)
    const float width = HEX_SQRT3 * hexSize;
    const float height = 2.0f * hexSize;

    // x position: odd rows shifted right by half width
    // Adjust for HEX_FILL: the drawn tiles are scaled by this factor,
    // so reduce spacing to make them appear closer together
    float xOffset = width / HEX_FILL * ((float)q + 0.5f * (float)(r & 1));

    // y position: row spacing is 1.5 * S (3/4 of height)
    float yOffset = 0.75f * height * (float)r;

    return Vector2{xOffset, yOffset};
}

// Inverse: convert pixel coordinates to hex coordinates
HexCoords HexCoords::fromPixel(Vector2 p, float hexSize) {
    // Get row first (y is independent of q offset)
    // y = 0.75 * height * r = 0.75 * 2*S * r = 1.5 * S * r
    float r = p.y / (1.5f * hexSize);

    // x = width/HEX_FILL * (q + 0.5*(r & 1))
    // For rounding purposes, we use the continuous approximation first:
    // x ≈ width/HEX_FILL * (q + 0.5*r)   [treating r&1 as r for continuous]
    // q = x * HEX_FILL/width - 0.5*r
    float width = HEX_SQRT3 * hexSize;
    float q_float = p.x * HEX_FILL / width - 0.5f * r;

    // Round to nearest integer hex coordinates
    int q_rounded = static_cast<int>(std::round(q_float));
    int r_rounded = static_cast<int>(std::round(r));

    // Now verify: the actual x uses (r & 1), not r
    // If r is even, offset should be 0; if r is odd, offset should be 0.5
    // We need to adjust q based on this

    // Get the true row parity from the rounded r
    int r_parity = r_rounded & 1;

    // Recompute q with correct parity
    float q_true = p.x * HEX_FILL / width - 0.5f * (float)r_parity;
    int q_final = static_cast<int>(std::round(q_true));

    return HexCoords(q_final, r_rounded);
}

// Get bounding rectangle for hex at given position
Rectangle HexCoords::getBounds(float hexSize) const {
    Vector2 pixel = toPixel(hexSize);
    float w = HEX_SQRT3 * hexSize * HEX_FILL;
    float h = 2.0f * hexSize * HEX_FILL;
    return Rectangle{
        pixel.x - w * 0.5f,
        pixel.y - h * 0.5f,
        w, h
    };
}

HexTile::HexTile(HexCoords coords, TileType* tileType, float hexSize)
    : SceneActor({0, 0}, HEX_SQRT3 * hexSize, 2.0f * hexSize),
      coords(coords), tileType(tileType), hexSize(hexSize) {
    // Position will be updated by setHexPosition
    setHexPosition(coords);

    // Generate hex vertices (pointy-top)
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
    if (!tileType) return;
    std::string key = tileType->getTexturePath();  // e.g., "single_tiles/ocean/ocean_0.png"
    Texture2D texture = AssetPack::loadTexture(key);
    setTexture(texture);
}

void HexTile::updateVertices() {
    vertices.clear();

    // Pointy-top hexagon vertices (6 points), center-relative
    // Pointy-top: width = sqrt(3) * S (across flats), height = 2 * S (corner to corner)
    float hw = HEX_SQRT3 * hexSize * 0.5f;   // half width (across flats / 2)

    // Vertices: top point, then clockwise
    vertices.push_back(Vector2{0.0f,     -hexSize});    // Top point (y = -S, corner to corner / 2)
    vertices.push_back(Vector2{hw,       -hexSize * 0.5f});  // Upper-right
    vertices.push_back(Vector2{hw,        hexSize * 0.5f});  // Lower-right
    vertices.push_back(Vector2{0.0f,      hexSize});    // Bottom point
    vertices.push_back(Vector2{-hw,       hexSize * 0.5f});  // Lower-left
    vertices.push_back(Vector2{-hw,      -hexSize * 0.5f});  // Upper-left
}

void HexTile::draw() {
    if (!visible) return;

    // Draw the tile texture if loaded
    if (texture.id != 0) {
        // Draw texture scaled to fit the hex cell using DrawTexturePro
        // Pointy-top: width = sqrt(3) * S, height = 2 * S
        Rectangle src = { 0, 0, (float)texture.width, (float)texture.height };
        float w = HEX_SQRT3 * hexSize * HEX_FILL;
        float h = 2.0f  * hexSize * HEX_FILL;
        Rectangle dest = {
            position.x - w * 0.5f,
            position.y - h * 0.5f,
            w, h
        };
        DrawTexturePro(texture, src, dest, {0, 0}, 0.0f, color);
    } else if (tileType) {
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
    // Check collision using polygon with local coordinates
    // Vertices are center-relative, so subtract position from point
    Vector2 local = { point.x - position.x, point.y - position.y };
    return CheckCollisionPointPoly(local, vertices.data(), (int)vertices.size());
}

Rectangle HexTile::getBounds() const {
    return HexCoords(coords.q, coords.r).getBounds(hexSize);
}
