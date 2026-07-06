#include "StarfieldEffect.hpp"
#include "SceneCamera.hpp"
#include "GameConstants.hpp"
#include <cmath>

StarfieldEffect::StarfieldEffect(SceneCamera* cam) : camera(cam) {}

void StarfieldEffect::init() {
    for (int i = 0; i < 200; i++)
        stars.push_back({(float)GetRandomValue(0, 4800), (float)GetRandomValue(0, GAME_H)});
    for (int i = 0; i < 80; i++)
        midStars.push_back({(float)GetRandomValue(0, 4800), (float)GetRandomValue(0, GAME_H)});
}

void StarfieldEffect::drawBackground() {
    float camX = camera->getRaylibCamera().target.x;
    for (auto& s : stars) {
        float sx = fmod(s.x - camX * 0.1f + 4800.0f, 4800.0f) * ((float)GAME_W / 4800.0f);
        DrawPixel((int)sx, (int)s.y, {180, 180, 220, 120});
    }
    for (auto& s : midStars) {
        float sx = fmod(s.x - camX * 0.25f + 4800.0f, 4800.0f) * ((float)GAME_W / 4800.0f);
        DrawCircle((int)sx, (int)s.y, 1.2f, {220, 220, 255, 180});
    }
}
