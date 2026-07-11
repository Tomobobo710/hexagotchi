#ifndef DEATH_SCENE_HPP
#define DEATH_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "raylib.h"
#include <memory>

// Final game-over screen -- reached when the gotchi actually dies (Health
// hits 0, see GotchiSim::update()'s collapse check). Reuses SceneManager's
// normal switchScene() fade (same mechanism GotchiScene/HexViewScene use to
// reach any other scene) rather than a custom merge/fade sequence -- there is
// no beat to play out here, just a static end card with one way out: TRY
// AGAIN, which fully resets globalGameState (see onTryAgain()) and starts a
// brand new playthrough from the title-screen's new-game entry point.
class DeathScene : public Scene {
public:
    DeathScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    Texture2D background_ = {0};
    std::unique_ptr<Button> tryAgainButton_;

    // Resets globalGameState to a brand-new default GameState (same pattern
    // TitleScene::onNewGame() uses) and routes back through the toy_animation
    // intro into GotchiScene, so a fresh playthrough (including the tutorial,
    // since tutorial_seen resets with everything else) starts cleanly.
    void onTryAgain();
};

#endif // DEATH_SCENE_HPP
