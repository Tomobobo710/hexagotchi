#include "TherapistOfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include <cmath>

static const Color TOM_COLOR      = {139, 172, 15, 255};
static const Color THERAPIST_COLOR = {22, 160, 133, 255};   // matches JS THERAPIST: '#16a085'
static const Color NARRATOR_COLOR  = {150, 150, 170, 255};

TherapistOfficeScene::TherapistOfficeScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {26, 22, 20, 255}), dialog(sharedDialog) {
}

void TherapistOfficeScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    tom = new SceneActor({380.0f, 400.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    therapist = new SceneActor({620.0f, 390.0f}, 48.0f, 72.0f);
    therapist->setTag("therapist");
    therapist->setVisible(false);
    addActor(therapist);

    background = AssetPack::loadTexture("backgrounds/therapistbg.png");

    // Ported from the JS prototype's "The Last Session" episode almost
    // line-for-line -- the digital-pet metaphor breakdown, ending on the
    // copay hike. NARRATOR lines have no speaking actor, matching how the
    // apartment scene handles them.
    events.push_back({
        { "Narrator",  "Tom's last covered therapy session.\nHe has been saving the hard stuff for today.",
          NARRATOR_COLOR, -1, false },
        { "Therapist", "So Tom, how have you been\nprocessing the divorce?",
          THERAPIST_COLOR, 1, false },
        { "Tom",      "I... okay so you know how I'm\nsometimes a digital pet?",
          TOM_COLOR, 0, false },
        { "Therapist", "We've talked about this metaphor.",
          THERAPIST_COLOR, 1, false },
        { "Tom",      "It's not a metaphor.\nSomebody watches me.\nI have to poop in front of them.",
          TOM_COLOR, 0, false },
        { "Therapist", "...Tom.",
          THERAPIST_COLOR, 1, false },
        { "Tom",      "I have to perform happiness.\nOn demand.\nWhile they press little buttons at me.",
          TOM_COLOR, 0, false },
        { "Therapist", "I think that IS a metaphor Tom.\nFor work maybe? For the marriage?",
          THERAPIST_COLOR, 1, false },
        { "Tom",      "Last week they made me eat\nthree meals in a row.\nI was not hungry.",
          TOM_COLOR, 0, true },
        { "Therapist", "...Your copay has increased\nto $200 starting next session.",
          THERAPIST_COLOR, 1, true },
        { "Tom",      "Of course it has.",
          TOM_COLOR, 0, false },
    });
}

void TherapistOfficeScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (activeEvent < 0) {
        // Ambient: both sitting, small idle motion -- Tom fidgets slightly
        // more than the therapist, who stays composed/still.
        tomFidgetTimer += deltaTime * 2.5f;
        therapistNodTimer += deltaTime * 0.8f;

        tom->setPosition({380.0f, 400.0f + sinf(tomFidgetTimer) * 3.0f});
        therapist->setPosition({620.0f, 390.0f + sinf(therapistNodTimer) * 1.5f});

        if (!getCamera()->isWideViewEnabled()) {
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

void TherapistOfficeScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
    else drawOffice();
    drawTom(tom->getPosition());
    drawTherapist(therapist->getPosition());
    EndMode2D();
}

void TherapistOfficeScene::cleanup() {
    Scene::cleanup();
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the event table -- reset so events doesn't accumulate
    // duplicates and a mid-event exit doesn't permanently block triggerEvent().
    events.clear();
    activeEvent = -1;
    lineIndex = 0;
}

void TherapistOfficeScene::triggerEvent(int index) {
    if (activeEvent >= 0) return;
    if (index < 0 || index >= (int)events.size()) return;

    activeEvent = index;
    lineIndex = 0;
    playLine(events[activeEvent][lineIndex]);
}

bool TherapistOfficeScene::isPlayingEvent() const {
    return activeEvent >= 0;
}

void TherapistOfficeScene::advanceLine() {
    if (activeEvent < 0) return;

    lineIndex++;
    auto& seq = events[activeEvent];
    if (lineIndex >= (int)seq.size()) {
        endEvent();
        return;
    }
    playLine(seq[lineIndex]);
}

void TherapistOfficeScene::playLine(const TherapistLine& line) {
    dialog->setSpeakerName(line.speaker);
    dialog->setSpeakerColor(line.speakerColor);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake);
}

void TherapistOfficeScene::endEvent() {
    activeEvent = -1;
    lineIndex = 0;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void TherapistOfficeScene::focusCameraOn(int actorIndex, bool shake) {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = therapist;

    if (target) {
        Vector2 pos = target->getCenter();
        getCamera()->setPosition(pos.x, pos.y - 40.0f);
        getCamera()->zoomTo(1.4f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(5.0f, 0.3f);
    }
}

// --- Set dressing ---------------------------------------------------------
void TherapistOfficeScene::drawOffice() {
    // Bookshelf behind the therapist
    Color shelfWood = {90, 65, 45, 255};
    DrawRectangle(700, 100, 220, 260, shelfWood);
    Color shelfDark = {60, 42, 30, 255};
    for (int y = 130; y < 360; y += 65) {
        DrawRectangle(700, y, 220, 6, shelfDark);
    }
    // Books, mixed colors
    Color bookColors[] = {{160, 60, 60, 255}, {60, 90, 140, 255}, {80, 140, 90, 255}, {150, 130, 60, 255}};
    for (int i = 0; i < 4; i++) {
        DrawRectangle(715 + i * 45, 100, 30, 30, bookColors[i]);
    }

    // Window with soft daylight, opposite side
    Color windowFrame = {70, 60, 55, 255};
    Color windowSky = {130, 150, 170, 255};
    DrawRectangle(60, 90, 160, 150, windowFrame);
    DrawRectangle(70, 100, 140, 130, windowSky);
    DrawLine(140, 100, 140, 230, windowFrame);
    DrawLine(70, 165, 210, 165, windowFrame);

    // Small side table with a box of tissues between the two chairs
    Color table = {80, 60, 50, 255};
    DrawRectangle(495, 470, 60, 40, table);
    Color tissueBox = {230, 230, 220, 255};
    DrawRectangle(505, 455, 40, 20, tissueBox);

    // Rug beneath both chairs
    Color rug = {110, 55, 55, 255};
    DrawEllipse(512, 470, 260, 60, rug);
}

void TherapistOfficeScene::drawTom(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    // Same slouched silhouette, seated -- lower/wider than standing poses
    DrawEllipse((int)cx, (int)(cy + 24), 28, 26, TOM_COLOR);
    DrawCircle((int)cx, (int)(cy - 10), 20, TOM_COLOR);

    Color darkTom = {70, 90, 5, 255};
    DrawEllipse((int)(cx - 8), (int)(cy - 12), 4, 2, darkTom);
    DrawEllipse((int)(cx + 8), (int)(cy - 12), 4, 2, darkTom);
    DrawLineEx({cx - 6, cy - 2}, {cx + 6, cy}, 2.0f, darkTom);
    // Hands folded in lap
    DrawEllipse((int)cx, (int)(cy + 20), 10, 6, darkTom);
}

void TherapistOfficeScene::drawTherapist(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 36.0f;

    // Composed, upright seated posture -- notepad on lap
    Color darkTherapist = {12, 100, 85, 255};
    DrawRectangle((int)(cx - 18), (int)(cy - 4), 36, 44, THERAPIST_COLOR);
    DrawCircle((int)cx, (int)(cy - 26), 16, THERAPIST_COLOR);

    // Calm, neutral expression
    DrawRectangle((int)(cx - 9), (int)(cy - 28), 5, 2, darkTherapist);
    DrawRectangle((int)(cx + 4), (int)(cy - 28), 5, 2, darkTherapist);
    DrawLineEx({cx - 5, cy - 19}, {cx + 5, cy - 19}, 2.0f, darkTherapist);

    // Notepad on lap
    Color notepad = {235, 230, 210, 255};
    DrawRectangle((int)(cx - 12), (int)(cy + 24), 24, 18, notepad);
    DrawLine((int)(cx - 8), (int)(cy + 29), (int)(cx + 8), (int)(cy + 29), darkTherapist);
    DrawLine((int)(cx - 8), (int)(cy + 34), (int)(cx + 4), (int)(cy + 34), darkTherapist);
}
