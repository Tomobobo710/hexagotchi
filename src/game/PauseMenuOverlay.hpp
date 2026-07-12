#ifndef PAUSE_MENU_OVERLAY_HPP
#define PAUSE_MENU_OVERLAY_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include <functional>

// Simple pause menu overlay drawn on top of a paused Scene: a dimmed
// background panel with Resume / Options buttons. Owns no game state itself
// -- GameScene decides what paused actually does; this class only presents
// the menu and reports selections via callbacks.
class PauseMenuOverlay {
public:
    // Bound to one Scene instance -- used by scenes (GotchiScene,
    // HexViewScene) that own their PauseMenuOverlay for their whole lifetime
    // and call the no-arg update().
    explicit PauseMenuOverlay(Scene& scene);

    // Not bound to any scene -- used by main.cpp's single global instance
    // shared across Tom's story-world scenes, where the "current scene"
    // changes every time the player switches rooms. Callers pass the active
    // scene's input handler explicitly each frame (see SkipSceneOverlay for
    // the same convention).
    PauseMenuOverlay();

    void open();   // Reset button state each time the menu is (re)shown
    void close();
    void update(float deltaTime);                    // uses the bound scene's input handler
    void update(float deltaTime, SceneInputHandler* input); // explicit input handler
    void draw();

    std::function<void()> onResume;
    std::function<void()> onClose;
    std::function<void()> onOptionsSelected;

private:
    Scene* scene = nullptr;
    Button resumeButton;
    Button optionsButton;
};

#endif // PAUSE_MENU_OVERLAY_HPP
