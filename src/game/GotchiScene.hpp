#ifndef GOTCHI_SCENE_HPP
#define GOTCHI_SCENE_HPP

#include "Scene.hpp"
#include "Gotchi.hpp"
#include "Button.hpp"
#include "EventType.h"
#include "EventBus.h"
#include <string>
#include <vector>
#include <memory>

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

private:
    Gotchi* gotchi = nullptr;
    std::string gotchiDir;
    float simTime_ = 0.0f;  // Total simulation time
    int frameCount_ = 0;    // Frame counter for animation
    std::vector<std::unique_ptr<Button>> buttons;
    std::string lastClickedButton_;  // Message to display when a button is clicked
    EventBus* eventBus_ = nullptr;   // Event bus for merge button events

    void addNavigationButton(const std::string& label, const std::string& targetScene, float x, float y);
    void addButton(const std::string& label, float x, float y, bool isMergeButton);
    void onMergeButtonClicked();
};

#endif // GOTCHI_SCENE_HPP
