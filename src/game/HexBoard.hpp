#ifndef HEX_BOARD_HPP
#define HEX_BOARD_HPP

#include "Scene.hpp"

class HexBoard : public Scene {
public:
    HexBoard();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;

private:
    // Hex grid properties
    float hexSize;
    int gridWidth;
    int gridHeight;

    // Hex center positions
    std::vector<Vector2> hexCenters;

    // Selected hex
    int selectedHex;
};

#endif
