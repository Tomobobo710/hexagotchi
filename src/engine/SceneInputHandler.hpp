#ifndef SCENE_INPUT_HANDLER_HPP
#define SCENE_INPUT_HANDLER_HPP

#include "raylib.h"
#include "SceneCamera.hpp"
#include <string>
#include <map>
#include <vector>

// Input action constants - generic
const std::string INPUT_ACTION_MOVE_LEFT = "move_left";
const std::string INPUT_ACTION_MOVE_RIGHT = "move_right";
const std::string INPUT_ACTION_MOVE_UP = "move_up";
const std::string INPUT_ACTION_MOVE_DOWN = "move_down";
const std::string INPUT_ACTION_JUMP = "jump";
const std::string INPUT_ACTION_INTERACT = "interact";
const std::string INPUT_ACTION_PAUSE = "pause";
const std::string INPUT_ACTION_ACCEPT = "accept";
const std::string INPUT_ACTION_CANCEL = "cancel";

// Mouse button actions
const std::string INPUT_ACTION_MOUSE_LEFT = "mouse_left";
const std::string INPUT_ACTION_MOUSE_RIGHT = "mouse_right";
const std::string INPUT_ACTION_MOUSE_MIDDLE = "mouse_middle";

// Default key bindings
const int INPUT_DEFAULT_MOVE_LEFT = KEY_A;
const int INPUT_DEFAULT_MOVE_RIGHT = KEY_D;
const int INPUT_DEFAULT_MOVE_UP = KEY_W;
const int INPUT_DEFAULT_MOVE_DOWN = KEY_S;
const int INPUT_DEFAULT_JUMP = KEY_SPACE;
const int INPUT_DEFAULT_INTERACT = KEY_E;
const int INPUT_DEFAULT_PAUSE = KEY_P;
const int INPUT_DEFAULT_ACCEPT = KEY_ENTER;
const int INPUT_DEFAULT_CANCEL = KEY_ESCAPE;

class SceneInputHandler {
public:
    // Constructor
    SceneInputHandler(SceneCamera* cam = nullptr);
    
    // Update - call once per frame
    void update();
    
    // Keyboard queries
    bool isKeyPressed(int key);
    bool isKeyHeld(int key);
    bool isKeyReleased(int key);
    
    // Mouse queries
    Vector2 getMousePosition() const;
    Vector2 getMouseWorldPosition() const;
    bool isMouseButtonPressed(int button);
    bool isMouseButtonHeld(int button);
    bool isMouseButtonReleased(int button);
    float getMouseWheel() const;
    
    // Action mapping
    void mapKey(const std::string& action, int key);
    void unmapKey(const std::string& action);
    int getMappedKey(const std::string& action) const;
    void setDefaultBindings();

    bool isActionPressed(const std::string& action);
    bool isActionHeld(const std::string& action);
    bool isActionReleased(const std::string& action);

    // Input capture mode - for key rebinding UI
    void setWaitingForInput(bool waiting);
    bool isWaitingForInput() const;
    int getCapturedKey() const;

    // Utility methods
    std::vector<std::string> getAllActions() const;
    static std::string getBindingDisplayName(int keyCode);
    static std::string getKeyDisplayName(int keyCode);
    static std::string getMouseButtonName(int buttonCode);
    
    // Clear all inputs (for resuming from pause)
    void clearAllInputs();
    
    // File I/O for bindings persistence
    void saveBindingsToFile(const std::string& filepath);
    bool loadBindingsFromFile(const std::string& filepath);
    
    // Camera
    void setCamera(SceneCamera* cam);
    
    // Get all input this frame
    int getLastKeyPressed() const;
    std::string getLastCharTyped() const;
    bool anyKeyPressed() const;
    
protected:
    SceneCamera* camera;
    std::map<std::string, int> actionKeyMap;
    std::map<int, bool> previousKeyState;
    std::map<int, bool> currentKeyState;
    std::map<int, bool> previousMouseState;
    std::map<int, bool> currentMouseState;
    
    int lastKeyPressed;
    std::string lastCharTyped;
    
    // Input capture mode
    bool waitingForInput;
    int capturedKey;
    int triggerKeyPressed;  // Key that triggered entering capture mode (to ignore it)
    
    // Helper
    void updateKeyState(int key);
    void updateMouseState(int button);
};

#endif // SCENE_INPUT_HANDLER_HPP
