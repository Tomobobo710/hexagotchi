#ifndef DIALOG_BOX_HPP
#define DIALOG_BOX_HPP

#include "raylib.h"
#include "CharacterRegistry.hpp"
#include <string>
#include <vector>
#include <functional>
#include <map>

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
// Baseline: the longest scripted line in the game so far (Karen's
// "Ronzer got a promotion..." line, 87 characters) should take 10 seconds --
// so duration scales with line length at 8.7 characters/second, and shorter
// lines get proportionally shorter auto-continue bars instead of a flat
// timer that reads as much too slow for a one-word line.
const float DIALOG_AUTO_CONTINUE_CHARS_PER_SECOND = 8.7f;
const float DIALOG_AUTO_CONTINUE_MIN_DURATION = 2.0f;
const float DIALOG_AUTO_CONTINUE_BAR_HEIGHT = 5.0f;

class DialogBox {
public:
    DialogBox(Vector2 position, float width, float height);
    ~DialogBox();

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

    // One-call alternative to hand-picking name/color/gradient/portrait per
    // line: pulls the character's display name, name color, and portrait
    // gradient straight from CharacterRegistry, and loads (and internally
    // caches) that character's portrait texture for the given emotion --
    // callers no longer need their own per-scene portrait Texture2D arrays
    // or per-line setSpeakerName()/setSpeakerColor()/setPortraitGradient()/
    // setPortraitTexture() calls. overrideName, if non-empty, replaces the
    // registry's display name just for this call (e.g. "Larry (Tom's boss)"
    // for a first-time introduction line) without touching the registry
    // entry itself. Clears the portrait entirely (silhouette placeholder)
    // if the character has no art for that emotion or any fallback.
    void setCharacter(CharacterId id, PortraitEmotion emotion, const std::string& overrideName = "");

    // Clears the dialog box's speaker identity back to the base "no one
    // set" state -- same as calling clearPortraitTexture() plus resetting
    // the name/color, for lines with no on-screen speaker (e.g. Narrator
    // lines that don't go through setCharacter()).
    void clearCharacter();
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

    // Skip the current dialog line and finish immediately
    void skip();

    // Auto-continue: a progress bar along the bottom edge (in the current
    // speaker's color) fills from the moment a line is shown. By default its
    // duration scales with the line's length (see
    // DIALOG_AUTO_CONTINUE_CHARS_PER_SECOND) so short lines don't linger and
    // long lines get enough time to read; setAutoContinueDuration()
    // overrides that with a fixed duration instead, if a caller ever needs
    // one. consumeAutoAdvance() returns true exactly once, the frame the bar
    // completes (and only once the text has fully revealed) -- callers
    // check it alongside their existing accept-key poll so a line advances
    // on input or on timeout, whichever comes first. Disabled by default
    // (opt-in per dialog box via setAutoContinueEnabled(true)) so scenes
    // that want manual-only pacing are unaffected.
    void setAutoContinueEnabled(bool enabled);
    void setAutoContinueDuration(float seconds);
    bool consumeAutoAdvance();
    float getAutoContinueProgress() const;
    
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

    // Check if dialog should advance (finished and ready for next line)
    bool shouldAdvance() const;

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

    bool autoContinueEnabled;
    // What the current scene/line actually asked for via
    // setAutoContinueEnabled(), independent of the live global Auto Dialog
    // setting. autoContinueEnabled (the effective, actually-used flag) is
    // recomputed from this every update() against the CURRENT GetDialogSpeed()
    // -- so toggling the setting in OPTIONS (even mid-line, e.g. paused in a
    // Tom-world scene) takes effect immediately instead of only on the next
    // setAutoContinueEnabled() call (previously only made at scenario/line
    // start), which was the "flip Auto Dialog but it doesn't apply until you
    // leave/re-enter" bug.
    bool requestedAutoContinue_;
    float autoContinueDuration;
    bool autoContinueDurationOverridden;
    float autoContinueTimer;
    bool autoAdvancePending;
    
    std::function<void(int)> onOptionSelected;
    std::function<void()> onFinished;

    // Portrait textures loaded via setCharacter(), cached and owned here --
    // load-once-per-(character,emotion), unloaded in the destructor. Keyed
    // on the packed (CharacterId, PortraitEmotion) pair rather than a
    // struct/pair key to avoid pulling in <utility> just for std::pair's
    // comparison operators.
    std::map<int, Texture2D> portraitCache;
    Texture2D getCachedPortrait(CharacterId id, PortraitEmotion emotion);

    void wrapText();
    void updateFade(float deltaTime);
    void updateCharReveal(float deltaTime);
    float getTextLineHeight() const;
    float getTextAreaX() const;
    float getTextAreaWidth() const;
};

#endif // DIALOG_BOX_HPP
