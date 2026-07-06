#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include "Scene.hpp"

class GameScene : public Scene {
public:
    bool landedLastFrame = true;
    float groundY = 560.0f;

    GameScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;
};

#endif
