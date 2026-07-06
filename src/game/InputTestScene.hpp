#ifndef INPUT_TEST_SCENE_HPP
#define INPUT_TEST_SCENE_HPP

#include "Scene.hpp"

class InputTestScene : public Scene {
public:
    InputTestScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;

private:
    // Rebinding state
    bool rebinding = false;
    std::string rebindingAction;  // Which action we're currently rebinding
    int newKey;                   // The captured key (set by input capture)
    bool bindingsChanged;         // Track if save is needed
};

#endif
