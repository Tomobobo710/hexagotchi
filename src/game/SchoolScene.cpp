#include "SchoolScene.hpp"
#include "GameConstants.hpp"
#include <cmath>

static const Color GARY_COLOR     = {139, 172, 15, 255};
static const Color KAREN_COLOR    = {200, 60, 90, 255};
static const Color ZAP_COLOR      = {41, 128, 185, 255};   // matches JS ZAP color scheme (blue kid)
static const Color NARRATOR_COLOR = {150, 150, 170, 255};

SchoolScene::SchoolScene(DialogBox* sharedDialog)
    : Scene(1024.0f, 576.0f, {36, 40, 30, 255}), dialog(sharedDialog) {
}

void SchoolScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1024.0f, 576.0f);

    gary = new SceneActor({440.0f, 400.0f}, 48.0f, 64.0f);
    gary->setTag("gary");
    gary->setVisible(false);
    addActor(gary);

    karen = new SceneActor({620.0f, 380.0f}, 48.0f, 72.0f);
    karen->setTag("karen");
    karen->setVisible(false);
    addActor(karen);

    zap = new SceneActor({680.0f, 440.0f}, 28.0f, 36.0f);
    zap->setTag("zap");
    zap->setVisible(false);
    addActor(zap);

    // Ported from the JS prototype's "The School Pickup Incident" episode
    // almost line-for-line -- Karen needling Gary for being late, Zap's lost
    // tooth, the tooth fairy inflation joke, ending on Gary quietly giving up
    // his last $5.
    events.push_back({
        { "Narrator", "Gary arrives at Zap's school.\n3:35 PM. Pickup was at 3:30.",
          NARRATOR_COLOR, -1, false },
        { "Karen", "You're late.",
          KAREN_COLOR, 1, false },
        { "Gary",  "I'm FIVE minutes late Karen,\nI drove the whole way here.",
          GARY_COLOR, 0, false },
        { "Karen", "Zap waited.\nBy himself.\nAgain.",
          KAREN_COLOR, 1, false },
        { "Gary",  "There are TEACHERS here,\nhe wasn't ALONE --",
          GARY_COLOR, 0, true },
        { "Zap",   "Hi Dad! I lost a tooth!",
          ZAP_COLOR, 2, false },
        { "Gary",  "Oh buddy! Which one?",
          GARY_COLOR, 0, false },
        { "Zap",   "This one! Mom says the fairy\ngives five dollars now.",
          ZAP_COLOR, 2, false },
        { "Gary",  "...Five dollars.",
          GARY_COLOR, 0, false },
        { "Karen", "Inflation, Gary.",
          KAREN_COLOR, 1, false },
        { "Gary",  "I know what inflation IS Karen.\nI AM experiencing it.",
          GARY_COLOR, 0, true },
        { "Narrator", "Gary gives Zap the tooth fairy money.\nIt is his last $5 bill.\nHe does not tell Zap this.",
          NARRATOR_COLOR, -1, false },
    });
}

void SchoolScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (activeEvent < 0) {
        // Ambient: Gary alone waiting outside, Karen and Zap only appear
        // during the scripted pickup itself.
        garyWaitTimer += deltaTime * 1.5f;
        gary->setPosition({440.0f, 400.0f + sinf(garyWaitTimer) * 3.0f});

        getCamera()->setPosition(480.0f, 320.0f);
        getCamera()->setZoom(1.0f);
    }

    if (activeEvent >= 0 && dialog->isVisible() && dialog->isFinished()) {
        SceneInputHandler* ih = getInputHandler();
        if (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE))) {
            advanceLine();
        }
    }
}

void SchoolScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    drawSchoolYard();
    drawGary(gary->getPosition());
    if (activeEvent >= 0) {
        drawKaren(karen->getPosition());
        drawZap(zap->getPosition());
    }
    EndMode2D();
}

void SchoolScene::cleanup() {
    Scene::cleanup();
}

void SchoolScene::triggerEvent(int index) {
    if (activeEvent >= 0) return;
    if (index < 0 || index >= (int)events.size()) return;

    activeEvent = index;
    lineIndex = 0;
    playLine(events[activeEvent][lineIndex]);
}

bool SchoolScene::isPlayingEvent() const {
    return activeEvent >= 0;
}

void SchoolScene::advanceLine() {
    if (activeEvent < 0) return;

    lineIndex++;
    auto& seq = events[activeEvent];
    if (lineIndex >= (int)seq.size()) {
        endEvent();
        return;
    }
    playLine(seq[lineIndex]);
}

void SchoolScene::playLine(const SchoolLine& line) {
    dialog->setSpeakerName(line.speaker);
    dialog->setSpeakerColor(line.speakerColor);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake);
}

void SchoolScene::endEvent() {
    activeEvent = -1;
    lineIndex = 0;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void SchoolScene::focusCameraOn(int actorIndex, bool shake) {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = gary;
    else if (actorIndex == 1) target = karen;
    else if (actorIndex == 2) target = zap;

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

    // Pickup-line bench where Gary waits
    Color bench = {110, 80, 55, 255};
    DrawRectangle(360, 440, 140, 14, bench);
    DrawRectangle(370, 454, 10, 30, bench);
    DrawRectangle(480, 454, 10, 30, bench);
}

void SchoolScene::drawGary(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    DrawEllipse((int)cx, (int)(cy + 20), 26, 30, GARY_COLOR);
    DrawCircle((int)cx, (int)(cy - 14), 20, GARY_COLOR);

    Color darkGary = {70, 90, 5, 255};
    DrawEllipse((int)(cx - 8), (int)(cy - 14), 4, 2, darkGary);
    DrawEllipse((int)(cx + 8), (int)(cy - 14), 4, 2, darkGary);
    DrawLineEx({cx - 7, cy - 5}, {cx + 7, cy - 3}, 2.0f, darkGary);
    DrawLineEx({cx - 24, cy + 8}, {cx - 32, cy + 26}, 5.0f, GARY_COLOR);
    DrawLineEx({cx + 24, cy + 8}, {cx + 32, cy + 26}, 5.0f, GARY_COLOR);
}

void SchoolScene::drawKaren(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 36.0f;

    Color darkKaren = {110, 20, 40, 255};
    DrawTriangle({cx - 22, cy + 40}, {cx + 22, cy + 40}, {cx, cy - 4}, KAREN_COLOR);
    DrawRectangle((int)(cx - 16), (int)(cy - 4), 32, 20, KAREN_COLOR);
    DrawCircle((int)cx, (int)(cy - 26), 16, KAREN_COLOR);

    DrawRectangle((int)(cx - 10), (int)(cy - 28), 6, 2, darkKaren);
    DrawRectangle((int)(cx + 4), (int)(cy - 28), 6, 2, darkKaren);
    DrawLineEx({cx - 6, cy - 18}, {cx + 6, cy - 18}, 2.0f, darkKaren);
    // Arms crossed, checking a watch -- impatience posture
    DrawLineEx({cx - 16, cy + 2}, {cx + 12, cy + 12}, 6.0f, darkKaren);
    DrawLineEx({cx + 16, cy + 2}, {cx - 12, cy + 12}, 6.0f, darkKaren);
}

void SchoolScene::drawZap(Vector2 pos) {
    float cx = pos.x + 14.0f;
    float cy = pos.y + 18.0f;

    // Small, energetic kid silhouette -- half Gary's height, bright and bouncy
    Color darkZap = {20, 70, 100, 255};
    DrawEllipse((int)cx, (int)(cy + 12), 14, 16, ZAP_COLOR);
    DrawCircle((int)cx, (int)(cy - 8), 12, ZAP_COLOR);

    // Big excited eyes
    DrawCircle((int)(cx - 5), (int)(cy - 9), 2, darkZap);
    DrawCircle((int)(cx + 5), (int)(cy - 9), 2, darkZap);
    // Big gap-tooth grin (missing tooth is the whole point of this scene)
    DrawRectangle((int)(cx - 4), (int)(cy - 2), 8, 3, darkZap);
    DrawRectangle((int)(cx - 1), (int)(cy - 2), 2, 3, ZAP_COLOR);  // the gap

    // Backpack straps
    DrawLineEx({cx - 8, cy + 2}, {cx - 8, cy + 20}, 3.0f, darkZap);
    DrawLineEx({cx + 8, cy + 2}, {cx + 8, cy + 20}, 3.0f, darkZap);
}
