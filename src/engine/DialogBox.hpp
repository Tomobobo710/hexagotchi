#ifndef DIALOG_BOX_HPP
#define DIALOG_BOX_HPP

#include "raylib.h"
#include <string>
#include <vector>
#include <functional>

// DialogBox constants
const Color DIALOG_BG_COLOR = {0, 0, 0, 200};
const Color DIALOG_BORDER_COLOR = WHITE;
const Color DIALOG_TEXT_COLOR = WHITE;
const Color DIALOG_SELECTED_COLOR = YELLOW;
const int DIALOG_DEFAULT_FONT_SIZE = 30;
const float DIALOG_DEFAULT_PADDING = 15.0f;
const float DIALOG_DEFAULT_BORDER = 2.0f;
const float DIALOG_CHAR_REVEAL_SPEED = 50.0f;  // Slower reveal for readability
const float DIALOG_FADE_DURATION = 0.3f;
const float DIALOG_OPTION_SPACING = 12.0f;
const float DIALOG_PORTRAIT_SIZE = 128.0f;
const float DIALOG_PORTRAIT_PADDING = 16.0f;

class DialogBox {
public:
    DialogBox(Vector2 position, float width, float height);
    
    void update(float deltaTime);
    void draw();
    
    void setText(const std::string& text);
    void setSpeakerName(const std::string& name);
    void setSpeakerColor(Color color);
    void setPortraitColor(Color color);
    void setPortraitGradient(Color top, Color bottom);

    // Optional portrait image. Caller loads/unloads the Texture2D (same
    // convention as SceneActor::setTexture / Button::setTexture). When set,
    // it's drawn stretched to fill the portrait box instead of the default
    // placeholder silhouette. clearPortraitTexture() reverts to the
    // silhouette without needing to unload/reset the texture yourself.
    void setPortraitTexture(Texture2D tex);
    void clearPortraitTexture();
    void setOptions(const std::vector<std::string>& opts);
    void addOption(const std::string& option);
    void clearOptions();
    
    void setTextColor(Color color);
    void setBackgroundColor(Color color);
    void setBorderColor(Color color);
    void setSelectedOptionColor(Color color);
    void setFontSize(int size);
    void setPadding(float p);
    void setBorderThickness(float t);
    void setOptionSpacing(float spacing);
    
    void setCharacterRevealSpeed(float charsPerSecond);
    void setFadeSpeed(float duration);
    void skipCharacterReveal();
    
    void show();
    void hide();
    bool isVisible() const;
    bool isFinished() const;
    bool hasOptions() const;
    int getSelectedOption() const;
    void selectOption(int index);
    void selectNextOption();
    void selectPreviousOption();
    void confirmSelection();
    
    void setPosition(Vector2 pos);
    Vector2 getPosition() const;
    void setAnchor(const std::string& anchor);
    
    void setOnOptionSelected(std::function<void(int)> callback);
    void setOnFinished(std::function<void()> callback);
    
    // Debug
    float getFadeAlpha() const { return fadeAlpha; }
    void drawDebugBounds() const;
    
protected:
    std::string fullText;
    std::string speakerName;
    Color speakerColor;
    Color portraitColor;
    Color portraitGradientTop;
    Color portraitGradientBottom;
    bool portraitUseGradient;
    Texture2D portraitTexture;
    std::vector<std::string> options;
    std::vector<std::string> wrappedTextLines;
    
    Vector2 position;
    float width, height;
    std::string anchorPoint;
    
    int currentCharIndex;
    int selectedOptionIndex;
    
    Color textColor;
    Color backgroundColor;
    Color borderColor;
    Color selectedOptionColor;
    
    int fontSize;
    float padding;
    float borderThickness;
    float optionSpacing;
    
    float charRevealSpeed;
    float charAccumulator;
    float fadeSpeed;
    bool visible;
    float fadeAlpha;
    float fadeTimer;
    
    std::function<void(int)> onOptionSelected;
    std::function<void()> onFinished;
    
    void wrapText();
    void updateFade(float deltaTime);
    void updateCharReveal(float deltaTime);
    float getTextLineHeight() const;
    float getTextAreaX() const;
    float getTextAreaWidth() const;
};

#endif // DIALOG_BOX_HPP
