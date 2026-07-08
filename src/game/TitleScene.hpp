#ifndef TITLE_SCENE_HPP
#define TITLE_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include <string>

// Scene 9 - Title screen scene
// Shows a black background with a "Start Game" button that transitions to GotchiScene
class TitleScene : public Scene {
public:
    TitleScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    Button* startButton = nullptr;
};

#endif // TITLE_SCENE_HPP
