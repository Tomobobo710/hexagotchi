#ifndef SCENE_MANAGER_HPP
#define SCENE_MANAGER_HPP

#include "Scene.hpp"
#include <map>
#include <string>
#include <functional>

// Scene transition effects
enum class TransitionEffect {
    NONE,
    FADE,
    SLIDE_LEFT,
    SLIDE_RIGHT,
    SLIDE_UP,
    SLIDE_DOWN
};

const float SCENE_TRANSITION_DURATION = 0.5f;

class SceneManager {
public:
    SceneManager();
    ~SceneManager();
    
    // Scene registration and management
    void registerScene(const std::string& name, Scene* scene);
    void unregisterScene(const std::string& name);
    Scene* getScene(const std::string& name);
    Scene* getCurrentScene();
    std::string getCurrentSceneName() const;
    
    // Scene switching
    virtual void switchScene(const std::string& sceneName, TransitionEffect effect = TransitionEffect::FADE, float duration = SCENE_TRANSITION_DURATION);
    void switchSceneImmediate(const std::string& sceneName);
    
    // Lifecycle
    void update(float deltaTime);
    void draw();
    
    // State
    bool isTransitioning() const;
    float getTransitionProgress() const;
    
    // Cleanup
    void cleanup();
    
protected:
    std::map<std::string, Scene*> scenes;
    Scene* currentScene;
    Scene* nextScene;
    std::string currentSceneName;
    std::string nextSceneName;
    
    // Transition state
    bool transitioning;
    float transitionTimer;
    float transitionDuration;
    TransitionEffect currentTransitionEffect;
    
    // Helper
    void updateTransition(float deltaTime);
    void drawTransition();
};

#endif // SCENE_MANAGER_HPP
