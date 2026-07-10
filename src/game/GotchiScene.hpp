#ifndef GOTCHI_SCENE_HPP
#define GOTCHI_SCENE_HPP

#include "Scene.hpp"
#include "Gotchi.hpp"
#include "Button.hpp"
#include "EventType.h"
#include "EventBus.h"
#include "GameState.h"
#include "MergeController.hpp"
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

private:
    Gotchi* gotchi = nullptr;
    GameState* gameState_ = nullptr;  // Shared vitals from GameState
    GotchiStats defaultStats_;  // Fallback if gameState_ is not set (for tests)
    GotchiMood defaultMood_;    // Fallback mood if gameState_ is not set
    std::string gotchiDir;
    float simTime_ = 0.0f;  // Total simulation time
    int frameCount_ = 0;    // Frame counter for animation
    std::vector<std::unique_ptr<Button>> buttons;
    std::string lastClickedButton_;  // Message to display when a button is clicked
    EventBus* eventBus_ = nullptr;   // Event bus for merge button events and CareAction subscription
    int careActionToken_ = 0;        // Event subscription token for CareAction

    // Action accumulation for Box C drivers (warmth/hygiene)
    float affectionAccumulator_ = 0.0f;
    float hygieneAccumulator_ = 0.0f;

    // Merge controller reference for button label updates
    MergeController* mergeController_ = nullptr;

    // Button cooldown system
    std::map<std::string, float> buttonCooldowns_;
    float buttonFeedbackTimer_ = 0.0f;

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

    // Explore button logic
    void addExploreButton();
    void updateExploreButtonVisibility();
    void onExploreButtonClicked();

    // Back button on hexboard - handled in HexViewScene
    // This is just a placeholder to note the navigation relationship
};

#endif // GOTCHI_SCENE_HPP
