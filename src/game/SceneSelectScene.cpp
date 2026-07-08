#include "SceneSelectScene.hpp"
#include "GameConstants.hpp"

static const Color SELECT_BG_COLOR = {18, 14, 24, 255};
static const float BUTTON_WIDTH = 320.0f;
static const float BUTTON_HEIGHT = 56.0f;
static const float BUTTON_SPACING = 20.0f;

SceneSelectScene::SceneSelectScene(SceneManager* manager)
    : Scene((float)GAME_W, (float)GAME_H, SELECT_BG_COLOR), sceneManager(manager) {
}

void SceneSelectScene::addSceneButton(const std::string& label, const std::string& sceneName, float y) {
    Button* btn = new Button({(float)GAME_W / 2.0f, y}, BUTTON_WIDTH, BUTTON_HEIGHT, label);
    btn->setAnchor("center");
    SceneManager* mgr = sceneManager;
    btn->setOnClick([mgr, sceneName]() {
        mgr->switchScene(sceneName, TransitionEffect::FADE, 0.5f);
    });
    buttons.push_back(std::unique_ptr<Button>(btn));
}

void SceneSelectScene::init() {
    buttons.clear();

    // Only the narrative/"world" side-scenes belong here -- the tomagotchi
    // pet-sim side has its own scenes, not listed on this hub.
    float startY = (float)GAME_H / 2.0f - 175.0f;
    addSceneButton("PIZZA PARLOR", "pizza_parlor", startY);
    addSceneButton("APARTMENT", "apartment", startY + (BUTTON_HEIGHT + BUTTON_SPACING));
    addSceneButton("THERAPIST'S OFFICE", "therapist_office", startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 2);
    addSceneButton("DATATEK SOLUTIONS (OFFICE)", "office", startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 3);
    addSceneButton("SCHOOL PICKUP", "school", startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 4);
    addSceneButton("3D MODEL TEST", "model3d_test", startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 5);
    addSceneButton("SCENE PREVIEW", "scene_preview", startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 6);

    // Future world-scenes get one more addSceneButton() call here.
}

void SceneSelectScene::update(float deltaTime) {
    Scene::update(deltaTime);

    SceneInputHandler* input = getInputHandler();
    for (auto& btn : buttons) {
        btn->update(input, deltaTime);
    }
}

void SceneSelectScene::draw() {
    Scene::draw();

    int titleWidth = MeasureText("TOM'S WORLD", 32);
    DrawText("TOM'S WORLD", (int)((float)GAME_W / 2.0f - titleWidth / 2.0f), 60, 32, {220, 220, 240, 255});

    const char* subtitle = "Select a scene to check in on him";
    int subWidth = MeasureText(subtitle, 14);
    DrawText(subtitle, (int)((float)GAME_W / 2.0f - subWidth / 2.0f), 100, 14, {150, 150, 170, 255});

    for (auto& btn : buttons) {
        btn->draw();
    }
}
