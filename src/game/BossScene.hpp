#ifndef BOSS_SCENE_HPP
#define BOSS_SCENE_HPP

#include "Scene.hpp"

class BossScene : public Scene {
public:
    float bossPhase = 0.0f;
    float groundY = 520.0f;
    bool landedLastFrame = true;

    BossScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;
};

#endif
