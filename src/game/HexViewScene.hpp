#ifndef HEX_VIEW_SCENE_HPP
#define HEX_VIEW_SCENE_HPP

#include "Scene.hpp"
#include "HexWorld.hpp"
#include "Gotchi.hpp"
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
    Gotchi* gotchi;
    std::unique_ptr<PauseMenuOverlay> pauseMenu;

    // Camera panning state
    bool cameraPanning;

    // World config (stored after init)
    float hexSize_;

    // Visual probe for click target
    bool  hasClickMarker_ = false;
    HexCoords clickMarkerHex_{0, 0};
    Vector2   clickMarkerWorld_{0, 0};
};

#endif
