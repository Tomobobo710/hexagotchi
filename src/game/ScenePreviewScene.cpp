#include "ScenePreviewScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SchoolSkyEffect.hpp"

ScenePreviewScene::ScenePreviewScene()
    : Scene(1280.0f, 720.0f, Color{20, 20, 24, 255}) {
}

void ScenePreviewScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);
    // Fit the whole 1280-wide world into the fixed 720-wide game frame:
    // zoom = frameWidth / worldWidth. Height (720/720 = 1.0) is already <=
    // this, so the narrower dimension (width) is what limits the fit.
    getCamera()->setZoom((float)GAME_W / 1280.0f);
    getCamera()->setPosition(1280.0f / 2.0f, 720.0f / 2.0f);

    entries.clear();
    entries.push_back({"PIZZA PARLOR", "backgrounds/parlorbg.png", Color{30, 20, 20, 255}, nullptr});
    entries.push_back({"APARTMENT", "backgrounds/apartmentbg.png", Color{22, 18, 26, 255}, nullptr});
    entries.push_back({"THERAPIST'S OFFICE", "backgrounds/therapistbg.png", Color{26, 22, 20, 255}, nullptr});
    entries.push_back({"DATATEK SOLUTIONS (OFFICE)", "backgrounds/officebg.png", Color{20, 22, 28, 255}, nullptr});
    entries.push_back({"SCHOOL PICKUP", "backgrounds/schoolbg.png", Color{202, 232, 250, 255},
        []() -> SceneEffect* { return new SchoolSkyEffect(); }});

    currentIndex = 0;
    loadCurrent();
}

void ScenePreviewScene::loadCurrent() {
    unloadCurrent();
    const Entry& e = entries[currentIndex];
    setBackgroundColor(e.clearColor);
    if (!e.backgroundKey.empty()) {
        background = AssetPack::loadTexture(e.backgroundKey);
    }
    if (e.makeEffect) {
        effect = e.makeEffect();
        addEffect(effect);
    }
}

void ScenePreviewScene::unloadCurrent() {
    if (background.id != 0) {
        UnloadTexture(background);
        background = {0};
    }
    // `effects` is Scene's own protected list (normally torn down wholesale
    // by Scene::cleanup()) -- here we're swapping the single active effect
    // out mid-session when the user cycles scenes, so clean it up and drop
    // it from that list directly instead of tearing the whole scene down.
    if (effect) {
        effect->cleanup();
        delete effect;
        effect = nullptr;
        effects.clear();
    }
}

void ScenePreviewScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) {
        currentIndex = (currentIndex + 1) % (int)entries.size();
        loadCurrent();
    }
    if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) {
        currentIndex = (currentIndex - 1 + (int)entries.size()) % (int)entries.size();
        loadCurrent();
    }
}

void ScenePreviewScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) {
        DrawTexture(background, 0, 0, WHITE);
    }
    EndMode2D();

    const Entry& e = entries[currentIndex];
    DrawText(e.label.c_str(), 16, 16, 24, RAYWHITE);
    DrawText("A/D or arrows: switch scene   7: Scene Select", 16, GAME_H - 26, 16, Color{200, 200, 210, 255});
}

void ScenePreviewScene::cleanup() {
    unloadCurrent();
    Scene::cleanup();
}
