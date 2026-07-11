#include "GotchiDialogBox.hpp"
#include <sstream>

GotchiDialogBox::GotchiDialogBox(Vector2 pos, float w, float h)
    : position_(pos), width_(w), height_(h) {
}

void GotchiDialogBox::setText(const std::string& text) {
    fullText_ = text;
    currentCharIndex_ = 0;
    charAccumulator_ = 0.0f;
    wrapText();
}

void GotchiDialogBox::show() {
    visible_ = true;
    fadeTimer_ = 0.0f;
    currentCharIndex_ = 0;
    charAccumulator_ = 0.0f;
}

void GotchiDialogBox::hide() {
    visible_ = false;
    fadeTimer_ = fadeSpeed_;
}

bool GotchiDialogBox::isVisible() const {
    return visible_ && fadeAlpha_ > 0.0f;
}

bool GotchiDialogBox::isFinished() const {
    return currentCharIndex_ >= totalCharCount_;
}

void GotchiDialogBox::skipReveal() {
    currentCharIndex_ = totalCharCount_;
}

void GotchiDialogBox::setPosition(Vector2 pos) {
    position_ = pos;
}

void GotchiDialogBox::updateFade(float deltaTime) {
    if (visible_) {
        fadeTimer_ -= deltaTime;
        if (fadeTimer_ < 0.0f) fadeTimer_ = 0.0f;
        fadeAlpha_ = 1.0f - (fadeTimer_ / fadeSpeed_);
        if (fadeAlpha_ > 1.0f) fadeAlpha_ = 1.0f;
    } else {
        fadeTimer_ += deltaTime;
        if (fadeTimer_ > fadeSpeed_) fadeTimer_ = fadeSpeed_;
        fadeAlpha_ = 1.0f - (fadeTimer_ / fadeSpeed_);
        if (fadeAlpha_ < 0.0f) fadeAlpha_ = 0.0f;
    }
}

void GotchiDialogBox::updateCharReveal(float deltaTime) {
    if (isFinished()) return;
    charAccumulator_ += charRevealSpeed_ * deltaTime;
    if (charAccumulator_ >= 1.0f) {
        int charsToAdd = (int)charAccumulator_;
        currentCharIndex_ += charsToAdd;
        charAccumulator_ -= charsToAdd;
    }
    if (currentCharIndex_ > totalCharCount_) {
        currentCharIndex_ = totalCharCount_;
    }
}

void GotchiDialogBox::update(float deltaTime) {
    updateFade(deltaTime);
    if (visible_) {
        updateCharReveal(deltaTime);
    }
}

void GotchiDialogBox::wrapText() {
    wrappedLines_.clear();
    totalCharCount_ = 0;

    if (fullText_.empty()) return;

    int fontSize = FONT_SIZE;
    int padding = PADDING;
    float maxWidth = width_ - padding * 2.0f;

    std::istringstream stream(fullText_);
    std::string word;
    std::string currentLine;

    while (stream >> word) {
        std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
        int lineWidth = MeasureText(testLine.c_str(), fontSize);

        if (lineWidth > (int)maxWidth && !currentLine.empty()) {
            wrappedLines_.push_back(currentLine);
            currentLine = word;
        } else {
            currentLine = testLine;
        }
    }
    if (!currentLine.empty()) {
        wrappedLines_.push_back(currentLine);
    }

    for (const auto& line : wrappedLines_) {
        totalCharCount_ += (int)line.length() + 1;  // +1 for the space/newline consumed between lines
    }
}

void GotchiDialogBox::draw() {
    if (fadeAlpha_ <= 0.01f) return;

    unsigned char alpha = (unsigned char)(255 * fadeAlpha_);

    // Speech-bubble body: rounded rect, anchored by top-left position_ here
    // (the caller picks position_ so the bubble sits just above the gotchi's
    // head), plus a small triangular tail pointing down at the gotchi.
    Color bubbleBg = { 255, 250, 235, (unsigned char)(alpha * 0.96f) };
    Color bubbleBorder = { 90, 70, 40, alpha };

    Rectangle bubble = { position_.x, position_.y, width_, height_ };
    DrawRectangleRounded(bubble, 0.25f, 8, bubbleBg);
    DrawRectangleRoundedLines(bubble, 0.25f, 8, bubbleBorder);

    // Tail: small triangle centered on the bubble's bottom edge.
    Vector2 tailCenter = { position_.x + width_ / 2.0f, position_.y + height_ };
    Vector2 tailLeft  = { tailCenter.x - 14.0f, tailCenter.y };
    Vector2 tailRight = { tailCenter.x + 14.0f, tailCenter.y };
    Vector2 tailPoint = { tailCenter.x, tailCenter.y + 18.0f };
    DrawTriangle(tailLeft, tailPoint, tailRight, bubbleBg);
    DrawLineV(tailLeft, tailPoint, bubbleBorder);
    DrawLineV(tailPoint, tailRight, bubbleBorder);

    // Text, revealed character-by-character, wrapped to the bubble's width so
    // it never draws outside the bubble bounds.
    Color textColor = { 40, 30, 20, alpha };
    int fontSize = FONT_SIZE;
    int padding = PADDING;
    float lineHeight = fontSize + 2.0f;

    int charCount = 0;
    for (size_t i = 0; i < wrappedLines_.size(); i++) {
        const std::string& line = wrappedLines_[i];
        std::string displayLine;
        for (char c : line) {
            if (charCount < currentCharIndex_) displayLine += c;
            charCount++;
        }
        charCount++;  // account for the separator counted in totalCharCount_
        if (!displayLine.empty()) {
            DrawText(displayLine.c_str(),
                     (int)(position_.x + padding),
                     (int)(position_.y + padding + i * lineHeight),
                     fontSize, textColor);
        }
    }

    // "Press SPACE" prompt, only once the line has fully revealed and
    // nothing else (e.g. a required tutorial action) is still blocking the
    // advance.
    if (isFinished() && !suppressPrompt_) {
        float blinkTime = (float)GetTime();
        if ((int)(blinkTime * 2) % 2 == 0) {
            const char* prompt = "TAP/Press SPACE to continue...";
            int promptSize = 14;
            Color promptColor = { 120, 100, 70, alpha };
            DrawText(prompt,
                      (int)(position_.x + width_ - padding - MeasureText(prompt, promptSize)),
                      (int)(position_.y + height_ - padding - promptSize + 10.0f),
                      promptSize, promptColor);
        }
    }
}

