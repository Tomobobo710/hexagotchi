#ifndef GOTCHI_SCENE_HPP
#define GOTCHI_SCENE_HPP

#include "Scene.hpp"
#include "Gotchi.hpp"
#include <string>

// Scene 8 - Gotchi display scene
// Shows the Gotchi character centered on screen, updating every frame
class GotchiScene : public Scene {
public:
    GotchiScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    Gotchi* gotchi = nullptr;
    std::string gotchiDir;
    float simTime_ = 0.0f;  // Total simulation time
    int frameCount_ = 0;    // Frame counter for animation
};

#endif // GOTCHI_SCENE_HPP
