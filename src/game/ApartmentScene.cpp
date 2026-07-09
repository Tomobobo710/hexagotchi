#include "ApartmentScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SceneDebugCamera.hpp"
#include <cmath>

static const Color TOM_COLOR      = {139, 172, 15, 255};
static const Color NARRATOR_COLOR  = {150, 150, 170, 255};
static const Color PHONE_COLOR     = {230, 160, 60, 255};

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

    cityWindow = new CityWindowEffect();
    addEffect(cityWindow);

    // Ported from the JS prototype's "Monday Morning" intro episode --
    // same beats (alarm, mirror, Karen's text, broken coffee machine),
    // adapted into our speaker/text/focus/shake line format. Karen's text
    // plays through a "KAREN (TEXT)" speaker, matching the original's PHONE
    // insert convention.
    events.push_back({
        { "Tom", "Ugh. 6:47 AM. Already late.",
          TOM_COLOR, 0, false },
        { "Tom", "The alarm has been going off for 40 minutes.\nI just... couldn't.",
          TOM_COLOR, 0, false },
        { "Narrator", "Tom shuffles to the bathroom.\nHe stares at himself in the mirror for 90 seconds.",
          NARRATOR_COLOR, -1, false },
        { "Tom", "I am a digital being.\nI do not have pores.\nWhy do I look like this.",
          TOM_COLOR, 0, false },
        { "Karen (text)", "'You were supposed to have the kids\nlast weekend. TOM.'",
          PHONE_COLOR, 0, true },
        { "Tom", "I KNOW Karen.\nI HAD THE KIDS.\nBloop ate my only good spatula.",
          TOM_COLOR, 0, false },
        { "Narrator", "Tom makes coffee.\nThe machine is broken.\nHe stares at it.",
          NARRATOR_COLOR, -1, false },
        { "Tom", "I need to go back out there soon.\nAct happy. Be a good pet.\n...I just need one minute.",
          TOM_COLOR, 0, false },
    });
}

void ApartmentScene::update(float deltaTime) {
    Scene::update(deltaTime);

    updateSceneDebugCamera(cityWindow, getCamera(), deltaTime);

    if (cityWindow && cityWindow->consumeTrainShakeRequest()) {
        // Duration computed by the effect itself from the actual visible
        // window/lead-in/trail-out timing, not a fixed guess -- see
        // CityWindowEffect::getShakeDuration().
        getCamera()->shake(6.0f, cityWindow->getShakeDuration());
    }

    if (activeEvent < 0) {
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

    if (activeEvent >= 0 && dialog->isVisible() && dialog->isFinished()) {
        SceneInputHandler* ih = getInputHandler();
        if (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE))) {
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

    drawSceneDebugCameraReadout(cityWindow, 16, 16);
}

void ApartmentScene::cleanup() {
    Scene::cleanup();
    cityWindow = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the event table -- reset so events doesn't accumulate
    // duplicates and a mid-event exit doesn't permanently block triggerEvent().
    events.clear();
    activeEvent = -1;
    lineIndex = 0;
}

void ApartmentScene::triggerEvent(int index) {
    if (activeEvent >= 0) return;
    if (index < 0 || index >= (int)events.size()) return;

    activeEvent = index;
    lineIndex = 0;
    playLine(events[activeEvent][lineIndex]);
}

void ApartmentScene::triggerStoryEvent(int eventIndex) {
    triggerEvent(eventIndex);
}

bool ApartmentScene::isPlayingEvent() const {
    return activeEvent >= 0;
}

void ApartmentScene::advanceLine() {
    if (activeEvent < 0) return;

    lineIndex++;
    auto& seq = events[activeEvent];
    if (lineIndex >= (int)seq.size()) {
        endEvent();
        return;
    }
    playLine(seq[lineIndex]);
}

void ApartmentScene::playLine(const ApartmentLine& line) {
    dialog->setSpeakerName(line.speaker);
    dialog->setSpeakerColor(line.speakerColor);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake);
}

void ApartmentScene::endEvent() {
    activeEvent = -1;
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

    // Same slouched silhouette as the pizza parlor for continuity, just
    // drawn a little more sunken/tired here.
    DrawEllipse((int)cx, (int)(cy + 22), 26, 28, TOM_COLOR);
    DrawCircle((int)cx, (int)(cy - 12), 20, TOM_COLOR);

    Color darkTom = {70, 90, 5, 255};
    // Half-lidded eyes -- flatter than the pizza parlor version
    DrawRectangle((int)(cx - 12), (int)(cy - 14), 8, 3, darkTom);
    DrawRectangle((int)(cx + 4), (int)(cy - 14), 8, 3, darkTom);
    // Flat, exhausted mouth
    DrawLineEx({cx - 6, cy - 3}, {cx + 6, cy - 3}, 2.0f, darkTom);
    // Arms hanging straight down, not even lifted
    DrawLineEx({cx - 22, cy + 10}, {cx - 26, cy + 30}, 5.0f, TOM_COLOR);
    DrawLineEx({cx + 22, cy + 10}, {cx + 26, cy + 30}, 5.0f, TOM_COLOR);
}
