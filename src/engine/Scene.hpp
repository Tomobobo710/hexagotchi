#ifndef SCENE_HPP
#define SCENE_HPP

#include "raylib.h"
#include "SceneActor.hpp"
#include "SceneCamera.hpp"
#include "SceneEffect.hpp"
#include "SceneInputHandler.hpp"
#include <vector>
#include <string>
#include <algorithm>

// Scene constants
const Color SCENE_DEFAULT_BG = RAYWHITE;
const float SCENE_DEFAULT_WIDTH = 1920.0f;
const float SCENE_DEFAULT_HEIGHT = 1080.0f;

class Scene {
public:
    // Constructor and destructor
    Scene(float width = SCENE_DEFAULT_WIDTH, float height = SCENE_DEFAULT_HEIGHT, Color bgColor = SCENE_DEFAULT_BG);
    virtual ~Scene();
    
    // Actor management
    void addActor(SceneActor* actor);
    void removeActor(SceneActor* actor);
    void removeActorByTag(const std::string& tag);
    
    SceneActor* findActorByTag(const std::string& tag);
    std::vector<SceneActor*> findActorsByLayer(int layer);
    std::vector<SceneActor*> getAllActors() const;
    
    int getActorCount() const;
    
    // Camera
    SceneCamera* getCamera();
    SceneCamera* createCamera(float x, float y, float zoom = CAMERA_DEFAULT_ZOOM);
    
    // Effects
    void addEffect(SceneEffect* effect);

    // Lifecycle - override these in subclasses
    virtual void init();
    virtual void update(float deltaTime);
    virtual void draw();
    virtual void cleanup();
    
    // State
    void setPaused(bool p);
    virtual bool isPaused() const;

    // Pause toggle - virtual so subclasses can override
    virtual void togglePause();
    
    // Properties
    void setBackgroundColor(Color color);
    Color getBackgroundColor() const;

    Vector2 getSize() const;
    float getWidth() const;
    float getHeight() const;
    
    // Utilities
    SceneActor* findActorAt(Vector2 worldPos);
    std::vector<SceneActor*> findActorsInRect(Rectangle rect);

    // Input
    SceneInputHandler* getInputHandler();

    // Scene manager access
    void setSceneManager(void* manager);
    void* getSceneManager() const;

    // Story event trigger — called by StorySequencer to play a beat's dialog
    // Override in subclasses to trigger their specific scripted event
    // Default implementation does nothing (for non-story scenes like GameScene, BossScene)
    virtual void triggerStoryEvent(int eventIndex);

protected:
    std::vector<SceneActor*> actors;
    std::vector<SceneActor*> actorsToRemove;
    std::vector<SceneEffect*> effects;
    SceneCamera* camera;
    SceneInputHandler inputHandler;
    bool paused;
    Color backgroundColor;
    float width, height;
    void* sceneMgr_;  // Pointer to SceneManager (opaque for header-only dependency)
    
    // Helper methods
    void processRemovals();
    void sortActorsByLayer();
};

#endif // SCENE_HPP
