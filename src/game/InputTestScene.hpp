#ifndef INPUT_TEST_SCENE_HPP
#define INPUT_TEST_SCENE_HPP

#include "Scene.hpp"
#include "Button.hpp"
#include "SceneActor.hpp"

class InputTestScene : public Scene {
public:
    InputTestScene();

    void init() override;
    void draw() override;
    void update(float deltaTime) override;
    void cleanup() override;

private:
    // Rebinding state
    bool rebinding = false;
    std::string rebindingAction;  // Which action we're currently rebinding
    int newKey;                   // The captured key (set by input capture)
    bool bindingsChanged;         // Track if save is needed

    // Button demo
    Button testButton;
    int clickCount = 0;

    // Sprite button demo
    Button spriteButton;
    Texture2D spriteButtonTexture;
    Texture2D spriteButtonHoverTexture;
    int spriteClickCount = 0;

    // Animated SceneActor demo
    SceneActor* animatedActor = nullptr;
    Texture2D animStripTexture;
};

#endif
