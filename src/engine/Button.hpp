#ifndef BUTTON_HPP
#define BUTTON_HPP

#include "raylib.h"
#include "SceneInputHandler.hpp"
#include <string>
#include <functional>

// Button constants
const Color BUTTON_BG_COLOR = {30, 30, 60, 220};
const Color BUTTON_BG_HOVER_COLOR = {50, 50, 100, 240};
const Color BUTTON_BG_PRESSED_COLOR = {20, 20, 45, 255};
const Color BUTTON_BG_DISABLED_COLOR = {40, 40, 40, 160};
const Color BUTTON_BORDER_COLOR = {100, 100, 200, 255};
const Color BUTTON_TEXT_COLOR = WHITE;
const Color BUTTON_TEXT_DISABLED_COLOR = {140, 140, 140, 255};
const int BUTTON_DEFAULT_FONT_SIZE = 20;
const float BUTTON_DEFAULT_PADDING = 12.0f;
const float BUTTON_DEFAULT_BORDER = 2.0f;
const float BUTTON_HOVER_FADE_SPEED = 8.0f;  // How fast hover tint eases in/out

class Button {
public:
    Button(Vector2 position, float width, float height, const std::string& label = "");

    // Call once per frame. Reads mouse state through the engine's input handler
    // so all input flows through one place. update() fires onClick on release.
    void update(SceneInputHandler* input, float deltaTime);
    void draw();

    // Content
    void setLabel(const std::string& text);
    const std::string& getLabel() const;

    // Layout
    void setPosition(Vector2 pos);
    Vector2 getPosition() const;
    void setSize(float width, float height);
    void setAnchor(const std::string& anchor);  // "top-left", "center", "top-center", "bottom"

    // Appearance
    void setBackgroundColor(Color color);
    void setHoverColor(Color color);
    void setPressedColor(Color color);
    void setBorderColor(Color color);
    void setTextColor(Color color);
    void setFontSize(int size);
    void setPadding(float p);
    void setBorderThickness(float t);

    // Sprite appearance. Pass a loaded Texture2D (caller owns/unloads it, same
    // convention as SceneActor::setTexture). When a texture is set, it's drawn
    // stretched to the button bounds instead of the flat-color fill; the
    // border/accent bar still draw on top unless setDrawFrameWithTexture(false).
    // Per-state textures are optional overrides checked in this priority:
    // disabled -> pressed -> hover -> normal. Any state left unset falls back
    // to the normal texture, so setTexture() alone is enough for a static sprite.
    void setTexture(Texture2D tex);
    void setHoverTexture(Texture2D tex);
    void setPressedTexture(Texture2D tex);
    void setDisabledTexture(Texture2D tex);
    void setTintColor(Color color);          // Tint applied to the drawn texture
    void setDrawFrameWithTexture(bool draw); // Keep border/accent bar over a sprite

    // State
    void setEnabled(bool enabled);
    bool isEnabled() const;
    void setVisible(bool visible);
    bool isVisible() const;

    bool isHovered() const;
    bool isPressed() const;

    // Tooltip: a short line drawn just above the button while it's hovered
    // (and enabled -- a locked button doesn't need to explain itself).
    // Empty string (the default) means no tooltip is ever drawn.
    void setTooltip(const std::string& text);

    // Callback
    void setOnClick(std::function<void()> callback);

    // Debug
    void drawDebugBounds() const;

    // Bounds access (public for shader overlay)
    Rectangle getBounds() const;

protected:
    std::string label;

    Vector2 position;
    float width, height;
    std::string anchorPoint;

    Color backgroundColor;
    Color hoverColor;
    Color pressedColor;
    Color disabledColor;
    Color borderColor;
    Color textColor;
    Color disabledTextColor;

    int fontSize;
    float padding;
    float borderThickness;

    Texture2D texture;
    Texture2D hoverTexture;
    Texture2D pressedTexture;
    Texture2D disabledTexture;
    Color tintColor;
    bool drawFrameWithTexture;

    bool enabled;
    bool visible;
    bool hovered;
    bool pressed;      // Mouse is down and was pressed while over this button
    float hoverAmount; // 0..1 eased hover tint

    std::string tooltip;

    std::function<void()> onClick;

    const Texture2D* resolveTexture() const;  // Active-state texture, or null if none set
};

#endif // BUTTON_HPP
