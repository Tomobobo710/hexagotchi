#include "HexTile.hpp"
#include <cmath>
#include "AssetPack.hpp"

static const float SQRT3 = 1.7320508f;
static const float HEX_FILL = 1.10f; // >1 covers transparent art margins

// Convert hex coordinates to pixel coordinates (pointy-top hex, odd-r offset)
// For pointy-top hexes, odd rows (r % 2 == 1) are offset by half a hex width
Vector2 HexCoords::toPixel(float hexSize) const {
    // Pointy-top: width = sqrt(3) * S (across flats, horizontal)
    //              height = 2 * S (corner to corner, vertical)
    const float width = SQRT3 * hexSize;
    const float height = 2.0f * hexSize;

    // x position: odd rows shifted right by half width
    float xOffset = width * ((float)q + 0.5f * (float)(r & 1));

    // y position: row spacing is 1.5 * S (3/4 of height)
    float yOffset = 0.75f * height * (float)r;

    return Vector2{xOffset, yOffset};
}

HexTile::HexTile(HexCoords coords, TileType* tileType, float hexSize)
    : SceneActor({0, 0}, SQRT3 * hexSize, 2.0f * hexSize),
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
    float hw = SQRT3 * hexSize * 0.5f;   // half width (across flats / 2)

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
    if (texture.id != 0) {
        static int n = 0;
        if (n < 3) {
            TraceLog(LOG_WARNING, "=== HEXDRAW pos=(%.0f,%.0f) texid=%d %dx%d a=%d ===",
                position.x, position.y, texture.id, texture.width, texture.height, color.a);
            n++;
        }
    }

    // Draw the tile texture if loaded
    if (texture.id != 0) {
        // Draw texture scaled to fit the hex cell using DrawTexturePro
        // Pointy-top: width = sqrt(3) * S, height = 2 * S
        Rectangle src = { 0, 0, (float)texture.width, (float)texture.height };
        float w = SQRT3 * hexSize * HEX_FILL;
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
