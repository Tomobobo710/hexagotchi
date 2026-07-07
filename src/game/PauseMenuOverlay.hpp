#ifndef PAUSE_MENU_OVERLAY_HPP
#define PAUSE_MENU_OVERLAY_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include <functional>

// Simple pause menu overlay drawn on top of a paused Scene: a dimmed
// background panel with Resume / Controls / Exit buttons. Owns no game state
// itself -- GameScene decides what paused/controlsOverlay actually do; this
// class only presents the menu and reports selections via callbacks.
class PauseMenuOverlay {
public:
    explicit PauseMenuOverlay(Scene& scene);

    void open();   // Reset button state each time the menu is (re)shown
    void close();
    void update(float deltaTime);
    void draw();

    std::function<void()> onResume;
    std::function<void()> onClose;
    std::function<void()> onControlsSelected;
    std::function<void()> onExitSelected;

private:
    Scene& scene;
    Button resumeButton;
    Button controlsButton;
    Button exitButton;
};

#endif // PAUSE_MENU_OVERLAY_HPP
