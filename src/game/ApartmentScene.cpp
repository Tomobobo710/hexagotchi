#include "ApartmentScene.hpp"
#include "GameConstants.hpp"
#include <cmath>

static const Color GARY_COLOR      = {139, 172, 15, 255};
static const Color NARRATOR_COLOR  = {150, 150, 170, 255};
static const Color PHONE_COLOR     = {230, 160, 60, 255};

ApartmentScene::ApartmentScene(DialogBox* sharedDialog)
    : Scene(1024.0f, 576.0f, {22, 18, 26, 255}), dialog(sharedDialog) {
}

void ApartmentScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1024.0f, 576.0f);

    gary = new SceneActor({470.0f, 380.0f}, 48.0f, 64.0f);
    gary->setTag("gary");
    gary->setVisible(false);
    addActor(gary);

    // Ported from the JS prototype's "Monday Morning" intro episode --
    // same beats (alarm, mirror, Karen's text, broken coffee machine),
    // adapted into our speaker/text/focus/shake line format. Karen's text
    // plays through a "KAREN (TEXT)" speaker, matching the original's PHONE
    // insert convention.
    events.push_back({
        { "Gary", "Ugh. 6:47 AM. Already late.",
          GARY_COLOR, 0, false },
        { "Gary", "The alarm has been going off for 40 minutes.\nI just... couldn't.",
          GARY_COLOR, 0, false },
        { "Narrator", "Gary shuffles to the bathroom.\nHe stares at himself in the mirror for 90 seconds.",
          NARRATOR_COLOR, -1, false },
        { "Gary", "I am a digital being.\nI do not have pores.\nWhy do I look like this.",
          GARY_COLOR, 0, false },
        { "Karen (text)", "'You were supposed to have the kids\nlast weekend. GARY.'",
          PHONE_COLOR, 0, true },
        { "Gary", "I KNOW Karen.\nI HAD THE KIDS.\nBloop ate my only good spatula.",
          GARY_COLOR, 0, false },
        { "Narrator", "Gary makes coffee.\nThe machine is broken.\nHe stares at it.",
          NARRATOR_COLOR, -1, false },
        { "Gary", "I need to go back out there soon.\nAct happy. Be a good pet.\n...I just need one minute.",
          GARY_COLOR, 0, false },
    });
}

void ApartmentScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (activeEvent < 0) {
        // Ambient: slow slumped sway, nothing else in the room moving --
        // this is meant to feel emptier and quieter than the pizza parlor.
        garySlumpTimer += deltaTime * 1.2f;
        gary->setPosition({470.0f, 380.0f + sinf(garySlumpTimer) * 3.0f});

        getCamera()->setPosition(512.0f, 288.0f);
        getCamera()->setZoom(1.0f);
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
    drawApartment();
    drawGary(gary->getPosition());
    EndMode2D();
}

void ApartmentScene::cleanup() {
    Scene::cleanup();
}

void ApartmentScene::triggerEvent(int index) {
    if (activeEvent >= 0) return;
    if (index < 0 || index >= (int)events.size()) return;

    activeEvent = index;
    lineIndex = 0;
    playLine(events[activeEvent][lineIndex]);
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
        Vector2 pos = gary->getCenter();
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

void ApartmentScene::drawGary(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    // Same slouched silhouette as the pizza parlor for continuity, just
    // drawn a little more sunken/tired here.
    DrawEllipse((int)cx, (int)(cy + 22), 26, 28, GARY_COLOR);
    DrawCircle((int)cx, (int)(cy - 12), 20, GARY_COLOR);

    Color darkGary = {70, 90, 5, 255};
    // Half-lidded eyes -- flatter than the pizza parlor version
    DrawRectangle((int)(cx - 12), (int)(cy - 14), 8, 3, darkGary);
    DrawRectangle((int)(cx + 4), (int)(cy - 14), 8, 3, darkGary);
    // Flat, exhausted mouth
    DrawLineEx({cx - 6, cy - 3}, {cx + 6, cy - 3}, 2.0f, darkGary);
    // Arms hanging straight down, not even lifted
    DrawLineEx({cx - 22, cy + 10}, {cx - 26, cy + 30}, 5.0f, GARY_COLOR);
    DrawLineEx({cx + 22, cy + 10}, {cx + 26, cy + 30}, 5.0f, GARY_COLOR);
}
