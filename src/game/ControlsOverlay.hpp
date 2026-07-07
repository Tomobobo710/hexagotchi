#ifndef CONTROLS_OVERLAY_HPP
#define CONTROLS_OVERLAY_HPP

#include "SceneInputHandler.hpp"
#include "Button.hpp"
#include <functional>
#include <string>
#include <vector>

// Key-rebinding screen shown from the pause menu. Click an action row to
// rebind it (press any key to capture), or the Back button to return to the
// pause menu. Reuses SceneInputHandler's existing rebinding support (the same
// mapKey/setWaitingForInput/getCapturedKey flow InputTestScene demos).
class ControlsOverlay {
public:
    explicit ControlsOverlay(SceneInputHandler* input);

    void open();
    void close();
    void update(float deltaTime);
    void draw();

    std::function<void()> onClose;

private:
    SceneInputHandler* input;
    Button backButton;
    std::vector<std::string> actions;

    bool rebinding = false;
    std::string rebindingAction;

    Rectangle rowBounds(int index) const;
    int rowAt(Vector2 point) const;

    static const int ROW_START_Y = 90;
    static const int ROW_HEIGHT = 28;
    static const int ROW_X = 60;
    static const int ROW_WIDTH = 600;
};

#endif // CONTROLS_OVERLAY_HPP
