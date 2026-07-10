#include "Scene.hpp"
#include "rlgl.h"

Scene::Scene(float w, float h, Color bgColor)
    : paused(false), backgroundColor(bgColor), width(w), height(h), sceneMgr_(nullptr) {
    // Create default camera
    camera = new SceneCamera(w / 2.0f, h / 2.0f, 1.0f);
    inputHandler.setCamera(camera);
}

Scene::~Scene() {
    cleanup();
    if (camera) delete camera;
}

void Scene::addActor(SceneActor* actor) {
    if (!actor) return;
    actors.push_back(actor);
    sortActorsByLayer();
}

void Scene::removeActor(SceneActor* actor) {
    if (!actor) return;
    actorsToRemove.push_back(actor);
}

void Scene::removeActorByTag(const std::string& tag) {
    auto it = std::find_if(actors.begin(), actors.end(),
        [&tag](SceneActor* actor) { return actor->getTag() == tag; });
    if (it != actors.end()) {
        removeActor(*it);
    }
}

SceneActor* Scene::findActorByTag(const std::string& tag) {
    auto it = std::find_if(actors.begin(), actors.end(),
        [&tag](SceneActor* actor) { return actor->getTag() == tag; });
    return (it != actors.end()) ? *it : nullptr;
}

std::vector<SceneActor*> Scene::findActorsByLayer(int layer) {
    std::vector<SceneActor*> result;
    for (auto actor : actors) {
        if (actor->getLayer() == layer) {
            result.push_back(actor);
        }
    }
    return result;
}

std::vector<SceneActor*> Scene::getAllActors() const {
    return actors;
}

int Scene::getActorCount() const {
    return actors.size();
}

SceneCamera* Scene::getCamera() {
    return camera;
}

SceneCamera* Scene::createCamera(float x, float y, float zoom) {
    if (camera) delete camera;
    camera = new SceneCamera(x, y, zoom);
    inputHandler.setCamera(camera);
    return camera;
}

void Scene::init() {
    // Override in subclasses
}

void Scene::update(float deltaTime) {
    // Always update input handler, even when paused (needed for pause menu and controls overlay)
    inputHandler.update();

    if (paused) return;

    // Wide view toggle (Numpad 0): zooms out to see the whole scene's
    // boundary rect at once, for eyeballing a 3D effect's placement across
    // the full scene instead of just the scene's normal gameplay framing.
    // Handled once here so every scene gets it for free instead of each
    // scene wiring its own zoomed-out preview mode. Gated to scenes entered
    // via the scene_select debug hub -- entrySceneName_ is stamped by
    // SceneManager on every switch -- so it's inert during the real
    // sequencer/merge flow.
    if (camera && entrySceneName_ == "scene_select" && IsKeyPressed(KEY_KP_0)) {
        camera->toggleWideView();
    }

    // Update camera
    if (camera) {
        camera->update(deltaTime);
    }

    // Update effects
    for (auto effect : effects) effect->update(deltaTime);

    // Mouse state for clickable actors, computed once per frame in world
    // space so it accounts for camera position/zoom.
    Vector2 mouseWorldPos = inputHandler.getMouseWorldPosition();
    bool mousePressedEdge = inputHandler.isMouseButtonPressed(MOUSE_BUTTON_LEFT);
    bool mouseReleasedEdge = inputHandler.isMouseButtonReleased(MOUSE_BUTTON_LEFT);

    // Update all active actors
    for (auto actor : actors) {
        if (actor->isActive()) {
            if (actor->isClickable()) {
                bool hoveredNow = CheckCollisionPointRec(mouseWorldPos, actor->getBounds());
                actor->updateClickState(hoveredNow, mousePressedEdge, mouseReleasedEdge);
            }
            actor->update(deltaTime);
        }
    }

    // Process removals
    processRemovals();
}

void Scene::draw() {
    ClearBackground(backgroundColor);

    // Background effects (before 2D actors - depth test disabled after each)
    for (auto effect : effects) {
        effect->drawBackground();
        rlDisableDepthTest();
    }

    // 2D actors
    if (camera) BeginMode2D(camera->getRaylibCamera());
    for (auto actor : actors) {
        if (actor->isVisible()) actor->draw();
    }
    if (camera) EndMode2D();

    // Foreground effects (after 2D actors)
    for (auto effect : effects) {
        effect->drawForeground();
        rlDisableDepthTest();
    }
}

void Scene::cleanup() {
    for (auto effect : effects) { effect->cleanup(); delete effect; }
    effects.clear();
    for (auto actor : actors) { delete actor; }
    actors.clear();
}

void Scene::addEffect(SceneEffect* effect) {
    effect->init();
    effects.push_back(effect);
}

void Scene::setPaused(bool p) {
    paused = p;
    if (paused) {
        inputHandler.clearAllInputs();
    }
}

bool Scene::isPaused() const {
    return paused;
}

void Scene::setBackgroundColor(Color color) {
    backgroundColor = color;
}

Color Scene::getBackgroundColor() const {
    return backgroundColor;
}

Vector2 Scene::getSize() const {
    return {width, height};
}

float Scene::getWidth() const {
    return width;
}

float Scene::getHeight() const {
    return height;
}

SceneActor* Scene::findActorAt(Vector2 worldPos) {
    for (auto actor : actors) {
        if (CheckCollisionPointRec(worldPos, actor->getBounds())) {
            return actor;
        }
    }
    return nullptr;
}

std::vector<SceneActor*> Scene::findActorsInRect(Rectangle rect) {
    std::vector<SceneActor*> result;
    for (auto actor : actors) {
        if (CheckCollisionRecs(rect, actor->getBounds())) {
            result.push_back(actor);
        }
    }
    return result;
}

SceneInputHandler* Scene::getInputHandler() {
    return &inputHandler;
}

void Scene::setSceneManager(void* manager) {
    sceneMgr_ = manager;
}

void* Scene::getSceneManager() const {
    return sceneMgr_;
}

// Default implementation: do nothing. Override in story scenes.
void Scene::triggerStoryEvent(int scenarioIndex) {
    (void)scenarioIndex;
}

void Scene::processRemovals() {
    for (auto actor : actorsToRemove) {
        auto it = std::find(actors.begin(), actors.end(), actor);
        if (it != actors.end()) {
            actors.erase(it);
        }
        delete actor;
    }
    actorsToRemove.clear();
}

void Scene::sortActorsByLayer() {
    std::sort(actors.begin(), actors.end(),
        [](SceneActor* a, SceneActor* b) {
            return a->getLayer() < b->getLayer();
        });
}

void Scene::triggerActorMoves(const std::vector<ActorMove>& moves, SceneActor* const* actorsByIndex, int actorCount) {
    for (const ActorMove& move : moves) {
        if (move.actorIndex < 0 || move.actorIndex >= actorCount) continue;
        SceneActor* actor = actorsByIndex[move.actorIndex];
        if (actor) actor->moveTo(move.waypoints, move.speed);
    }
}

// Default implementation of togglePause - just sets paused state
void Scene::togglePause() {
    paused = !paused;
    if (paused) {
        inputHandler.clearAllInputs();
    }
}
