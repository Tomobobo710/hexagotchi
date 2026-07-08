#ifndef SCENE_PREVIEW_SCENE_HPP
#define SCENE_PREVIEW_SCENE_HPP

#include "Scene.hpp"
#include "SceneEffect.hpp"
#include <string>
#include <vector>
#include <functional>

// Generic "look at the background/effects, nothing else" viewer for Tom's
// world-scenes. No actors, no dialog, no event logic -- just the scene's
// background art plus any SceneEffect it registers (e.g. SchoolSkyEffect's
// jet/clouds), camera zoomed out so the whole 1280x720 world fits inside the
// fixed 720x720 game frame at once. Cycle between registered scenes with
// LEFT/RIGHT so new backgrounds/effects can be checked here as they're built,
// without wading through actors, triggers, or dialog to see them.
class ScenePreviewScene : public Scene {
public:
    struct Entry {
        std::string label;          // shown on screen, e.g. "SCHOOL"
        std::string backgroundKey;  // AssetPack key, empty if this scene has none yet
        Color clearColor;           // shown behind any transparent background pixels
        std::function<SceneEffect*()> makeEffect;  // nullptr if this scene has no effect
    };

    ScenePreviewScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    std::vector<Entry> entries;
    int currentIndex = 0;

    Texture2D background = {0};
    SceneEffect* effect = nullptr;

    void loadCurrent();
    void unloadCurrent();
};

#endif // SCENE_PREVIEW_SCENE_HPP
