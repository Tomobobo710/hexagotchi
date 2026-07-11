#ifndef DEATH_SCENE_HPP
#define DEATH_SCENE_HPP

#include "Scene.hpp"
#include "raylib.h"

// Final game-over screen -- reached when the gotchi actually dies (Health
// hits 0, see Gotchi::updateStats()'s dead_ = true). Reuses SceneManager's
// normal switchScene() fade (same mechanism GotchiScene/HexViewScene use to
// reach any other scene) rather than a custom merge/fade sequence -- there is
// no beat to play out here, just a static end card. Deliberately a dead end:
// nothing in this scene ever switches away from it.
class DeathScene : public Scene {
public:
    DeathScene();

    void init() override;
    void draw() override;
    void cleanup() override;

private:
    Texture2D background_ = {0};
};

#endif // DEATH_SCENE_HPP
