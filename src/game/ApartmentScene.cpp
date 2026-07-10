#include "ApartmentScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SceneDebugCamera.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

ApartmentScene::ApartmentScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {22, 18, 26, 255}), dialog(sharedDialog) {
}

void ApartmentScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    tom = new SceneActor({470.0f, 380.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    background = AssetPack::loadTexture("backgrounds/apartmentbg.png");

    tomPoses[0] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Sad);
    tomPoses[1] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Mid);
    tomPoses[2] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Happy);

    cityWindow = new CityWindowEffect();
    addEffect(cityWindow);

    // Ported from the JS prototype's "Monday Morning" intro episode --
    // same beats (alarm, mirror, Karen's text, broken coffee machine),
    // adapted into our speaker/text/focus/shake line format. Karen's text
    // plays through a "KAREN (TEXT)" speaker, matching the original's PHONE
    // insert convention.
    scenarios.push_back({
        { CharacterId::Tom, "Ugh. 6:47 AM. Already late.",
          0, false },
        { CharacterId::Tom, "The alarm has been going off for 40 minutes.\nI just... couldn't.",
          0, false },
        { CharacterId::Narrator, "Tom shuffles to the bathroom.\nHe stares at himself in the mirror for 90 seconds.",
          -1, false },
        { CharacterId::Tom, "I am a digital being.\nI do not have pores.\nWhy do I look like this.",
          0, false },
        { CharacterId::Phone, "'You were supposed to have the kids\nlast weekend. TOM.'",
          0, true },
        { CharacterId::Tom, "I KNOW Karen.\nI HAD THE KIDS.\nBimmy ate my only good spatula.",
          0, false },
        { CharacterId::Narrator, "Tom makes coffee.\nThe machine is broken.\nHe stares at it.",
          -1, false },
        { CharacterId::Tom, "I need to go back out there soon.\nAct happy. Be a good pet.\n...I just need one minute.",
          0, false },
    });
}

void ApartmentScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (getEntrySceneName() == "scene_select") updateSceneDebugCamera(cityWindow, getCamera(), deltaTime);

    if (cityWindow && cityWindow->consumeTrainShakeRequest()) {
        // Duration computed by the effect itself from the actual visible
        // window/lead-in/trail-out timing, not a fixed guess -- see
        // CityWindowEffect::getShakeDuration().
        float dur = cityWindow->getShakeDuration();
        Vector2 p = getCamera()->getPosition();
        TraceLog(LOG_INFO, "[TRAINDBG] shake() called dur=%.3f pos=(%.2f,%.2f) zoom=%.3f", dur, p.x, p.y, getCamera()->getZoom());
        getCamera()->shake(6.0f, dur);
    }

    if (activeScenario < 0) {
        // Ambient: slow slumped sway, nothing else in the room moving --
        // this is meant to feel emptier and quieter than the pizza parlor.
        tomSlumpTimer += deltaTime * 1.2f;
        tom->setPosition({470.0f, 380.0f + sinf(tomSlumpTimer) * 3.0f});

        // setPosition()/setZoom() hard-reset the camera every single frame --
        // skip the re-pin while a shake (from the train passing) is mid-
        // flight, or while wide view is active, so those don't get stomped
        // back out before ever being visible (same reason as isShaking()).
        if (!getCamera()->isShaking() && !getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(512.0f, 288.0f);
            getCamera()->setZoom(1.0f);
        }
    }

    {
        static int dbgFrame = 0;
        static bool wasShaking = false;
        dbgFrame++;
        bool nowShaking = getCamera()->isShaking();
        if (nowShaking != wasShaking) {
            Vector2 p = getCamera()->getPosition();
            TraceLog(LOG_INFO, "[TRAINDBG] isShaking transitioned %s->%s pos=(%.2f,%.2f) zoom=%.3f",
                wasShaking ? "true" : "false", nowShaking ? "true" : "false", p.x, p.y, getCamera()->getZoom());
            wasShaking = nowShaking;
        }
        if (dbgFrame % 10 == 0) {
            Vector2 p = getCamera()->getPosition();
            TraceLog(LOG_INFO, "[TRAINDBG] frame=%d pos=(%.2f,%.2f) zoom=%.3f shaking=%d",
                dbgFrame, p.x, p.y, getCamera()->getZoom(), nowShaking ? 1 : 0);
        }
    }

    if (activeScenario >= 0 && dialog->isVisible() && dialog->isFinished()) {
        SceneInputHandler* ih = getInputHandler();
        if (dialog->consumeAutoAdvance() || (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE)))) {
            advanceLine();
        }
    }
}

void ApartmentScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) {
        DrawTexture(background, 0, 0, WHITE);
    } else {
        drawApartment();
    }
    drawTom(tom->getPosition());
    EndMode2D();

    if (getEntrySceneName() == "scene_select") drawSceneDebugCameraReadout(cityWindow, 16, 16);
}

void ApartmentScene::cleanup() {
    Scene::cleanup();
    cityWindow = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 3; i++) {
        if (tomPoses[i].id != 0) UnloadTexture(tomPoses[i]);
    }
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the scenario table -- reset so scenarios doesn't
    // accumulate duplicates and a mid-scenario exit doesn't permanently
    // block triggerScenario().
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
}

void ApartmentScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void ApartmentScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool ApartmentScene::isPlayingScenario() const {
    return activeScenario >= 0;
}

void ApartmentScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[1] = {tom};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 1);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void ApartmentScene::playLine(const ApartmentLine& line) {
    dialog->setCharacter(line.speaker, line.emotion);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake);

    SceneActor* actorsByIndex[1] = {tom};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 1);
}

void ApartmentScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void ApartmentScene::focusCameraOn(int actorIndex, bool shake) {
    if (actorIndex == 0) {
        Vector2 pos = tom->getCenter();
        getCamera()->setPosition(pos.x, pos.y - 30.0f);
        getCamera()->zoomTo(1.3f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(5.0f, 0.3f);
    }
}

// --- Set dressing ---------------------------------------------------------
void ApartmentScene::drawApartment() {
    // Bed
    Color bedFrame = {90, 70, 50, 255};
    Color bedSheet = {70, 90, 110, 255};
    DrawRectangle(80, 340, 220, 120, bedFrame);
    DrawRectangle(90, 330, 200, 40, bedSheet);

    // Window with dim early-morning light
    Color windowFrame = {60, 50, 60, 255};
    Color windowSky = {40, 55, 80, 255};
    DrawRectangle(720, 100, 180, 160, windowFrame);
    DrawRectangle(730, 110, 160, 140, windowSky);
    DrawLine(810, 110, 810, 250, windowFrame);
    DrawLine(730, 180, 890, 180, windowFrame);

    // Kitchen counter with the broken coffee machine
    Color counter = {80, 60, 70, 255};
    DrawRectangle(600, 420, 260, 100, counter);
    Color machine = {50, 50, 55, 255};
    DrawRectangle(640, 380, 50, 45, machine);
    DrawRectangle(650, 372, 10, 10, {30, 30, 30, 255});
    // Small dead/off indicator light
    DrawCircle(645, 385, 3, {90, 20, 20, 255});

    // Bathroom mirror on the far wall
    Color mirrorFrame = {70, 60, 80, 255};
    Color mirrorGlass = {50, 55, 65, 255};
    DrawRectangle(400, 60, 90, 130, mirrorFrame);
    DrawRectangle(408, 68, 74, 114, mirrorGlass);
}

void ApartmentScene::drawTom(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    Texture2D pose = tomPoses[1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 30.0f - pose.height), WHITE);
}
