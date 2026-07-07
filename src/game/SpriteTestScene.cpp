#include "SpriteTestScene.hpp"
#include "SpriteLoader.hpp"
#include "../effects/StarfieldEffect.hpp"

static const char* GOTCHI_DIR = "assets/gotchis/001";
static const char* GOTCHI_ACTIONS[] = {"idle", "walk", "attack", "bounce", "blink", "hurt", "die"};
static const float GOTCHI_FRAME_DURATION = 0.12f;
static const float GOTCHI_DISPLAY_SCALE = 4.0f;  // Source frames are 64x64; scale up for visibility

SpriteTestScene::SpriteTestScene()
    : Scene(720.0f, 720.0f, {18, 18, 30, 255}) {
}

void SpriteTestScene::init() {
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);
    addEffect(new StarfieldEffect(getCamera()));

    for (const char* action : GOTCHI_ACTIONS) {
        std::vector<Texture2D> frames = SpriteLoader::loadFrames(GOTCHI_DIR, action);
        if (!frames.empty()) {
            actionNames.push_back(action);
            animFrames[action] = frames;
        }
    }

    gotchi = new SceneActor({360.0f, 300.0f}, 64.0f, 64.0f);
    gotchi->setScale({GOTCHI_DISPLAY_SCALE, GOTCHI_DISPLAY_SCALE});
    gotchi->setClickable(true);
    gotchi->setOnClick([this]() {
        clickCount++;
        selectAction(selectedAction + 1);
    });
    addActor(gotchi);

    if (!actionNames.empty()) selectAction(0);
}

void SpriteTestScene::selectAction(int index) {
    if (actionNames.empty()) return;
    selectedAction = ((index % (int)actionNames.size()) + (int)actionNames.size()) % (int)actionNames.size();
    const std::string& name = actionNames[selectedAction];
    bool loop = (name != "die" && name != "hurt" && name != "attack");
    gotchi->setAnimationFrames(animFrames[name], GOTCHI_FRAME_DURATION, loop);
}

void SpriteTestScene::cleanup() {
    Scene::cleanup();
    for (auto& pair : animFrames) {
        SpriteLoader::unloadFrames(pair.second);
    }
    animFrames.clear();
}

void SpriteTestScene::update(float deltaTime) {
    Scene::update(deltaTime);

    auto ih = getInputHandler();
    if (ih && ih->isActionPressed(INPUT_ACTION_MOVE_RIGHT)) selectAction(selectedAction + 1);
    if (ih && ih->isActionPressed(INPUT_ACTION_MOVE_LEFT)) selectAction(selectedAction - 1);
}

void SpriteTestScene::draw() {
    Scene::draw();

    DrawText("SPRITE TEST", 16, 8, 18, {180, 180, 255, 255});
    DrawText("A/D: Cycle animation   Click gotchi to cycle too", 16, 690, 12, {180, 180, 220, 255});

    char clickBuf[32];
    snprintf(clickBuf, sizeof(clickBuf), "Clicks: %d", clickCount);
    DrawText(clickBuf, 600, 40, 14, {180, 180, 220, 255});

    if (actionNames.empty()) {
        DrawText("No gotchi frames found under assets/gotchis/001", 16, 40, 14, {255, 120, 120, 255});
        return;
    }

    const std::string& name = actionNames[selectedAction];
    DrawText(("Action: " + name).c_str(), 16, 40, 16, {100, 200, 255, 255});

    char frameBuf[32];
    snprintf(frameBuf, sizeof(frameBuf), "Frame: %d / %d", gotchi->getFrame() + 1, (int)animFrames.at(name).size());
    DrawText(frameBuf, 16, 62, 14, {180, 180, 220, 255});

    // List all available actions, highlighting the active one.
    int ly = 100;
    DrawText("ACTIONS", 16, ly, 14, {100, 200, 255, 255});
    ly += 18;
    for (int i = 0; i < (int)actionNames.size(); i++) {
        Color c = (i == selectedAction) ? YELLOW : Color{180, 180, 220, 255};
        DrawText(actionNames[i].c_str(), 16, ly, 12, c);
        ly += 16;
    }
}
