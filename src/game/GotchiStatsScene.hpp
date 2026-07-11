#ifndef GOTCHI_STATS_SCENE_HPP
#define GOTCHI_STATS_SCENE_HPP

#include "Scene.hpp"
#include "Gotchi.hpp"
#include "Button.hpp"
#include "GameState.h"
#include "EventBus.h"
#include <string>
#include <vector>
#include <memory>

// Scene for displaying detailed gotchi stats
// Shows all status bars for the most relevant fields
class GotchiStatsScene : public Scene {
public:
    GotchiStatsScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    // Set the shared GameState reference (for vitals)
    void setGameState(GameState* state) { gameState_ = state; }

private:
    Gotchi* gotchi = nullptr;
    GameState* gameState_ = nullptr;  // Shared vitals from GameState
    GotchiStats defaultStats_;  // Fallback if gameState_ is not set (for tests)
    GotchiMood defaultMood_;    // Fallback mood if gameState_ is not set
    std::string gotchiDir;
    float simTime_ = 0.0f;  // Total simulation time
    int frameCount_ = 0;    // Frame counter for animation
    std::vector<std::unique_ptr<Button>> buttons;

    void addNavigationButton(const std::string& label, const std::string& targetScene, float x, float y);
};

#endif // GOTCHI_STATS_SCENE_HPP
