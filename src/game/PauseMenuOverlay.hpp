#ifndef PAUSE_MENU_OVERLAY_HPP
#define PAUSE_MENU_OVERLAY_HPP

#include "../engine/SceneInputHandler.hpp"
#include <string>
#include <vector>
#include <functional>

// Forward declaration
class GameScene;

// Pause Menu Overlay: UI overlay for pause menu within GameScene
// Handles input navigation and drawing without being a full Scene
class PauseMenuOverlay {
public:
    explicit PauseMenuOverlay(GameScene& parent);

    void update(float deltaTime, SceneInputHandler* handler);
    void draw() const;

    bool isOpen() const { return open_; }
    void open();
    void close();

    // Callbacks - called by GameScene when menu state changes
    std::function<void()> onClose;        // Called when menu closed
    std::function<void()> onResume;       // Called when resuming game

    // Option selection callbacks (for Controls/Exit)
    std::function<void()> onControlsSelected;
    std::function<void()> onExitSelected;

private:
    GameScene& parent_;
    bool open_;
    int selectedOptionIndex_;
    float fadeAlpha_;

    const std::vector<std::string> menuOptions = {"Resume Game", "Controls", "Exit Game"};

    // Constants
    static const int MENU_X;
    static const int MENU_Y;
    static const int MENU_WIDTH;
    static const int BUTTON_HEIGHT;
    static const int FONT_SIZE;

    void updateMenu(float deltaTime, SceneInputHandler* handler);
    void drawMenu() const;

    // Mouse interaction
    int getHoveredOption() const;
    void handleMouseClick(SceneInputHandler* handler);
};

#endif
