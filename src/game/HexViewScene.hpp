#ifndef HEX_VIEW_SCENE_HPP
#define HEX_VIEW_SCENE_HPP

#include "Scene.hpp"
#include "HexWorld.hpp"
#include "Gotchi.hpp"
#include "GotchiStats.hpp"
#include "GotchiMood.hpp"
#include "Button.hpp"
#include "PauseMenuOverlay.hpp"
#include "TutorialController.hpp"
#include <memory>
#include <vector>
class Hotbar;
class EventBus;
class Item;
class GameState;

class HexViewScene : public Scene {
public:
    HexViewScene();
    ~HexViewScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;

    // Set the event bus for emitting CareAction events
    void setEventBus(EventBus* bus) { eventBus_ = bus; }

    // Set the tutorial controller reference for button locking + step draw/advance
    void setTutorialController(TutorialController* tc) { tutorialController_ = tc; }

    // Set the shared game state for vitals and mood
    void setGameState(GameState* state) { gameState_ = state; }

private:
    // Pause functionality
    void togglePause() override;
    bool isPaused() const override { return paused; }

    // Callback for pause menu
    void onExitSelected();  // Request exit

    // Back button logic
    void addBackButton();
    void onBackButtonClicked();

private:
    HexWorld* world;
    Gotchi* gotchi;
    std::unique_ptr<PauseMenuOverlay> pauseMenu;

    // Camera panning state
    bool cameraPanning;

    // World config (stored after init)
    float hexSize_;

    // Shared game state (owned by main.cpp)
    GameState* gameState_ = nullptr;

    // Fallback vitals and mood for Gotchi (used if no gameState available)
    GotchiStats defaultStats_;
    GotchiMood defaultMood_;

    // Visual probe for click target
    bool  hasClickMarker_ = false;
    HexCoords clickMarkerHex_{0, 0};
    Vector2   clickMarkerWorld_{0, 0};

    // Tool images (cached for performance)
    Texture2D toolFoodImage_ = {0};
    Texture2D toolBallImage_ = {0};
    Texture2D toolWaterImage_ = {0};

    // Hotbar UI (screen-space palette of items)
    std::unique_ptr<Hotbar> hotbar_;

    // Event bus for emitting CareAction events
    EventBus* eventBus_ = nullptr;

    // Back button
    std::unique_ptr<Button> backButton_;

    // Tutorial controller reference -- drives back-button/hotbar locking and
    // owns the GotchiDialogBox drawn/advanced while a tutorial step belongs
    // to this scene
    TutorialController* tutorialController_ = nullptr;

    // Vitals display
    void drawVitals() const;
};

#endif
