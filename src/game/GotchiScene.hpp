#ifndef GOTCHI_SCENE_HPP
#define GOTCHI_SCENE_HPP

#include "Scene.hpp"
#include "Gotchi.hpp"
#include "Button.hpp"
#include "EventType.h"
#include "EventBus.h"
#include "GameState.h"
#include "MergeController.hpp"
#include "TutorialController.hpp"
#include <string>
#include <vector>
#include <memory>
#include <map>

// Scene 8 - Gotchi display scene
// Shows the Gotchi character centered on screen, updating every frame
class GotchiScene : public Scene {
public:
    GotchiScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void addButtons();

    // Set the event bus for merge button events
    void setEventBus(EventBus* bus) { eventBus_ = bus; }

    // Set the shared GameState reference (for vitals)
    void setGameState(GameState* state) { gameState_ = state; }

    // Set the merge controller reference for button label updates
    void setMergeController(MergeController* mc) { mergeController_ = mc; }

    // Set the tutorial controller reference for button locking + step draw/advance
    void setTutorialController(TutorialController* tc) { tutorialController_ = tc; }

private:
    Gotchi* gotchi = nullptr;
    GameState* gameState_ = nullptr;  // Shared vitals from GameState
    GotchiStats defaultStats_;  // Fallback if gameState_ is not set (for tests)
    GotchiMood defaultMood_;    // Fallback mood if gameState_ is not set
    std::string gotchiDir;
    float simTime_ = 0.0f;  // Total simulation time
    int frameCount_ = 0;    // Frame counter for animation
    std::vector<std::unique_ptr<Button>> buttons;
    Button* mergeButton_ = nullptr;  // Non-owning; points into buttons. Set in addButton() so the
                                      // merge-label refresh in update() never has to guess an index.
    std::string lastClickedButton_;  // Message to display when a button is clicked
    EventBus* eventBus_ = nullptr;   // Event bus for merge button events and CareAction subscription
    int careActionToken_ = 0;        // Event subscription token for CareAction

    // Action accumulation for Box C drivers (warmth/hygiene)
    float affectionAccumulator_ = 0.0f;
    float hygieneAccumulator_ = 0.0f;

    // Merge controller reference for button label updates
    MergeController* mergeController_ = nullptr;

    // Tutorial controller reference -- drives button locking + owns the
    // GotchiDialogBox drawn/advanced while a tutorial step belongs to this scene
    TutorialController* tutorialController_ = nullptr;

    // Applies tutorialController_->isActionUnlocked() to every care/nav
    // button each frame, plus the always-on Merge lock (only the sleep
    // collapse gate unlocks it -- see applySleepCollapseGate()).
    void applyTutorialLocks();

    // Trips GameState::sleepCollapsed once sleep reaches 0 while out in the
    // world. This is the only way Merge ever unlocks.
    void applySleepCollapseGate();

    // One-shot guard so the "death" scene switch only fires once per entry
    // into this scene -- see the isDead() check in update().
    bool deathTriggered_ = false;

    // Counts down from DEATH_HOLD_SECONDS once isDead() first flips true;
    // the death animation plays and holds on screen for this long before
    // GotchiScene actually switches to DeathScene. Negative = not counting.
    float deathHoldTimer_ = -1.0f;
    static constexpr float DEATH_HOLD_SECONDS = 5.0f;

    // Button cooldown system
    std::map<std::string, float> buttonCooldowns_;
    float buttonFeedbackTimer_ = 0.0f;

    // Background texture for gotchi scene
    Texture2D background_ = {0};  // Gotchi background image

    // Biome-based backgrounds
    std::map<std::string, Texture2D> biomeBackgrounds_;
    std::string currentBiome_;
    bool needsBackgroundUpdate_ = true;

    // Action shader overlay
    Shader actionShader_ = {0};
    Texture2D whitePixel_ = {0};  // 1x1 white texture for DrawTexturePro UVs
    int locMode_ = -1, locProgress_ = -1, locTime_ = -1, locResolution_ = -1;
    float actionOverlayTimer_ = 0.0f;
    float actionOverlayDuration_ = 0.0f;
    int actionOverlayMode_ = -1;

    void addNavigationButton(const std::string& label, const std::string& targetScene, float x, float y);
    void addButton(const std::string& label, float x, float y, bool isMergeButton);

    // Handle button clicks - routes to specific action handlers
    void handleGotchiAction(const std::string& action);

    // Trigger the action shader overlay
    void triggerActionShader(int mode, float duration);

    // Get the gotchi's screen rectangle for shader overlay
    Rectangle getGotchiScreenRect();

    // Merge button callback - emits MergeRequested on the bus
    void onMergeButtonClicked();

    // Explore button callback -- switches to the hexboard scene
    void onExploreButtonClicked();

    // Background management
    void loadBiomeBackgrounds();
    void updateBackgroundForHex(int q, int r);

    // Back button on hexboard - handled in HexViewScene
    // This is just a placeholder to note the navigation relationship
};

#endif // GOTCHI_SCENE_HPP
