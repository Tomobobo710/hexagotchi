#include "SceneManager.hpp"
#include "GameConstants.hpp"

SceneManager::SceneManager()
    : currentScene(nullptr), nextScene(nullptr), currentSceneName(""), nextSceneName(""),
      transitioning(false), transitionTimer(0.0f), transitionDuration(SCENE_TRANSITION_DURATION),
      currentTransitionEffect(TransitionEffect::FADE) {
}

SceneManager::~SceneManager() { cleanup(); }

void SceneManager::registerScene(const std::string& name, Scene* scene) {
    if (!scene) return;
    scenes[name] = scene;
}

void SceneManager::unregisterScene(const std::string& name) {
    auto it = scenes.find(name);
    if (it != scenes.end()) {
        it->second->cleanup();
        delete it->second;
        scenes.erase(it);
    }
}

Scene* SceneManager::getScene(const std::string& name) {
    auto it = scenes.find(name);
    return (it != scenes.end()) ? it->second : nullptr;
}

Scene* SceneManager::getCurrentScene() { return currentScene; }
std::string SceneManager::getCurrentSceneName() const { return currentSceneName; }

void SceneManager::switchScene(const std::string& sceneName, TransitionEffect effect, float duration) {
    Scene* scene = getScene(sceneName);
    if (!scene || sceneName == currentSceneName) return;
    nextScene = scene;
    nextSceneName = sceneName;
    currentTransitionEffect = effect;
    transitionDuration = duration;
    transitionTimer = 0.0f;
    transitioning = true;
}

void SceneManager::switchSceneImmediate(const std::string& sceneName) {
    Scene* scene = getScene(sceneName);
    if (!scene || sceneName == currentSceneName) return;
    if (currentScene) currentScene->cleanup();
    currentScene = scene;
    currentSceneName = sceneName;
    currentScene->setSceneManager(this);
    currentScene->init();
    // Zero-length update lets the scene's own ambient camera re-pinning
    // (position/zoom set in update(), not init()) take effect before the
    // first draw -- otherwise the first frame renders at the camera's raw
    // constructor default (scene center, zoom 1.0) instead of its real
    // resting framing.
    currentScene->update(0.0f);
    transitioning = false;
    transitionTimer = 0.0f;
}

void SceneManager::update(float deltaTime) {
    if (transitioning) updateTransition(deltaTime);
    else if (currentScene) currentScene->update(deltaTime);
}

void SceneManager::draw() {
    if (transitioning) {
        if (currentScene) currentScene->draw();
        drawTransition();
    } else if (currentScene) {
        currentScene->draw();
    }
}

bool SceneManager::isTransitioning() const { return transitioning; }
float SceneManager::getTransitionProgress() const {
    if (transitionDuration == 0.0f) return 1.0f;
    return transitionTimer / transitionDuration;
}

void SceneManager::cleanup() {
    for (auto& pair : scenes) { pair.second->cleanup(); delete pair.second; }
    scenes.clear();
    currentScene = nullptr;
    nextScene = nullptr;
}

void SceneManager::updateTransition(float deltaTime) {
    transitionTimer += deltaTime;
    float halfDuration = transitionDuration * 0.5f;
    if (transitionTimer >= halfDuration && currentScene != nextScene) {
        if (currentScene) currentScene->cleanup();
        currentScene = nextScene;
        currentSceneName = nextSceneName;
        currentScene->setSceneManager(this);
        currentScene->init();
        // Same reasoning as switchSceneImmediate() -- get the camera pinned
        // to its real resting spot before the transition's second half
        // starts fading the new scene in.
        currentScene->update(0.0f);
    }
    if (transitionTimer >= transitionDuration) {
        nextScene = nullptr;
        nextSceneName = "";
        transitioning = false;
        transitionTimer = 0.0f;
    }
}

void SceneManager::drawTransition() {
    float progress = getTransitionProgress();
    switch (currentTransitionEffect) {
        case TransitionEffect::FADE: {
            float alpha = (progress < 0.5f) ? progress * 2.0f : (1.0f - progress) * 2.0f;
            DrawRectangle(0, 0, GAME_W, GAME_H, {0, 0, 0, (unsigned char)(alpha * 255)});
            break;
        }
        case TransitionEffect::SLIDE_LEFT: {
            int d = (int)(GAME_W * progress);
            DrawRectangle(-GAME_W + d, 0, GAME_W, GAME_H, BLACK);
            break;
        }
        case TransitionEffect::SLIDE_RIGHT: {
            int d = (int)(GAME_W * progress);
            DrawRectangle(GAME_W - d, 0, GAME_W, GAME_H, BLACK);
            break;
        }
        case TransitionEffect::SLIDE_UP: {
            int d = (int)(GAME_H * progress);
            DrawRectangle(0, -GAME_H + d, GAME_W, GAME_H, BLACK);
            break;
        }
        case TransitionEffect::SLIDE_DOWN: {
            int d = (int)(GAME_H * progress);
            DrawRectangle(0, GAME_H - d, GAME_W, GAME_H, BLACK);
            break;
        }
        default: break;
    }
}
