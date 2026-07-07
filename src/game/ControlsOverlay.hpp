#ifndef CONTROLS_OVERLAY_HPP
#define CONTROLS_OVERLAY_HPP

#include "../engine/SceneInputHandler.hpp"
#include <string>
#include <vector>
#include <functional>

// Controls Overlay: Menu for configuring control bindings
// Embedded within GameScene (not a separate Scene) to preserve game state
class ControlsOverlay {
public:
    explicit ControlsOverlay(SceneInputHandler* inputHandler);

    void update(float deltaTime);
    void draw() const;

    bool isOpen() const { return open_; }
    void open();
    void close();

    // Callback when controls are closed (e.g., ESC pressed)
    std::function<void()> onClose;

private:
    SceneInputHandler* inputHandler_;
    bool open_;
    int selectedBindingIndex_;
    float fadeAlpha_;

    static const int MENU_X;
    static const int MENU_Y;
    static const int MENU_WIDTH;
    static const int MENU_HEIGHT;
    static const int ITEM_HEIGHT;

    // State for key binding
    bool waitingForInput_;
    std::vector<std::string> actionNames_;
    std::string statusMessage_;
    float messageTimer_;

    void updateMenu(float deltaTime);
    void updateNavigation(float deltaTime);
    void drawMenu() const;
    void loadBindings();
    void saveBindings();

    int getHoveredItem() const;
    void handleMouseClick();
    static std::string getBindingDisplayName(int keyCode);  // Returns display name for key code
    static std::string getKeyName(int keyCode);
};

#endif
