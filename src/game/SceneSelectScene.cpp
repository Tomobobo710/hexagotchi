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
    addSceneButtonAt(label, sceneName, (float)GAME_W / 2.0f, y);
}

void SceneSelectScene::addSceneButtonAt(const std::string& label, const std::string& sceneName, float x, float y) {
    addSceneButtonAt(label, sceneName, x, y, BUTTON_WIDTH);
}

void SceneSelectScene::addSceneButtonAt(const std::string& label, const std::string& sceneName, float x, float y, float width) {
    Button* btn = new Button({x, y}, width, BUTTON_HEIGHT, label);
    btn->setAnchor("center");
    SceneManager* mgr = sceneManager;
    btn->setOnClick([mgr, sceneName]() {
        mgr->switchScene(sceneName);
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
    addSceneButton("MERGE TRANSITION", "merge", startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 6);
    addSceneButton("TOY ANIMATION", "toy_animation", startY + (BUTTON_HEIGHT + BUTTON_SPACING) * 7);

    // First column is full (spans GAME_W/2 +/- BUTTON_WIDTH/2, i.e. 200..520
    // at GAME_W=720) -- new entries go in a narrower second column using the
    // remaining right margin instead.
    float col2Width = 160.0f;
    float col2X = (float)GAME_W - col2Width / 2.0f - 20.0f;
    addSceneButtonAt("CREDITS", "credits", col2X, startY, col2Width);

    // Future world-scenes get one more addSceneButton() (col 1, if there's
    // room) or addSceneButtonAt() (col 2) call here.
    // (Numpad 0 in any world scene toggles a wide view showing the whole
    // scene at once, for eyeballing 3D effect placement -- see
    // SceneCamera::toggleWideView() -- so there's no separate preview scene
    // needed anymore.)
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
