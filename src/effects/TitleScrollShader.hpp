#ifndef TITLE_SCROLL_SHADER_HPP
#define TITLE_SCROLL_SHADER_HPP

#include "SceneEffect.hpp"
#include "raylib.h"

// TitleScrollShader - an infinitely scrolling background shader inspired by
// Toe Tam & Earl's funky liquid/gas background effects. Uses perlin-style
// noise with multiple layers moving at different speeds and directions to
// create a rich, organic scroll effect.
//
// This is a SceneEffect that renders a fullscreen quad with the shader.
// Usage:
//     TitleScrollShader* effect = new TitleScrollShader();
//     scene->addEffect(effect);
//
// The shader uses:
//   - time: drives the scrolling animation
//   - color1, color2, color3: three colors for the organic pattern

class TitleScrollShader : public SceneEffect {
public:
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
    int color1Loc_ = -1;
    int color2Loc_ = -1;
    int color3Loc_ = -1;

    float time_ = 0.0f;

    // Three colors for the scrolling pattern
    Color color1_ = {64, 128, 192, 255};   // bluish
    Color color2_ = {128, 64, 160, 255};   // purplish
    Color color3_ = {192, 128, 64, 255};   // orangish

    // Texture for the fullscreen quad (1x1 white - not sampled by shader)
    Texture2D texture_ = {0};
};

// Helper function to load the title scroll shader directly
Shader LoadTitleScrollShader();

#endif // TITLE_SCROLL_SHADER_HPP
