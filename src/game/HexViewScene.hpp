#ifndef HEX_VIEW_SCENE_HPP
#define HEX_VIEW_SCENE_HPP

#include "Scene.hpp"
#include "HexWorld.hpp"
#include <memory>

class PauseMenuOverlay;

class HexViewScene : public Scene {
public:
    HexViewScene();
    ~HexViewScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;

private:
    // Pause functionality
    void togglePause() override;
    bool isPaused() const override { return paused; }

    // Callback for pause menu
    void onExitSelected();  // Request exit

private:
    HexWorld* world;
    std::unique_ptr<PauseMenuOverlay> pauseMenu;

    // Camera panning state
    bool cameraPanning;
};

#endif
