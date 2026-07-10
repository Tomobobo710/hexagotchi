#include "SchoolScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SchoolSkyEffect.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

// Matches the opaque sky color actually painted in schoolbg.png's daytime
// sky, sampled from the art -- schoolbg.png's sky region is otherwise
// transparent (see the scene-effect background discussion), so this is what
// shows through those gaps until an effect fills them.
SchoolScene::SchoolScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {202, 232, 250, 255}), dialog(sharedDialog) {
}

void SchoolScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    addEffect(new SchoolSkyEffect());

    tom = new SceneActor({440.0f, 400.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    karen = new SceneActor({620.0f, 380.0f}, 48.0f, 72.0f);
    karen->setTag("karen");
    karen->setVisible(false);
    addActor(karen);

    jimmy = new SceneActor({680.0f, 440.0f}, 28.0f, 36.0f);
    jimmy->setTag("jimmy");
    jimmy->setVisible(false);
    addActor(jimmy);

    background = AssetPack::loadTexture("backgrounds/schoolbg.png");

    poses[0][0] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Sad);
    poses[0][1] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Mid);
    poses[0][2] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Happy);
    poses[1][0] = CharacterRegistry::loadPose(CharacterId::Karen, PoseEmotion::Sad);
    poses[1][1] = CharacterRegistry::loadPose(CharacterId::Karen, PoseEmotion::Mid);
    poses[1][2] = CharacterRegistry::loadPose(CharacterId::Karen, PoseEmotion::Happy);
    poses[2][0] = CharacterRegistry::loadPose(CharacterId::Jimmy, PoseEmotion::Sad);
    poses[2][1] = CharacterRegistry::loadPose(CharacterId::Jimmy, PoseEmotion::Mid);
    poses[2][2] = CharacterRegistry::loadPose(CharacterId::Jimmy, PoseEmotion::Happy);

    // Ported from the JS prototype's "The School Pickup Incident" episode
    // almost line-for-line -- Karen needling Tom for being late, Jimmy's lost
    // tooth, the tooth fairy inflation joke, ending on Tom quietly giving up
    // his last $5.
    scenarios.push_back({
        { CharacterId::Narrator, "Tom arrives at Jimmy's school.\n3:35 PM. Pickup was at 3:30.",
          -1, false },
        { CharacterId::Karen, "You're late.",
          1, false },
        { CharacterId::Tom, "I'm FIVE minutes late Karen,\nI drove the whole way here.",
          0, false },
        { CharacterId::Karen, "Jimmy waited.\nBy himself.\nAgain.",
          1, false },
        { CharacterId::Tom, "There are TEACHERS here,\nhe wasn't ALONE --",
          0, true },
        { CharacterId::Jimmy, "Hi Dad! I lost a tooth!",
          2, false, PortraitEmotion::Happy },
        { CharacterId::Tom, "Oh buddy! Which one?",
          0, false },
        { CharacterId::Jimmy, "This one! Mom says the fairy\ngives five dollars now.",
          2, false, PortraitEmotion::Happy },
        { CharacterId::Tom, "...Five dollars.",
          0, false },
        { CharacterId::Karen, "Inflation, Tom.",
          1, false },
        { CharacterId::Tom, "I know what inflation IS Karen.\nI AM experiencing it.",
          0, true },
        { CharacterId::Narrator, "Tom gives Jimmy the tooth fairy money.\nIt is his last $5 bill.\nHe does not tell Jimmy this.",
          -1, false },
    });
}

void SchoolScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (activeScenario < 0) {
        // Ambient: Tom alone waiting outside, Karen and Jimmy only appear
        // during the scripted pickup itself.
        tomWaitTimer += deltaTime * 1.5f;
        tom->setPosition({440.0f, 400.0f + sinf(tomWaitTimer) * 3.0f});

        if (!getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(480.0f, 320.0f);
            getCamera()->setZoom(1.0f);
        }
    }

    if (activeScenario >= 0 && dialog->isVisible() && dialog->isFinished()) {
        SceneInputHandler* ih = getInputHandler();
        if (dialog->consumeAutoAdvance() || (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE)))) {
            advanceLine();
        }
    }
}

void SchoolScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
    else drawSchoolYard();
    drawTom(tom->getPosition());
    if (activeScenario >= 0) {
        drawKaren(karen->getPosition());
        drawJimmy(jimmy->getPosition());
    }
    EndMode2D();
}

void SchoolScene::cleanup() {
    Scene::cleanup();
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int actor = 0; actor < 3; actor++) {
        for (int emotion = 0; emotion < 3; emotion++) {
            if (poses[actor][emotion].id != 0) UnloadTexture(poses[actor][emotion]);
        }
    }
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the scenario table -- reset so scenarios doesn't
    // accumulate duplicates and a mid-scenario exit doesn't permanently
    // block triggerScenario().
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
}

void SchoolScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void SchoolScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool SchoolScene::isPlayingScenario() const {
    return activeScenario >= 0;
}

void SchoolScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[3] = {tom, karen, jimmy};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 3);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void SchoolScene::playLine(const SchoolLine& line) {
    dialog->setCharacter(line.speaker, line.emotion);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake);

    SceneActor* actorsByIndex[3] = {tom, karen, jimmy};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 3);
}

void SchoolScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void SchoolScene::focusCameraOn(int actorIndex, bool shake) {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = karen;
    else if (actorIndex == 2) target = jimmy;

    if (target) {
        Vector2 pos = target->getCenter();
        getCamera()->setPosition(pos.x, pos.y - 30.0f);
        getCamera()->zoomTo(1.4f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(5.0f, 0.3f);
    }
}

// --- Set dressing ---------------------------------------------------------
void SchoolScene::drawSchoolYard() {
    // Grass ground
    Color grass = {60, 90, 45, 255};
    DrawRectangle(0, 420, 1024, 156, grass);

    // School building in the background
    Color brick = {150, 90, 70, 255};
    DrawRectangle(600, 100, 380, 320, brick);
    Color roof = {90, 55, 45, 255};
    DrawRectangle(590, 90, 400, 20, roof);
    // Windows, evenly spaced
    Color windowGlass = {120, 150, 170, 255};
    for (int i = 0; i < 4; i++) {
        DrawRectangle(630 + i * 85, 140, 50, 60, windowGlass);
        DrawRectangle(630 + i * 85, 230, 50, 60, windowGlass);
    }
    // Entrance doors
    Color doorColor = {70, 45, 35, 255};
    DrawRectangle(760, 340, 60, 80, doorColor);

    // Flagpole
    Color pole = {120, 120, 120, 255};
    DrawRectangle(90, 150, 6, 270, pole);
    Color flag = {180, 60, 60, 255};
    DrawTriangle({96, 155}, {96, 195}, {150, 175}, flag);

    // Pickup-line bench where Tom waits
    Color bench = {110, 80, 55, 255};
    DrawRectangle(360, 440, 140, 14, bench);
    DrawRectangle(370, 454, 10, 30, bench);
    DrawRectangle(480, 454, 10, 30, bench);
}

void SchoolScene::drawTom(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    Texture2D pose = poses[0][1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 30.0f - pose.height), WHITE);
}

void SchoolScene::drawKaren(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 36.0f;

    Texture2D pose = poses[1][1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 34.0f - pose.height), WHITE);
}

void SchoolScene::drawJimmy(Vector2 pos) {
    float cx = pos.x + 14.0f;
    float cy = pos.y + 18.0f;

    Texture2D pose = poses[2][1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 16.0f - pose.height), WHITE);
}
