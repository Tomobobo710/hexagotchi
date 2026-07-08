#include "OfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "rlgl.h"
#include <cmath>

static const Color TOM_COLOR     = {139, 172, 15, 255};
static const Color BOSS_COLOR     = {142, 68, 173, 255};   // matches JS BOSS: '#8e44ad'
static const Color NARRATOR_COLOR = {150, 150, 170, 255};

OfficeScene::OfficeScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {20, 22, 28, 255}), dialog(sharedDialog) {
}

void OfficeScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    background = AssetPack::loadTexture("backgrounds/officebg.png");

    tom = new SceneActor({420.0f, 400.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    boss = new SceneActor({650.0f, 380.0f}, 50.0f, 76.0f);
    boss->setTag("boss");
    boss->setVisible(false);
    addActor(boss);

    // --- Event 0: "Performance Review", ported almost line-for-line ---
    events.push_back({
        { "Narrator", "Tom arrives at Datatek Solutions.\n9:14 AM. His shift started at 9:00.",
          NARRATOR_COLOR, -1, false },
        { "Boss",  "Tom. My office.\nBring the Hendricks file.",
          BOSS_COLOR, 1, false },
        { "Tom",  "...What's the Hendricks file.",
          TOM_COLOR, 0, false },
        { "Boss",  "The one I emailed you about.\nFive times.\nAlso on the physical memo.",
          BOSS_COLOR, 1, false },
        { "Tom",  "I don't have a desk.\nYou took my desk.",
          TOM_COLOR, 0, true },
        { "Boss",  "We hot-desk now Tom.\nIt's a flex workspace environment.",
          BOSS_COLOR, 1, false },
        { "Tom",  "I sit on a yoga ball.\nI am 34 years old.",
          TOM_COLOR, 0, false },
        { "Narrator", "The review goes poorly.\nTom does not receive the 3% raise.\nHe receives a 'verbal commendation'.",
          NARRATOR_COLOR, -1, false },
        { "Tom",  "A verbal commendation.\nI have $11 in my account.\nA verbal commendation.",
          TOM_COLOR, 0, true },
    });

    // --- Event 1: "The Promotion (Sort Of)", ported almost line-for-line ---
    events.push_back({
        { "Narrator", "Tom's boss calls him in.\nThis time it's different.",
          NARRATOR_COLOR, -1, false },
        { "Boss",  "Tom we're expanding your role.\nCongratulations.",
          BOSS_COLOR, 1, false },
        { "Tom",  "...A raise?",
          TOM_COLOR, 0, false },
        { "Boss",  "More responsibility!\nYou'll now manage the Henderson account\nAND the Brickford account.",
          BOSS_COLOR, 1, false },
        { "Tom",  "Okay and the raise --",
          TOM_COLOR, 0, false },
        { "Boss",  "We're calling it a 'growth opportunity'.",
          BOSS_COLOR, 1, false },
        { "Tom",  "So no raise.",
          TOM_COLOR, 0, false },
        { "Boss",  "We're also moving your start time to 8:30.",
          BOSS_COLOR, 1, false },
        { "Tom",  "Earlier?!",
          TOM_COLOR, 0, true },
        { "Boss",  "It's the flex workspace, Tom.\nThe ball yoga spot is first come, first served now.",
          BOSS_COLOR, 1, false },
        { "Tom",  "I... cannot process this\nright now.",
          TOM_COLOR, 0, false },
        { "Narrator", "Tom processes it on his commute home.\nHe misses his exit.\nTwice.",
          NARRATOR_COLOR, -1, false },
    });
}

void OfficeScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (activeEvent < 0) {
        // Ambient: Tom balancing/wobbling on the yoga ball, boss absent
        // (only appears during the scripted events, called into "his office").
        tomWobbleTimer += deltaTime * 3.0f;
        tom->setPosition({420.0f, 400.0f + sinf(tomWobbleTimer) * 6.0f});

        if (!getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(460.0f, 320.0f);
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

void OfficeScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();

    // Background art first (its own 2D pass).
    BeginMode2D(cam);
    drawOffice();
    EndMode2D();

    // 3D object layer: renders ON TOP of the background art but BEHIND the
    // actors, since the actor draws come after this. Needs its own 3D mode
    // (can't share the 2D camera), so the 2D pass is split around it.
    draw3DObjectLayer();

    // Actors last, on top of the 3D object.
    BeginMode2D(cam);
    drawTom(tom->getPosition());
    if (activeEvent >= 0) drawBoss(boss->getPosition());
    EndMode2D();
}

void OfficeScene::draw3DObjectLayer() {
    Camera3D cam3d = {};
    cam3d.position   = {0.0f, 0.0f, 6.0f};
    cam3d.target     = {0.0f, 0.0f, 0.0f};
    cam3d.up         = {0.0f, 1.0f, 0.0f};
    cam3d.fovy       = 45.0f;
    cam3d.projection = CAMERA_PERSPECTIVE;

    cubeSpin += GetFrameTime() * 40.0f;

    BeginMode3D(cam3d);
        rlPushMatrix();
        rlRotatef(cubeSpin, 0.3f, 1.0f, 0.0f);
        DrawCube({0.0f, 0.0f, 0.0f}, 1.5f, 1.5f, 1.5f, Color{200, 80, 80, 255});
        DrawCubeWires({0.0f, 0.0f, 0.0f}, 1.5f, 1.5f, 1.5f, WHITE);
        rlPopMatrix();
    EndMode3D();
}

void OfficeScene::cleanup() {
    Scene::cleanup();
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the event table -- reset so events doesn't accumulate
    // duplicates and a mid-event exit doesn't permanently block triggerEvent().
    events.clear();
    activeEvent = -1;
    lineIndex = 0;

    if (background.id != 0) { UnloadTexture(background); background = {0}; }
}

void OfficeScene::triggerEvent(int index) {
    if (activeEvent >= 0) return;
    if (index < 0 || index >= (int)events.size()) return;

    activeEvent = index;
    lineIndex = 0;
    playLine(events[activeEvent][lineIndex]);
}

bool OfficeScene::isPlayingEvent() const {
    return activeEvent >= 0;
}

void OfficeScene::advanceLine() {
    if (activeEvent < 0) return;

    lineIndex++;
    auto& seq = events[activeEvent];
    if (lineIndex >= (int)seq.size()) {
        endEvent();
        return;
    }
    playLine(seq[lineIndex]);
}

void OfficeScene::playLine(const OfficeLine& line) {
    dialog->setSpeakerName(line.speaker);
    dialog->setSpeakerColor(line.speakerColor);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake);
}

void OfficeScene::endEvent() {
    activeEvent = -1;
    lineIndex = 0;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void OfficeScene::focusCameraOn(int actorIndex, bool shake) {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = boss;

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
void OfficeScene::drawOffice() {
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);

    // Open-plan "flex workspace" -- rows of identical hot-desks, mostly empty
    Color deskColor = {70, 70, 85, 255};
    Color deskDark = {50, 50, 62, 255};
    for (int row = 0; row < 2; row++) {
        for (int col = 0; col < 3; col++) {
            float dx = 80.0f + col * 220.0f;
            float dy = 90.0f + row * 100.0f;
            DrawRectangle((int)dx, (int)dy, 160, 12, deskColor);
            DrawRectangle((int)dx, (int)(dy + 12), 160, 4, deskDark);
        }
    }

    // Glass-walled "boss office" on the right
    Color glassFrame = {40, 40, 50, 255};
    Color glassPane = {90, 100, 120, 120};
    DrawRectangle(760, 60, 220, 300, glassFrame);
    DrawRectangle(772, 72, 196, 276, glassPane);
    // Small nameplate
    DrawRectangle(790, 40, 100, 16, {30, 30, 38, 255});

    // Motivational poster, because of course there is one
    Color posterFrame = {60, 55, 40, 255};
    Color posterBg = {200, 190, 160, 255};
    DrawRectangle(60, 300, 100, 130, posterFrame);
    DrawRectangle(68, 308, 84, 114, posterBg);
}

void OfficeScene::drawTom(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    // Balanced precariously on the yoga ball (drawn beneath him)
    Color ballColor = {200, 80, 80, 200};
    DrawCircle((int)cx, (int)(cy + 46), 22, ballColor);

    DrawEllipse((int)cx, (int)(cy + 18), 26, 28, TOM_COLOR);
    DrawCircle((int)cx, (int)(cy - 16), 20, TOM_COLOR);

    Color darkTom = {70, 90, 5, 255};
    DrawEllipse((int)(cx - 8), (int)(cy - 16), 4, 2, darkTom);
    DrawEllipse((int)(cx + 8), (int)(cy - 16), 4, 2, darkTom);
    DrawLineEx({cx - 7, cy - 7}, {cx + 7, cy - 5}, 2.0f, darkTom);

    // Arms out slightly for balance
    DrawLineEx({cx - 24, cy}, {cx - 34, cy + 10}, 5.0f, TOM_COLOR);
    DrawLineEx({cx + 24, cy}, {cx + 34, cy + 10}, 5.0f, TOM_COLOR);
}

void OfficeScene::drawBoss(Vector2 pos) {
    float cx = pos.x + 25.0f;
    float cy = pos.y + 38.0f;

    // Sharp-suited, imposing -- taller and boxier than Tom
    Color darkBoss = {90, 40, 110, 255};
    DrawRectangle((int)(cx - 20), (int)(cy - 6), 40, 52, BOSS_COLOR);
    DrawCircle((int)cx, (int)(cy - 30), 18, BOSS_COLOR);

    // Stern narrow eyes
    DrawRectangle((int)(cx - 11), (int)(cy - 32), 6, 2, darkBoss);
    DrawRectangle((int)(cx + 5), (int)(cy - 32), 6, 2, darkBoss);
    // Flat unimpressed mouth
    DrawLineEx({cx - 6, cy - 22}, {cx + 6, cy - 22}, 2.0f, darkBoss);

    // Tie
    DrawTriangle({cx - 3, cy - 4}, {cx + 3, cy - 4}, {cx, cy + 10}, darkBoss);
}
