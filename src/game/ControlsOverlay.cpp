#include "ControlsOverlay.hpp"
#include "GameConstants.hpp"

static const Color CONTROLS_DIM_COLOR = {0, 0, 0, 190};
static const Color CONTROLS_ROW_COLOR = {30, 30, 60, 200};
static const Color CONTROLS_ROW_HOVER_COLOR = {50, 50, 100, 220};
static const Color CONTROLS_ROW_ACTIVE_COLOR = {80, 60, 20, 230};
static const Color CONTROLS_BORDER_COLOR = {100, 100, 200, 255};

ControlsOverlay::ControlsOverlay(SceneInputHandler* in)
    : input(in),
      backButton({(float)GAME_W / 2.0f, (float)GAME_H - 60.0f}, 160.0f, 40.0f, "BACK") {
    backButton.setAnchor("center");
    backButton.setOnClick([this]() { close(); });
    if (input) actions = input->getAllActions();
}

void ControlsOverlay::open() {
    rebinding = false;
    rebindingAction = "";
    if (input) input->setWaitingForInput(false);
}

void ControlsOverlay::close() {
    rebinding = false;
    rebindingAction = "";
    if (input) input->setWaitingForInput(false);
    if (onClose) onClose();
}

Rectangle ControlsOverlay::rowBounds(int index) const {
    return {(float)ROW_X, (float)(ROW_START_Y + index * ROW_HEIGHT), (float)ROW_WIDTH, (float)(ROW_HEIGHT - 4)};
}

int ControlsOverlay::rowAt(Vector2 point) const {
    for (int i = 0; i < (int)actions.size(); i++) {
        if (CheckCollisionPointRec(point, rowBounds(i))) return i;
    }
    return -1;
}

void ControlsOverlay::update(float deltaTime) {
    if (!input) return;

    backButton.update(input, deltaTime);

    // Finish an in-progress capture once a key comes in.
    if (rebinding && input->isWaitingForInput()) {
        int captured = input->getCapturedKey();
        if (captured >= 0) {
            input->mapKey(rebindingAction, captured);
            rebinding = false;
            rebindingAction = "";
        }
        return;  // Don't also process a click while capturing.
    }

    // Click a row to start rebinding it.
    if (!rebinding && input->isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        int row = rowAt(input->getMousePosition());
        if (row >= 0) {
            rebindingAction = actions[row];
            rebinding = true;
            input->setWaitingForInput(true);
        }
    }
}

void ControlsOverlay::draw() {
    DrawRectangle(0, 0, GAME_W, GAME_H, CONTROLS_DIM_COLOR);

    int titleWidth = MeasureText("CONTROLS", 26);
    DrawText("CONTROLS", (int)(GAME_W / 2.0f - titleWidth / 2.0f), 36, 26, WHITE);

    Vector2 mouse = input ? input->getMousePosition() : Vector2{-1, -1};

    for (int i = 0; i < (int)actions.size(); i++) {
        Rectangle bounds = rowBounds(i);
        bool isRebindingThis = rebinding && rebindingAction == actions[i];
        bool hovered = !rebinding && CheckCollisionPointRec(mouse, bounds);

        Color rowColor = isRebindingThis ? CONTROLS_ROW_ACTIVE_COLOR
                        : hovered ? CONTROLS_ROW_HOVER_COLOR
                        : CONTROLS_ROW_COLOR;
        DrawRectangleRec(bounds, rowColor);
        DrawRectangleLinesEx(bounds, 1.0f, CONTROLS_BORDER_COLOR);

        DrawText(actions[i].c_str(), (int)bounds.x + 12, (int)bounds.y + 6, 14, {200, 200, 240, 255});

        std::string keyLabel;
        if (isRebindingThis) {
            keyLabel = "press a key...";
        } else if (input) {
            int key = input->getMappedKey(actions[i]);
            keyLabel = (key >= 0) ? SceneInputHandler::getKeyDisplayName(key) : "None";
        }
        int keyWidth = MeasureText(keyLabel.c_str(), 14);
        Color keyColor = isRebindingThis ? YELLOW : Color{220, 220, 255, 255};
        DrawText(keyLabel.c_str(), (int)(bounds.x + bounds.width - keyWidth - 12), (int)bounds.y + 6, 14, keyColor);
    }

    DrawText("Click an action, then press a key to rebind it.", ROW_X, (int)(ROW_START_Y + actions.size() * ROW_HEIGHT + 16), 12, {150, 150, 190, 255});

    backButton.draw();
}
