#include "InputTestScene.hpp"
#include "../effects/StarfieldEffect.hpp"

InputTestScene::InputTestScene()
    : Scene(720.0f, 720.0f, {18, 18, 30, 255}) {
    rebinding = false;
    rebindingAction = "";
    newKey = -1;
    bindingsChanged = false;
}

void InputTestScene::init() {
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);
    addEffect(new StarfieldEffect(getCamera()));
}

void InputTestScene::draw() {
    Scene::draw();

    // --- LEFT COLUMN (x: 16-340) ---
    int lx = 16;
    int ly = 20;

    DrawText("INPUT HANDLER", lx, ly, 20, {255, 255, 255, 255});
    ly += 28;

    auto ih = getInputHandler();

    // --- Left Column: Action Queries ---
    DrawText("ACTIONS", lx, ly, 14, {100, 200, 255, 255});
    ly += 18;

    if (ih) {
        const char* actionNames[] = {"move_left", "move_right", "move_up", "move_down",
                                        "jump", "interact", "pause", "accept", "cancel"};
        for (int i = 0; i < 9; i++) {
            std::string p = ih->isActionPressed(actionNames[i]) ? "P" : "";
            std::string h = ih->isActionHeld(actionNames[i]) ? "H" : "";
            std::string r = ih->isActionReleased(actionNames[i]) ? "R" : "";

            std::string label = actionNames[i];
            std::string status = p.empty() && h.empty() && r.empty() ? "---" : (p + h + r);

            Color color = {140, 140, 180, 255};
            if (!p.empty()) color = {100, 255, 100, 255};
            else if (!h.empty()) color = {255, 220, 80, 255};
            else if (!r.empty()) color = {255, 100, 100, 255};

            DrawText(label.c_str(), lx, ly, 11, {180, 180, 220, 255});
            DrawText(status.c_str(), lx + 110, ly, 11, color);
            ly += 15;
        }
    }

    ly += 8;

    // --- Left Column: Raw Keys ---
    DrawText("RAW KEYS", lx, ly, 14, {100, 200, 255, 255});
    ly += 18;

    if (ih) {
        int rawKeys[] = {KEY_A, KEY_D, KEY_W, KEY_S, KEY_SPACE, KEY_LEFT_SHIFT,
                         KEY_E, KEY_P, KEY_ENTER, KEY_ESCAPE};
        const char* rawNames[] = {"A", "D", "W", "S", "Space", "LShift",
                                   "E", "P", "Enter", "Esc"};
        for (int i = 0; i < 10; i++) {
            std::string p = ih->isKeyPressed(rawKeys[i]) ? "P" : "";
            std::string h = ih->isKeyHeld(rawKeys[i]) ? "H" : "";
            std::string r = ih->isKeyReleased(rawKeys[i]) ? "R" : "";

            std::string status = p.empty() && h.empty() && r.empty() ? "---" : (p + h + r);

            Color color = {140, 140, 180, 255};
            if (!p.empty()) color = {100, 255, 100, 255};
            else if (!h.empty()) color = {255, 220, 80, 255};
            else if (!r.empty()) color = {255, 100, 100, 255};

            DrawText(rawNames[i], lx, ly, 11, {180, 180, 220, 255});
            DrawText(status.c_str(), lx + 60, ly, 11, color);
            ly += 15;
        }
    }

    ly += 8;

    // --- Left Column: Last Input ---
    DrawText("LAST INPUT", lx, ly, 14, {100, 200, 255, 255});
    ly += 18;

    if (ih) {
        int lk = ih->getLastKeyPressed();
        if (lk >= 0) {
            DrawText("Key:", lx, ly, 11, {180, 180, 220, 255});
            DrawText(SceneInputHandler::getKeyDisplayName(lk).c_str(), lx + 50, ly, 11, {255, 255, 100, 255});
            ly += 15;
        }
        std::string ch = ih->getLastCharTyped();
        if (!ch.empty()) {
            DrawText("Char:", lx, ly, 11, {180, 180, 220, 255});
            char cbuf[32];
            snprintf(cbuf, sizeof(cbuf), "'%c'", ch[0]);
            DrawText(cbuf, lx + 55, ly, 11, {255, 255, 100, 255});
            ly += 15;
        }
        if (ih->anyKeyPressed()) {
            DrawText("Any:", lx, ly, 11, {180, 180, 220, 255});
            DrawText("YES", lx + 45, ly, 11, {255, 255, 100, 255});
            ly += 15;
        }
        if (ly == 20 + 28 + 9*15 + 18 + 10*15 + 8 + 18) {
            // Nothing was printed
            DrawText("(none)", lx, ly, 11, {100, 100, 140, 255});
            ly += 15;
        }
    }

    ly += 8;

    // --- Left Column: Action Bindings ---
    DrawText("BINDINGS", lx, ly, 14, {100, 200, 255, 255});
    ly += 18;

    if (ih) {
        auto actions = ih->getAllActions();
        for (const auto& action : actions) {
            int key = ih->getMappedKey(action);
            std::string keyName = (key >= 0) ? SceneInputHandler::getKeyDisplayName(key) : "None";
            DrawText(action.c_str(), lx, ly, 11, {180, 180, 220, 255});
            DrawText(keyName.c_str(), lx + 110, ly, 11, {220, 220, 255, 255});
            ly += 14;
        }
    }

    // --- RIGHT COLUMN (x: 360-704) ---
    int rx = 360;
    int ry = 20;

    // --- Right Column: Mouse ---
    DrawText("MOUSE", rx, ry, 14, {100, 200, 255, 255});
    ry += 18;

    if (ih) {
        Vector2 mp = ih->getMousePosition();
        Vector2 mwp = ih->getMouseWorldPosition();

        DrawText("Screen:", rx, ry, 11, {180, 180, 220, 255});
        char mbuf[64];
        snprintf(mbuf, sizeof(mbuf), "(%.0f, %.0f)", mp.x, mp.y);
        DrawText(mbuf, rx + 65, ry, 11, {220, 220, 255, 255});
        ry += 15;

        DrawText("World:", rx, ry, 11, {180, 180, 220, 255});
        snprintf(mbuf, sizeof(mbuf), "(%.0f, %.0f)", mwp.x, mwp.y);
        DrawText(mbuf, rx + 60, ry, 11, {220, 220, 255, 255});
        ry += 15;

        const char* mbNames[] = {"Left", "Right", "Middle"};
        for (int b = 0; b < 3; b++) {
            int btn = b + 1;
            std::string ms = "OFF";
            Color color = {140, 140, 180, 255};
            if (ih->isMouseButtonPressed(btn)) { ms = "PRESSED"; color = {100, 255, 100, 255}; }
            else if (ih->isMouseButtonHeld(btn)) { ms = "HELD"; color = {255, 220, 80, 255}; }
            else if (ih->isMouseButtonReleased(btn)) { ms = "RELEASED"; color = {255, 100, 100, 255}; }

            DrawText(mbNames[b], rx, ry, 11, {180, 180, 220, 255});
            DrawText(ms.c_str(), rx + 60, ry, 11, color);
            ry += 15;
        }

        float wheel = ih->getMouseWheel();
        if (fabs(wheel) > 0.01f) {
            DrawText("Wheel:", rx, ry, 11, {180, 180, 220, 255});
            char wbuf[32];
            snprintf(wbuf, sizeof(wbuf), "%.2f", wheel);
            DrawText(wbuf, rx + 60, ry, 11, {255, 200, 100, 255});
            ry += 15;
        }
    }

    ry += 8;

    // --- Right Column: Rebinding ---
    DrawText("REBINDING", rx, ry, 14, {100, 200, 255, 255});
    ry += 18;

    if (ih) {
        auto actions = ih->getAllActions();
        for (const auto& action : actions) {
            int key = ih->getMappedKey(action);
            std::string keyName = (key >= 0) ? SceneInputHandler::getKeyDisplayName(key) : "None";

            if (rebinding && rebindingAction == action) {
                DrawText(action.c_str(), rx, ry, 11, {255, 255, 100, 255});
                if (ih->isWaitingForInput()) {
                    DrawText("...press key...", rx + 110, ry, 11, {255, 200, 50, 255});
                } else if (newKey >= 0) {
                    DrawText(("-> " + SceneInputHandler::getKeyDisplayName(newKey)).c_str(), rx + 110, ry, 11, {100, 255, 100, 255});
                    newKey = -1;
                    bindingsChanged = true;
                } else {
                    DrawText("...waiting...", rx + 110, ry, 11, {255, 200, 50, 255});
                }
            } else {
                DrawText(action.c_str(), rx, ry, 11, {180, 180, 220, 255});
                DrawText(keyName.c_str(), rx + 110, ry, 11, {220, 220, 255, 255});
            }
            ry += 14;
        }
    }

    ry += 8;

    // --- Right Column: Controls ---
    DrawText("CONTROLS", rx, ry, 14, {100, 200, 255, 255});
    ry += 18;

    DrawText("R: Toggle rebinding", rx, ry, 11, {180, 180, 220, 255}); ry += 15;
    DrawText("ESC: Clear all inputs", rx, ry, 11, {180, 180, 220, 255}); ry += 15;
    DrawText("L: Save bindings.json", rx, ry, 11, {180, 180, 220, 255}); ry += 15;
    DrawText("O: Load bindings.json", rx, ry, 11, {180, 180, 220, 255}); ry += 15;
    DrawText("1: Overworld", rx, ry, 11, {180, 180, 220, 255}); ry += 15;
    DrawText("2: Boss Arena", rx, ry, 11, {180, 180, 220, 255});

    if (bindingsChanged) {
        ry += 4;
        DrawText("* bindings changed", rx, ry, 10, {255, 200, 50, 255});
    }
}

void InputTestScene::update(float deltaTime) {
    Scene::update(deltaTime);

    auto ih = getInputHandler();

    // R: toggle rebinding mode
    if (ih && IsKeyPressed(KEY_R)) {
        if (rebinding) {
            rebinding = false;
            rebindingAction = "";
            newKey = -1;
            ih->setWaitingForInput(false);
        } else {
            rebinding = true;
            auto actions = ih->getAllActions();
            if (!actions.empty()) {
                rebindingAction = actions[0];
                ih->setWaitingForInput(true);
            }
        }
    }

    // Cycle to next action when current rebinding is done
    if (ih && rebinding && !ih->isWaitingForInput() && newKey >= 0) {
        auto actions = ih->getAllActions();
        int currentIdx = -1;
        for (int i = 0; i < (int)actions.size(); i++) {
            if (actions[i] == rebindingAction) { currentIdx = i; break; }
        }
        if (currentIdx >= 0 && currentIdx < (int)actions.size() - 1) {
            rebindingAction = actions[currentIdx + 1];
            ih->setWaitingForInput(true);
        } else {
            rebinding = false;
            rebindingAction = "";
        }
    }

    // Check for captured key during rebinding
    if (rebinding && ih && ih->isWaitingForInput()) {
        int captured = ih->getCapturedKey();
        if (captured >= 0) {
            newKey = captured;
        }
    }

    // ESC: clear all inputs
    if (IsKeyPressed(KEY_ESCAPE)) {
        if (ih) ih->clearAllInputs();
    }

    // L: save bindings
    if (ih && IsKeyPressed(KEY_L)) {
        ih->saveBindingsToFile("bindings.json");
        bindingsChanged = false;
    }

    // O: load bindings
    if (ih && IsKeyPressed(KEY_O)) {
        if (ih->loadBindingsFromFile("bindings.json")) {
            bindingsChanged = false;
        }
    }
}
