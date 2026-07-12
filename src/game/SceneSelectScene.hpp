#ifndef SCENE_SELECT_SCENE_HPP
#define SCENE_SELECT_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "SceneManager.hpp"
#include <vector>
#include <memory>
#include <string>

// Hub scene listing the gotchi's "world" side-scenes (pizza parlor, etc,
// owned by the narrative side of the project -- not the tomagotchi/pet-sim
// scenes). One button per registered scene; clicking a button switches the
// SceneManager to that scene. New world-scenes get added here as they're
// built, instead of claiming another number key in main.cpp.
class SceneSelectScene : public Scene {
public:
    SceneSelectScene(SceneManager* manager);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;

private:
    SceneManager* sceneManager;
    std::vector<std::unique_ptr<Button>> buttons;

    void addSceneButton(const std::string& label, const std::string& sceneName, float y);
    void addSceneButtonAt(const std::string& label, const std::string& sceneName, float x, float y);
    void addSceneButtonAt(const std::string& label, const std::string& sceneName, float x, float y, float width);
};

#endif // SCENE_SELECT_SCENE_HPP
