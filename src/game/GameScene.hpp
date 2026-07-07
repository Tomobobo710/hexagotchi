#ifndef GAME_SCENE_HPP
#define GAME_SCENE_HPP

#include "Scene.hpp"
#include <memory>

class PauseMenuOverlay;
class ControlsOverlay;

class GameScene : public Scene {
public:
    bool landedLastFrame = true;
    float groundY = 560.0f;

    GameScene();
    ~GameScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;

private:
    // Pause functionality
    void togglePause() override;
    bool isPaused() const override { return paused; }

    // Accessors for pause menu and controls overlay
    PauseMenuOverlay* getPauseMenu() const { return pauseMenu.get(); }
    ControlsOverlay* getControlsOverlay() const { return controlsOverlay.get(); }

    // Callbacks for pause menu
    void onControlsSelected();  // Open controls overlay
    void onExitSelected();      // Request exit

private:
    std::unique_ptr<PauseMenuOverlay> pauseMenu;
    std::unique_ptr<ControlsOverlay> controlsOverlay;
};

#endif
