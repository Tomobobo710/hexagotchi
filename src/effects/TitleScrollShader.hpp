#ifndef TITLE_SCROLL_SHADER_HPP
#define TITLE_SCROLL_SHADER_HPP

#include "SceneEffect.hpp"
#include "raylib.h"

// TitleScrollShader - a slowly drifting animated hexagon grid background,
// each hexagon colored with one of the 9 cast members' identity colors
// (pulled from CharacterRegistry so the palette IS the cast).
//
// This is a SceneEffect that renders a fullscreen quad with the shader.
// Usage:
//     TitleScrollShader* effect = new TitleScrollShader();
//     scene->addEffect(effect);
//
// The shader uses:
//   - time: drives the scrolling animation
//   - palette[9]: the cast's name colors, one per hex by its per-cell hash

class TitleScrollShader : public SceneEffect {
public:
    static const int PALETTE_SIZE = 9;

    TitleScrollShader();

    void init() override;
    void update(float deltaTime) override;
    void drawBackground() override;
    void cleanup() override;

private:
    Shader shader_ = {0};
    bool loaded_ = false;

    int timeLoc_ = -1;
    int aspectLoc_ = -1;
    int paletteLoc_ = -1;

    float time_ = 0.0f;

    // The 9 cast identity colors, filled from CharacterRegistry in init().
    Color palette_[PALETTE_SIZE] = {};

    // Texture for the fullscreen quad (1x1 white - not sampled by shader)
    Texture2D texture_ = {0};
};

// Helper function to load the title scroll shader directly
Shader LoadTitleScrollShader();

#endif // TITLE_SCROLL_SHADER_HPP
