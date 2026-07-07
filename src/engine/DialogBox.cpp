#include "DialogBox.hpp"
#include <sstream>
#include <algorithm>

DialogBox::DialogBox(Vector2 pos, float w, float h)
    : position(pos), width(w), height(h), anchorPoint("center"),
      speakerName(""), speakerColor(YELLOW), portraitColor({80, 80, 120, 255}),
      portraitTexture({0}),
      currentCharIndex(0), charAccumulator(0.0f), selectedOptionIndex(0),
      textColor(WHITE), backgroundColor({10, 10, 30, 220}),
      borderColor({100, 100, 200, 255}), selectedOptionColor(YELLOW),
      fontSize(DIALOG_DEFAULT_FONT_SIZE), padding(DIALOG_DEFAULT_PADDING),
      borderThickness(DIALOG_DEFAULT_BORDER), optionSpacing(DIALOG_OPTION_SPACING),
      charRevealSpeed(DIALOG_CHAR_REVEAL_SPEED), fadeSpeed(DIALOG_FADE_DURATION),
      visible(false), fadeAlpha(0.0f), fadeTimer(0.0f) {
}

void DialogBox::update(float deltaTime) {
    updateFade(deltaTime);
    if (visible) {
        updateCharReveal(deltaTime);
    }
}

void DialogBox::draw() {
    if (fadeAlpha <= 0.01f) return;

    if (wrappedTextLines.empty() && !fullText.empty()) {
        wrapText();
    }

    // Resolve top-left draw position
    Vector2 drawPos = position;
    if (anchorPoint == "center" || anchorPoint == "top-center") {
        drawPos.x -= width / 2.0f;
    } else if (anchorPoint == "bottom") {
        drawPos.x -= width / 2.0f;
        drawPos.y -= height;
    }

    unsigned char alpha = (unsigned char)(255 * fadeAlpha);

    // --- Background with subtle gradient feel via two rects ---
    Color bg = backgroundColor;
    bg.a = (unsigned char)(220 * fadeAlpha);
    DrawRectangle((int)drawPos.x, (int)drawPos.y, (int)width, (int)height, bg);

    // Thin top accent bar
    Color accent = borderColor;
    accent.a = alpha;
    DrawRectangle((int)drawPos.x, (int)drawPos.y, (int)width, 3, accent);

    // Outer border
    DrawRectangleLines((int)drawPos.x, (int)drawPos.y, (int)width, (int)height, accent);

    // --- Portrait box on the left ---
    float portraitX = drawPos.x + DIALOG_PORTRAIT_PADDING;
    float portraitY = drawPos.y + (height - DIALOG_PORTRAIT_SIZE) / 2.0f;

    // Portrait background
    Color portraitBg = portraitColor;
    portraitBg.a = alpha;
    DrawRectangle((int)portraitX, (int)portraitY, (int)DIALOG_PORTRAIT_SIZE, (int)DIALOG_PORTRAIT_SIZE, portraitBg);

    // Portrait border
    DrawRectangleLines((int)portraitX, (int)portraitY, (int)DIALOG_PORTRAIT_SIZE, (int)DIALOG_PORTRAIT_SIZE, accent);

    if (portraitTexture.id != 0) {
        // Real portrait art, stretched to fill the portrait box.
        Rectangle src = {0.0f, 0.0f, (float)portraitTexture.width, (float)portraitTexture.height};
        Rectangle dest = {portraitX, portraitY, DIALOG_PORTRAIT_SIZE, DIALOG_PORTRAIT_SIZE};
        Color tint = WHITE;
        tint.a = alpha;
        DrawTexturePro(portraitTexture, src, dest, {0.0f, 0.0f}, 0.0f, tint);
    } else {
        // No art set yet: placeholder character silhouette.
        Color silhouette = { (unsigned char)(portraitColor.r + 40), (unsigned char)(portraitColor.g + 40), (unsigned char)(portraitColor.b + 60), alpha };
        // Head
        DrawCircle((int)(portraitX + DIALOG_PORTRAIT_SIZE / 2), (int)(portraitY + DIALOG_PORTRAIT_SIZE * 0.32f), DIALOG_PORTRAIT_SIZE * 0.18f, silhouette);
        // Body
        DrawRectangle(
            (int)(portraitX + DIALOG_PORTRAIT_SIZE * 0.28f),
            (int)(portraitY + DIALOG_PORTRAIT_SIZE * 0.52f),
            (int)(DIALOG_PORTRAIT_SIZE * 0.44f),
            (int)(DIALOG_PORTRAIT_SIZE * 0.38f),
            silhouette
        );
    }

    // --- Speaker name above portrait ---
    if (!speakerName.empty()) {
        // Name plate background
        int nameWidth = MeasureText(speakerName.c_str(), fontSize);
        float nameX = drawPos.x + DIALOG_PORTRAIT_PADDING;
        float nameY = drawPos.y - (float)(fontSize + 8);
        Color nameBg = { 10, 10, 30, alpha };
        DrawRectangle((int)nameX - 6, (int)nameY - 4, nameWidth + 12, fontSize + 8, nameBg);
        DrawRectangleLines((int)nameX - 6, (int)nameY - 4, nameWidth + 12, fontSize + 8, accent);
        Color nameColor = speakerColor;
        nameColor.a = alpha;
        DrawText(speakerName.c_str(), (int)nameX, (int)nameY, fontSize, nameColor);
    }

    // --- Text area ---
    float textX = getTextAreaX() + drawPos.x;
    float textY = drawPos.y + padding;

    Color textWithAlpha = textColor;
    textWithAlpha.a = alpha;

    int charCount = 0;
    for (size_t i = 0; i < wrappedTextLines.size(); i++) {
        const std::string& line = wrappedTextLines[i];
        std::string displayLine = "";
        for (char c : line) {
            if (charCount < currentCharIndex) displayLine += c;
            charCount++;
        }
        if (!displayLine.empty()) {
            DrawText(displayLine.c_str(), (int)textX, (int)(textY + i * getTextLineHeight()), fontSize, textWithAlpha);
        }
    }

    if (false) {
    } else {
        // "Press SPACE" prompt when finished
        float blinkTime = (float)GetTime();
        if ((int)(blinkTime * 2) % 2 == 0) {
            Color promptColor = { 180, 180, 180, alpha };
            DrawText("SPACE", (int)(drawPos.x + width - padding - MeasureText("SPACE", fontSize - 4)), (int)(drawPos.y + height - padding - (fontSize - 4)), fontSize - 4, promptColor);
        }
    }

    // --- Options ---
    if (hasOptions() && isFinished()) {
        float optionsY = textY + wrappedTextLines.size() * getTextLineHeight() + 8;
        for (size_t i = 0; i < options.size(); i++) {
            bool selected = (i == (size_t)selectedOptionIndex);
            // Option highlight background
            if (selected) {
                Color highlightBg = { 60, 60, 120, alpha };
                DrawRectangle((int)(textX - 4), (int)(optionsY + i * (fontSize + optionSpacing) - 2),
                              MeasureText(options[i].c_str(), fontSize) + 24, fontSize + 4, highlightBg);
            }
            Color optionColor = selected ? YELLOW : WHITE;
            optionColor.a = alpha;
            std::string prefix = selected ? "> " : "  ";
            DrawText((prefix + options[i]).c_str(), (int)textX, (int)(optionsY + i * (fontSize + optionSpacing)), fontSize, optionColor);
        }
    }
}

void DialogBox::setSpeakerName(const std::string& name) { speakerName = name; }
void DialogBox::setSpeakerColor(Color c) { speakerColor = c; }
void DialogBox::setPortraitColor(Color c) { portraitColor = c; }
void DialogBox::setPortraitTexture(Texture2D tex) { portraitTexture = tex; }
void DialogBox::clearPortraitTexture() { portraitTexture = {0}; }

void DialogBox::setText(const std::string& text) {
    fullText = text;
    currentCharIndex = 0;
    selectedOptionIndex = 0;
    wrapText();
}

void DialogBox::setOptions(const std::vector<std::string>& opts) {
    options = opts;
    selectedOptionIndex = 0;
}

void DialogBox::addOption(const std::string& option) {
    options.push_back(option);
}

void DialogBox::clearOptions() {
    options.clear();
    selectedOptionIndex = 0;
}

void DialogBox::setTextColor(Color color) {
    textColor = color;
}

void DialogBox::setBackgroundColor(Color color) {
    backgroundColor = color;
}

void DialogBox::setBorderColor(Color color) {
    borderColor = color;
}

void DialogBox::setSelectedOptionColor(Color color) {
    selectedOptionColor = color;
}

void DialogBox::setFontSize(int size) {
    fontSize = size;
}

void DialogBox::setPadding(float p) {
    padding = p;
}

void DialogBox::setBorderThickness(float t) {
    borderThickness = t;
}

void DialogBox::setOptionSpacing(float spacing) {
    optionSpacing = spacing;
}

void DialogBox::setCharacterRevealSpeed(float charsPerSecond) {
    charRevealSpeed = charsPerSecond;
}

void DialogBox::setFadeSpeed(float duration) {
    fadeSpeed = duration;
}

void DialogBox::skipCharacterReveal() {
    currentCharIndex = fullText.length();
}

void DialogBox::show() {
    visible = true;
    fadeTimer = 0.0f;
    currentCharIndex = 0;
    charAccumulator = 0.0f;
    selectedOptionIndex = 0;
}

void DialogBox::hide() {
    visible = false;
    fadeTimer = fadeSpeed;
}

bool DialogBox::isVisible() const {
    return visible && fadeAlpha > 0.0f;
}

bool DialogBox::isFinished() const {
    return currentCharIndex >= (int)fullText.length();
}

bool DialogBox::hasOptions() const {
    return !options.empty();
}

int DialogBox::getSelectedOption() const {
    return selectedOptionIndex;
}

void DialogBox::selectOption(int index) {
    if (index >= 0 && index < (int)options.size()) {
        selectedOptionIndex = index;
    }
}

void DialogBox::selectNextOption() {
    if (options.empty()) return;
    selectedOptionIndex = (selectedOptionIndex + 1) % options.size();
}

void DialogBox::selectPreviousOption() {
    if (options.empty()) return;
    selectedOptionIndex = (selectedOptionIndex - 1 + options.size()) % options.size();
}

void DialogBox::confirmSelection() {
    if (onOptionSelected) {
        onOptionSelected(selectedOptionIndex);
    }
}

void DialogBox::setPosition(Vector2 pos) {
    position = pos;
}

Vector2 DialogBox::getPosition() const {
    return position;
}

void DialogBox::setAnchor(const std::string& anchor) {
    anchorPoint = anchor;
}

void DialogBox::setOnOptionSelected(std::function<void(int)> callback) {
    onOptionSelected = callback;
}

void DialogBox::setOnFinished(std::function<void()> callback) {
    onFinished = callback;
}

void DialogBox::wrapText() {
    wrappedTextLines.clear();
    
    if (fullText.empty()) {
        return;
    }
    
    std::istringstream stream(fullText);
    std::string word;
    std::string currentLine = "";
    
    float maxWidth = getTextAreaWidth();
    
    while (stream >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        int lineWidth = MeasureText(testLine.c_str(), fontSize);
        
        if (lineWidth > (int)maxWidth && !currentLine.empty()) {
            wrappedTextLines.push_back(currentLine);
            currentLine = word;
        } else {
            currentLine = testLine;
        }
    }
    
    if (!currentLine.empty()) {
        wrappedTextLines.push_back(currentLine);
    }
}

void DialogBox::updateFade(float deltaTime) {
    if (visible) {
        fadeTimer -= deltaTime;
        if (fadeTimer < 0.0f) fadeTimer = 0.0f;
        fadeAlpha = 1.0f - (fadeTimer / fadeSpeed);
        if (fadeAlpha > 1.0f) fadeAlpha = 1.0f;
    } else {
        fadeTimer += deltaTime;
        if (fadeTimer > fadeSpeed) fadeTimer = fadeSpeed;
        fadeAlpha = 1.0f - (fadeTimer / fadeSpeed);
        if (fadeAlpha < 0.0f) fadeAlpha = 0.0f;
    }
}

void DialogBox::updateCharReveal(float deltaTime) {
    if (isFinished()) {
        if (onFinished) {
            onFinished();
        }
        return;
    }
    
    charAccumulator += charRevealSpeed * deltaTime;
    if (charAccumulator >= 1.0f) {
        int charsToAdd = (int)charAccumulator;
        currentCharIndex += charsToAdd;
        charAccumulator -= charsToAdd;
    }
    if (currentCharIndex > (int)fullText.length()) {
        currentCharIndex = fullText.length();
    }
}

float DialogBox::getTextLineHeight() const {
    return fontSize + 6.0f;
}

float DialogBox::getTextAreaX() const {
    return DIALOG_PORTRAIT_PADDING + DIALOG_PORTRAIT_SIZE + DIALOG_PORTRAIT_PADDING;
}

float DialogBox::getTextAreaWidth() const {
    return width - getTextAreaX() - padding;
}

void DialogBox::drawDebugBounds() const {
    Vector2 drawPos = position;
    if (anchorPoint == "center") {
        drawPos.x -= width / 2.0f;
    }
    DrawRectangleLines((int)drawPos.x, (int)drawPos.y, (int)width, (int)height, RED);
}
