#ifndef MOON_EFFECT_HPP
#define MOON_EFFECT_HPP

#include "SceneEffect.hpp"
#include "raylib.h"
#include "rlights.h"   // declarations only; RLIGHTS_IMPLEMENTATION lives in MoonEffect.cpp

class MoonEffect : public SceneEffect {
public:
    Shader shader;
    Model model;
    Light light;
    float rotation = 0.0f;

    void init() override;
    void update(float deltaTime) override;
    void drawBackground() override;
    void cleanup() override;
};

#endif
