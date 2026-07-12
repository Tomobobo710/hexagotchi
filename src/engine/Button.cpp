#include "Button.hpp"
#include "AudioManager.hpp"

Button::Button(Vector2 pos, float w, float h, const std::string& text)
    : label(text), position(pos), width(w), height(h), anchorPoint("top-left"),
      backgroundColor(BUTTON_BG_COLOR), hoverColor(BUTTON_BG_HOVER_COLOR),
      pressedColor(BUTTON_BG_PRESSED_COLOR), disabledColor(BUTTON_BG_DISABLED_COLOR),
      borderColor(BUTTON_BORDER_COLOR), textColor(BUTTON_TEXT_COLOR),
      disabledTextColor(BUTTON_TEXT_DISABLED_COLOR),
      fontSize(BUTTON_DEFAULT_FONT_SIZE), padding(BUTTON_DEFAULT_PADDING),
      borderThickness(BUTTON_DEFAULT_BORDER),
      texture({0}), hoverTexture({0}), pressedTexture({0}), disabledTexture({0}),
      tintColor(WHITE), drawFrameWithTexture(false),
      enabled(true), visible(true), hovered(false), pressed(false),
      hoverAmount(0.0f) {
}

Rectangle Button::getBounds() const {
    Vector2 topLeft = position;
    if (anchorPoint == "center") {
        topLeft.x -= width / 2.0f;
        topLeft.y -= height / 2.0f;
    } else if (anchorPoint == "top-center") {
        topLeft.x -= width / 2.0f;
    } else if (anchorPoint == "bottom") {
        topLeft.x -= width / 2.0f;
        topLeft.y -= height;
    }
    return {topLeft.x, topLeft.y, width, height};
}

void Button::update(SceneInputHandler* input, float deltaTime) {
    if (!visible || !enabled || !input) {
        hovered = false;
        pressed = false;
        hoverAmount = 0.0f;
        return;
    }

    Rectangle bounds = getBounds();
    Vector2 mouse = input->getMousePosition();
    hovered = CheckCollisionPointRec(mouse, bounds);

    // Begin a press only when the click starts over the button.
    if (hovered && input->isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        pressed = true;
    }

    // Release fires the click only if the cursor is still over the button.
    if (input->isMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        if (pressed && hovered && onClick) {
            AudioManager::Get().playClick();
            onClick();
        }
        pressed = false;
    }

    // Cancel a press if the mouse button is no longer held (e.g. released off-button).
    if (!input->isMouseButtonHeld(MOUSE_BUTTON_LEFT)) {
        pressed = false;
    }

    // Ease the hover tint toward its target.
    float target = hovered ? 1.0f : 0.0f;
    hoverAmount += (target - hoverAmount) * BUTTON_HOVER_FADE_SPEED * deltaTime;
    if (hoverAmount < 0.0f) hoverAmount = 0.0f;
    if (hoverAmount > 1.0f) hoverAmount = 1.0f;
}

static Color lerpColor(Color a, Color b, float t) {
    return {
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

const Texture2D* Button::resolveTexture() const {
    if (!enabled && disabledTexture.id != 0) return &disabledTexture;
    if (pressed && hovered && pressedTexture.id != 0) return &pressedTexture;
    if (hovered && hoverTexture.id != 0) return &hoverTexture;
    if (texture.id != 0) return &texture;
    return nullptr;
}

void Button::draw() {
    if (!visible) return;

    Rectangle bounds = getBounds();
    const Texture2D* activeTexture = resolveTexture();

    if (activeTexture) {
        // Stretch the sprite to fill the button bounds.
        Rectangle src = {0.0f, 0.0f, (float)activeTexture->width, (float)activeTexture->height};
        DrawTexturePro(*activeTexture, src, bounds, {0.0f, 0.0f}, 0.0f, tintColor);
        if (drawFrameWithTexture) {
            DrawRectangle((int)bounds.x, (int)bounds.y, (int)bounds.width, 3, borderColor);
            DrawRectangleLinesEx(bounds, borderThickness, borderColor);
        }
    } else {
        // Resolve the fill for the current state.
        Color fill;
        if (!enabled) {
            fill = disabledColor;
        } else if (pressed && hovered) {
            fill = pressedColor;
        } else {
            fill = lerpColor(backgroundColor, hoverColor, hoverAmount);
        }

        DrawRectangleRec(bounds, fill);

        // Thin top accent bar, matching the DialogBox look.
        DrawRectangle((int)bounds.x, (int)bounds.y, (int)bounds.width, 3, borderColor);
        DrawRectangleLinesEx(bounds, borderThickness, borderColor);
    }

    // Centered label.
    if (!label.empty()) {
        Color txt = enabled ? textColor : disabledTextColor;
        int textWidth = MeasureText(label.c_str(), fontSize);
        int textX = (int)(bounds.x + (bounds.width - textWidth) / 2.0f);
        int textY = (int)(bounds.y + (bounds.height - fontSize) / 2.0f);
        DrawText(label.c_str(), textX, textY, fontSize, txt);
    }

    // Tooltip: a small pill just above the button, only while hovered and
    // enabled -- a locked button shouldn't explain a feature the player
    // can't use yet.
    if (!tooltip.empty() && hovered && enabled) {
        int tipFontSize = 14;
        int tipPadding = 8;
        int tipWidth = MeasureText(tooltip.c_str(), tipFontSize);
        float tipX = bounds.x + (bounds.width - tipWidth) / 2.0f - tipPadding;
        float tipHeight = (float)(tipFontSize + tipPadding * 2);
        float tipY = bounds.y - tipHeight - 6.0f;

        Rectangle tipRect = { tipX, tipY, (float)(tipWidth + tipPadding * 2), tipHeight };
        DrawRectangleRounded(tipRect, 0.3f, 8, {0, 0, 0, 200});
        DrawText(tooltip.c_str(), (int)(tipX + tipPadding), (int)(tipY + tipPadding), tipFontSize, WHITE);
    }
}

void Button::setLabel(const std::string& text) { label = text; }
const std::string& Button::getLabel() const { return label; }

void Button::setTooltip(const std::string& text) { tooltip = text; }

void Button::setPosition(Vector2 pos) { position = pos; }
Vector2 Button::getPosition() const { return position; }
void Button::setSize(float w, float h) { width = w; height = h; }
void Button::setAnchor(const std::string& anchor) { anchorPoint = anchor; }

void Button::setBackgroundColor(Color color) { backgroundColor = color; }
void Button::setHoverColor(Color color) { hoverColor = color; }
void Button::setPressedColor(Color color) { pressedColor = color; }
void Button::setBorderColor(Color color) { borderColor = color; }
void Button::setTextColor(Color color) { textColor = color; }
void Button::setFontSize(int size) { fontSize = size; }
void Button::setPadding(float p) { padding = p; }
void Button::setBorderThickness(float t) { borderThickness = t; }

void Button::setTexture(Texture2D tex) { texture = tex; }
void Button::setHoverTexture(Texture2D tex) { hoverTexture = tex; }
void Button::setPressedTexture(Texture2D tex) { pressedTexture = tex; }
void Button::setDisabledTexture(Texture2D tex) { disabledTexture = tex; }
void Button::setTintColor(Color color) { tintColor = color; }
void Button::setDrawFrameWithTexture(bool draw) { drawFrameWithTexture = draw; }

void Button::setEnabled(bool e) { enabled = e; }
bool Button::isEnabled() const { return enabled; }
void Button::setVisible(bool v) { visible = v; }
bool Button::isVisible() const { return visible; }

bool Button::isHovered() const { return hovered; }
bool Button::isPressed() const { return pressed; }

void Button::setOnClick(std::function<void()> callback) { onClick = callback; }

void Button::drawDebugBounds() const {
    Rectangle bounds = getBounds();
    DrawRectangleLinesEx(bounds, 1.0f, RED);
    DrawCircleV(position, 3.0f, GREEN);  // Anchor point marker
}
