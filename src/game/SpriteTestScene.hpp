#ifndef SPRITE_TEST_SCENE_HPP
#define SPRITE_TEST_SCENE_HPP

#include "Scene.hpp"
#include "SceneActor.hpp"
#include <string>
#include <vector>
#include <map>

// Demo scene for SceneActor's sprite-sheet and frame-list animation support,
// using the real hexagotchi character art (assets/gotchis/001). Kept separate
// from InputTestScene so that scene stays focused on input-handler debugging.
class SpriteTestScene : public Scene {
public:
    SpriteTestScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;
    void cleanup() override;

private:
    std::map<std::string, std::vector<Texture2D>> animFrames;  // action name -> frames
    std::vector<std::string> actionNames;
    int selectedAction = 0;

    SceneActor* gotchi = nullptr;
    int clickCount = 0;

    void selectAction(int index);
};

#endif // SPRITE_TEST_SCENE_HPP
