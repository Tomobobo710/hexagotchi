#include "HexBoard.hpp"
#include "raylib.h"

HexBoard::HexBoard()
    : Scene(720.0f, 720.0f, {18, 18, 30, 255}),
      hexSize(30.0f),
      gridWidth(11),
      gridHeight(11),
      selectedHex(-1) {
}

void HexBoard::init() {
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);

    // Generate hex grid centers
    hexCenters.clear();
    float hexWidth = hexSize * 2.0f;
    float hexHeight = hexSize * 1.73205080757f;  // sqrt(3)

    int startX = (720 - (gridWidth - 1) * hexWidth / 2) / 2;
    int startY = (720 - (gridHeight - 1) * hexHeight / 2) / 2;

    for (int row = 0; row < gridHeight; row++) {
        for (int col = 0; col < gridWidth; col++) {
            float x = startX + col * hexWidth * 0.75f;
            float y = startY + row * hexHeight * 0.65f;
            if (col % 2 == 1) {
                y += hexHeight * 0.5f;
            }
            hexCenters.push_back({x, y});
        }
    }
}

void HexBoard::draw() {
    Scene::draw();

    // Draw hex grid
    for (size_t i = 0; i < hexCenters.size(); i++) {
        Vector2 center = hexCenters[i];
        Color color = (i == (size_t)selectedHex) ? Color{255, 255, 100, 255} : Color{100, 150, 255, 100};
        DrawPoly(center, 6, hexSize, 0.0f, color);

        // Draw hex coordinates
        char label[16];
        snprintf(label, sizeof(label), "%d,%d", (int)(i % gridWidth), (int)(i / gridWidth));
        DrawText(label, (int)center.x - 15, (int)center.y - 4, 8, Color{200, 200, 200, 255});
    }

    // Draw instructions
    DrawText("HEX BOARD", 14, 8, 18, Color{180, 180, 255, 255});
    DrawText("Mouse to select hex", 14, 32, 12, Color{140, 140, 180, 255});
    DrawText("1: Overworld  2: Boss  3: Hexboard  4: Input Test  ESC: Exit", 720 - 340, 8, 12, Color{140, 140, 180, 255});
}

void HexBoard::update(float deltaTime) {
    Scene::update(deltaTime);

    // Handle mouse input for hex selection
    Vector2 mousePos = getInputHandler()->getMouseWorldPosition();
    selectedHex = -1;

    for (size_t i = 0; i < hexCenters.size(); i++) {
        Vector2 center = hexCenters[i];
        float dist = sqrt((mousePos.x - center.x) * (mousePos.x - center.x) +
                          (mousePos.y - center.y) * (mousePos.y - center.y));
        if (dist < hexSize) {
            selectedHex = (int)i;
            break;
        }
    }
}
