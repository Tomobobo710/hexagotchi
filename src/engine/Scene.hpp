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

// One actor's scripted walk for a single scenario line, authored via the
// scene editor's waypoint tool (see tools/scene_editor.cpp) and hand-copied
// into a scene's scenario table as `movesAtStart`/`movesAtEnd` on a line
// struct (e.g. PizzaLine, OfficeLine). `actorIndex` matches the same
// per-scene actor-index convention `focusActor` already uses (0 = first
// named actor, 1 = second, etc). playLine() fires movesAtStart right after
// setting up dialog/camera for a line; advanceLine() fires movesAtEnd for
// the line that's finishing, before moving to the next one. Movement runs
// in the background via SceneActor::moveTo() -- it does NOT block dialog
// advancement, so a walk that needs to visibly finish before the next line
// should be triggered a line early.
struct ActorMove {
    int actorIndex;
    std::vector<Vector2> waypoints;
    float speed;
};

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

    // Name of the scene we were switched in FROM, stamped by SceneManager
    // right before init() runs on every entry. Lets a scene gate debug-only
    // affordances (raw-key event triggers, camera-tuning keys, etc) to only
    // work when reached via the "scene_select" debug hub, not via the real
    // sequencer/merge flow.
    void setEntrySceneName(const std::string& name) { entrySceneName_ = name; }
    const std::string& getEntrySceneName() const { return entrySceneName_; }

    // Scenario trigger — called by StorySequencer to play a beat's dialog.
    // Override in subclasses to trigger their specific scripted scenario.
    // Default implementation does nothing (for non-story scenes like GameScene, BossScene)
    virtual void triggerStoryEvent(int scenarioIndex);

    // True while a triggerStoryEvent()'d scenario is still playing out (dialog
    // lines not yet exhausted). StorySequencer polls this to know when a
    // sequence step is actually done, rather than reacting to DialogBox's
    // per-line onFinished callback (which fires once per line typed out, not
    // once per scenario). Default false for non-story scenes.
    virtual bool isPlayingScenario() const { return false; }

    // Number of scripted scenarios this scene defines (scenarios.size() in
    // subclasses that have one). Lets the scene_select debug hub cycle
    // through and preview them generically (see main.cpp's Z key) without
    // needing to know each scene's concrete type. Default 0 for non-story
    // scenes.
    virtual int getScenarioCount() const { return 0; }

    // Plays scenario `index`'s dialog/camera/actor-move script from the top.
    // Same override subclasses already provide for the StorySequencer-driven
    // triggerStoryEvent() path, exposed here too so the scene_select debug
    // hub can trigger scenarios without a per-scene-type cast. Default no-op.
    virtual void triggerScenario(int index) {}

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
    std::string entrySceneName_;  // See getEntrySceneName() above

    // Helper methods
    void processRemovals();
    void sortActorsByLayer();

    // Fires each ActorMove's moveTo() against actorsByIndex[move.actorIndex]
    // (bounds-checked; out-of-range or null entries are silently skipped).
    // Scenes call this from playLine()/advanceLine() with their own local
    // actor-index array (the same one focusCameraOn() already uses) to
    // dispatch a line's movesAtStart/movesAtEnd.
    void triggerActorMoves(const std::vector<ActorMove>& moves, SceneActor* const* actorsByIndex, int actorCount);
};

#endif // SCENE_HPP
