#include "SceneInputHandler.hpp"
#include <fstream>
#include <sstream>
#include <cstring>
#include <cstdio>

SceneInputHandler::SceneInputHandler(SceneCamera* cam)
    : camera(cam), lastKeyPressed(-1), lastCharTyped(""),
      waitingForInput(false), capturedKey(-1), triggerKeyPressed(-1) {
    setDefaultBindings();
}

void SceneInputHandler::update() {
    // Store previous states
    previousKeyState = currentKeyState;
    previousMouseState = currentMouseState;

    // Track last key pressed
    lastKeyPressed = GetKeyPressed();
    lastCharTyped = "";
    int charPressed = GetCharPressed();
    if (charPressed > 0) {
        lastCharTyped = static_cast<char>(charPressed);
    }

    // Handle input capture mode
    // Only capture if we were already in waiting state AND a new key was pressed
    // Ignore the key that triggered entering capture mode (e.g., Enter to start binding)
    // Also ignore keys that are typically used for navigation/cancel (ESC, BACKSPACE)
    if (waitingForInput && anyKeyPressed() && lastKeyPressed != -1) {
        // Skip the key that triggered capture mode
        bool isNavigationKey = (lastKeyPressed == triggerKeyPressed ||
                                lastKeyPressed == KEY_ESCAPE ||
                                lastKeyPressed == KEY_BACKSPACE);
        if (!isNavigationKey) {
            capturedKey = lastKeyPressed;
            waitingForInput = false;
        }
    }

    // Update all mapped keys
    for (auto& pair : actionKeyMap) {
        updateKeyState(pair.second);
    }

    // Update mouse buttons
    updateMouseState(MOUSE_BUTTON_LEFT);
    updateMouseState(MOUSE_BUTTON_RIGHT);
    updateMouseState(MOUSE_BUTTON_MIDDLE);
}

bool SceneInputHandler::isKeyPressed(int key) {
    return IsKeyPressed(key);
}

bool SceneInputHandler::isKeyHeld(int key) {
    return IsKeyDown(key);
}

bool SceneInputHandler::isKeyReleased(int key) {
    return IsKeyReleased(key);
}

Vector2 SceneInputHandler::getMousePosition() const {
    return GetMousePosition();
}

Vector2 SceneInputHandler::getMouseWorldPosition() const {
    if (!camera) return GetMousePosition();
    return camera->screenToWorld(GetMousePosition());
}

bool SceneInputHandler::isMouseButtonPressed(int button) {
    return IsMouseButtonPressed(button);
}

bool SceneInputHandler::isMouseButtonHeld(int button) {
    return IsMouseButtonDown(button);
}

bool SceneInputHandler::isMouseButtonReleased(int button) {
    return IsMouseButtonReleased(button);
}

float SceneInputHandler::getMouseWheel() const {
    return GetMouseWheelMove();
}

void SceneInputHandler::mapKey(const std::string& action, int key) {
    actionKeyMap[action] = key;
}

void SceneInputHandler::unmapKey(const std::string& action) {
    actionKeyMap.erase(action);
}

int SceneInputHandler::getMappedKey(const std::string& action) const {
    auto it = actionKeyMap.find(action);
    return (it != actionKeyMap.end()) ? it->second : -1;
}

void SceneInputHandler::setDefaultBindings() {
    actionKeyMap.clear();
    // Generic bindings (for compatibility)
    mapKey(INPUT_ACTION_MOVE_LEFT, INPUT_DEFAULT_MOVE_LEFT);
    mapKey(INPUT_ACTION_MOVE_RIGHT, INPUT_DEFAULT_MOVE_RIGHT);
    mapKey(INPUT_ACTION_MOVE_UP, INPUT_DEFAULT_MOVE_UP);
    mapKey(INPUT_ACTION_MOVE_DOWN, INPUT_DEFAULT_MOVE_DOWN);
    mapKey(INPUT_ACTION_JUMP, INPUT_DEFAULT_JUMP);
    mapKey(INPUT_ACTION_INTERACT, INPUT_DEFAULT_INTERACT);
    mapKey(INPUT_ACTION_PAUSE, INPUT_DEFAULT_PAUSE);
    mapKey(INPUT_ACTION_ACCEPT, INPUT_DEFAULT_ACCEPT);
    mapKey(INPUT_ACTION_CANCEL, INPUT_DEFAULT_CANCEL);
    mapKey(INPUT_ACTION_MOUSE_LEFT, MOUSE_BUTTON_LEFT);
    mapKey(INPUT_ACTION_MOUSE_RIGHT, MOUSE_BUTTON_RIGHT);
    mapKey(INPUT_ACTION_MOUSE_MIDDLE, MOUSE_BUTTON_MIDDLE);
}

bool SceneInputHandler::isActionPressed(const std::string& action) {
    int key = getMappedKey(action);
    if (key == -1) return false;
    return IsKeyPressed(key);
}

bool SceneInputHandler::isActionHeld(const std::string& action) {
    int key = getMappedKey(action);
    if (key == -1) return false;
    return IsKeyDown(key);
}

bool SceneInputHandler::isActionReleased(const std::string& action) {
    int key = getMappedKey(action);
    if (key == -1) return false;
    return IsKeyReleased(key);
}

void SceneInputHandler::setCamera(SceneCamera* cam) {
    camera = cam;
}

int SceneInputHandler::getLastKeyPressed() const {
    return lastKeyPressed;
}

std::string SceneInputHandler::getLastCharTyped() const {
    return lastCharTyped;
}

bool SceneInputHandler::anyKeyPressed() const {
    return lastKeyPressed != -1;
}

void SceneInputHandler::updateKeyState(int key) {
    previousKeyState[key] = currentKeyState[key];
    currentKeyState[key] = IsKeyDown(key);
}

void SceneInputHandler::updateMouseState(int button) {
    previousMouseState[button] = currentMouseState[button];
    currentMouseState[button] = IsMouseButtonDown(button);
}

// Input capture mode methods
void SceneInputHandler::setWaitingForInput(bool waiting) {
    waitingForInput = waiting;
    capturedKey = -1;
    triggerKeyPressed = lastKeyPressed;  // Store the key that triggered capture to ignore it
}

bool SceneInputHandler::isWaitingForInput() const {
    return waitingForInput;
}

int SceneInputHandler::getCapturedKey() const {
    return capturedKey;
}

// Utility methods
std::vector<std::string> SceneInputHandler::getAllActions() const {
    std::vector<std::string> actions;
    for (const auto& pair : actionKeyMap) {
        actions.push_back(pair.first);
    }
    return actions;
}

std::string SceneInputHandler::getKeyDisplayName(int keyCode) {
    if (keyCode == -1) return "None";

    // Keyboard keys
    switch (keyCode) {
        case KEY_A: return "A"; case KEY_B: return "B"; case KEY_C: return "C";
        case KEY_D: return "D"; case KEY_E: return "E"; case KEY_F: return "F";
        case KEY_G: return "G"; case KEY_H: return "H"; case KEY_I: return "I";
        case KEY_J: return "J"; case KEY_K: return "K"; case KEY_L: return "L";
        case KEY_M: return "M"; case KEY_N: return "N"; case KEY_O: return "O";
        case KEY_P: return "P"; case KEY_Q: return "Q"; case KEY_R: return "R";
        case KEY_S: return "S"; case KEY_T: return "T"; case KEY_U: return "U";
        case KEY_V: return "V"; case KEY_W: return "W"; case KEY_X: return "X";
        case KEY_Y: return "Y"; case KEY_Z: return "Z";

        case KEY_ZERO: return "0"; case KEY_ONE: return "1"; case KEY_TWO: return "2";
        case KEY_THREE: return "3"; case KEY_FOUR: return "4"; case KEY_FIVE: return "5";
        case KEY_SIX: return "6"; case KEY_SEVEN: return "7"; case KEY_EIGHT: return "8";
        case KEY_NINE: return "9";

        case KEY_SPACE: return "Space"; case KEY_ENTER: return "Enter";
        case KEY_ESCAPE: return "Esc"; case KEY_BACKSPACE: return "Backspace";
        case KEY_TAB: return "Tab"; case KEY_DELETE: return "Del";

        case KEY_LEFT: return "Left"; case KEY_RIGHT: return "Right";
        case KEY_UP: return "Up"; case KEY_DOWN: return "Down";

        case KEY_LEFT_SHIFT: return "LShift"; case KEY_RIGHT_SHIFT: return "RShift";
        case KEY_LEFT_CONTROL: return "LCtrl"; case KEY_RIGHT_CONTROL: return "RCtrl";
        case KEY_LEFT_ALT: return "LAlt"; case KEY_RIGHT_ALT: return "RAlt";

        default:
            // Fallback to key name if available
            const char* name = GetKeyName(keyCode);
            return (name && strlen(name) > 0) ? std::string(name) : "Unknown";
    }
}

std::string SceneInputHandler::getMouseButtonName(int buttonCode) {
    switch (buttonCode) {
        case MOUSE_BUTTON_LEFT: return "Mouse Left";
        case MOUSE_BUTTON_RIGHT: return "Mouse Right";
        case MOUSE_BUTTON_MIDDLE: return "Mouse Middle";
        default: return "Mouse " + std::to_string(buttonCode);
    }
}

std::string SceneInputHandler::getBindingDisplayName(int keyCode) {
    if (keyCode == -1) return "None";

    // Keyboard keys
    switch (keyCode) {
        case KEY_A: return "A"; case KEY_B: return "B"; case KEY_C: return "C";
        case KEY_D: return "D"; case KEY_E: return "E"; case KEY_F: return "F";
        case KEY_G: return "G"; case KEY_H: return "H"; case KEY_I: return "I";
        case KEY_J: return "J"; case KEY_K: return "K"; case KEY_L: return "L";
        case KEY_M: return "M"; case KEY_N: return "N"; case KEY_O: return "O";
        case KEY_P: return "P"; case KEY_Q: return "Q"; case KEY_R: return "R";
        case KEY_S: return "S"; case KEY_T: return "T"; case KEY_U: return "U";
        case KEY_V: return "V"; case KEY_W: return "W"; case KEY_X: return "X";
        case KEY_Y: return "Y"; case KEY_Z: return "Z";

        case KEY_ZERO: return "0"; case KEY_ONE: return "1"; case KEY_TWO: return "2";
        case KEY_THREE: return "3"; case KEY_FOUR: return "4"; case KEY_FIVE: return "5";
        case KEY_SIX: return "6"; case KEY_SEVEN: return "7"; case KEY_EIGHT: return "8";
        case KEY_NINE: return "9";

        case KEY_SPACE: return "Space"; case KEY_ENTER: return "Enter";
        case KEY_ESCAPE: return "Esc"; case KEY_BACKSPACE: return "Backspace";
        case KEY_TAB: return "Tab"; case KEY_DELETE: return "Del";

        case KEY_LEFT: return "Left"; case KEY_RIGHT: return "Right";
        case KEY_UP: return "Up"; case KEY_DOWN: return "Down";

        case KEY_LEFT_SHIFT: return "LShift"; case KEY_RIGHT_SHIFT: return "RShift";
        case KEY_LEFT_CONTROL: return "LCtrl"; case KEY_RIGHT_CONTROL: return "RCtrl";
        case KEY_LEFT_ALT: return "LAlt"; case KEY_RIGHT_ALT: return "RAlt";

        default:
            // Fallback to key name if available
            const char* name = GetKeyName(keyCode);
            return (name && strlen(name) > 0) ? std::string(name) : "Unknown";
    }
}

// File I/O for bindings
void SceneInputHandler::saveBindingsToFile(const std::string& filepath) {
    // Create directory if it doesn't exist (using std::filesystem - C++17)
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        std::string dirPath = filepath.substr(0, lastSlash);
#ifdef __EMSCRIPTEN__
        // Emscripten has no persistent file system by default, skip directory creation
#else
        // Try to create directory (this is a no-op if directory exists)
        // Using system() with mkdir for portability (works on Linux/Windows)
        std::string mkdirCmd = "mkdir -p \"" + dirPath + "\"";
        int result = system(mkdirCmd.c_str());
        (void)result;  // Suppress unused variable warning
#endif
    }

    std::ofstream file(filepath);
    if (!file.is_open()) {
        fprintf(stderr, "WARNING: Could not open bindings file for writing: %s\n", filepath.c_str());
        return;
    }

    file << "{\n";
    bool first = true;
    for (const auto& pair : actionKeyMap) {
        if (!first) file << ",\n";
        first = false;
        file << "  \"" << pair.first << "\": " << pair.second;
    }
    file << "\n}\n";

    file.close();
}

void SceneInputHandler::clearAllInputs() {
    // Clear all tracked input states to simulate release of all keys
    currentKeyState.clear();
    previousKeyState.clear();
    currentMouseState.clear();
    previousMouseState.clear();

    // Reset captured key and waiting state
    capturedKey = -1;
    waitingForInput = false;

    // Clear tracked key press info
    lastKeyPressed = -1;
    lastCharTyped = "";
}

bool SceneInputHandler::loadBindingsFromFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) return false;

    // Simple JSON parser for our specific format
    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();

    // Parse key-value pairs
    size_t pos = 0;
    while (pos < content.length()) {
        // Find opening quote for key
        size_t keyStart = content.find('"', pos);
        if (keyStart == std::string::npos) break;

        // Find closing quote for key
        size_t keyEnd = content.find('"', keyStart + 1);
        if (keyEnd == std::string::npos) break;

        std::string key = content.substr(keyStart + 1, keyEnd - keyStart - 1);

        // Find colon after key
        size_t colonPos = content.find(':', keyEnd);
        if (colonPos == std::string::npos) break;

        // Find the value (integer)
        int value = 0;
        std::istringstream iss(content.substr(colonPos + 1));
        if (!(iss >> value)) {
            pos = colonPos + 1;
            continue;
        }

        // Map the key to value
        actionKeyMap[key] = value;

        // Move past this entry
        pos = content.find('\n', colonPos);
        if (pos == std::string::npos) break;
    }

    file.close();
    return true;
}
