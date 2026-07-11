#ifndef GOTCHI_DIALOG_BOX_HPP
#define GOTCHI_DIALOG_BOX_HPP

#include "raylib.h"
#include <string>
#include <vector>
#include <functional>

// GotchiDialogBox -- a separate, lighter-weight dialog box for the
// gotchi/hexboard tutorial. Deliberately NOT a DialogBox subclass or reuse:
// it never has a speaker roster, portraits, or dialog options -- it's always
// "the gotchi talking" (or a bare narrator line), so the whole
// CharacterRegistry/portrait-cache machinery in DialogBox would be dead
// weight here. Visually it's a rounded speech-bubble anchored above the
// gotchi rather than DialogBox's letterboxed cinematic bar, so the tutorial
// reads as "the pet is talking to you" rather than a cutscene.
//
// Advance model: same char-reveal-then-wait-for-input shape as DialogBox,
// but only ever manual -- no auto-continue -- since tutorial pacing should
// never race ahead of the player. isFinished() + consumeAdvance() (checked
// against KEY_SPACE by the caller) is the whole API a tutorial driver needs.
class GotchiDialogBox {
public:
    GotchiDialogBox(Vector2 position, float width, float height);

    void update(float deltaTime);
    void draw();

    // Sets the line and (re)starts the character reveal from scratch. Also
    // re-wraps the line to the bubble's width immediately, so the char-reveal
    // draw indexes into wrapped (not raw) text and never overruns the bubble.
    void setText(const std::string& text);

    void show();
    void hide();
    bool isVisible() const;

    // True once the full line has been revealed (ready to advance).
    bool isFinished() const;

    // Pressing space/accept when finished should skip the reveal instead of
    // advancing, same convention DialogBox uses for its skip key.
    void skipReveal();

    void setPosition(Vector2 pos);

private:
    Vector2 position_;
    float width_, height_;

    std::string fullText_;
    std::vector<std::string> wrappedLines_;
    int totalCharCount_ = 0;      // sum of wrappedLines_ lengths (reveal counts against this)
    int currentCharIndex_ = 0;
    float charAccumulator_ = 0.0f;
    float charRevealSpeed_ = 45.0f;

    bool visible_ = false;
    float fadeAlpha_ = 0.0f;
    float fadeTimer_ = 0.0f;
    float fadeSpeed_ = 0.25f;

    void updateFade(float deltaTime);
    void updateCharReveal(float deltaTime);
    void wrapText();  // fills wrappedLines_/totalCharCount_ from fullText_
};

#endif // GOTCHI_DIALOG_BOX_HPP
