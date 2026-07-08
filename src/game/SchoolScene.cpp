#include "SchoolScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SchoolSkyEffect.hpp"
#include <cmath>

static const Color TOM_COLOR     = {139, 172, 15, 255};
static const Color KAREN_COLOR    = {200, 60, 90, 255};
static const Color JIMMY_COLOR      = {41, 128, 185, 255};   // matches JS JIMMY color scheme (blue kid)
static const Color NARRATOR_COLOR = {150, 150, 170, 255};

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

    // Ported from the JS prototype's "The School Pickup Incident" episode
    // almost line-for-line -- Karen needling Tom for being late, Jimmy's lost
    // tooth, the tooth fairy inflation joke, ending on Tom quietly giving up
    // his last $5.
    events.push_back({
        { "Narrator", "Tom arrives at Jimmy's school.\n3:35 PM. Pickup was at 3:30.",
          NARRATOR_COLOR, -1, false },
        { "Karen", "You're late.",
          KAREN_COLOR, 1, false },
        { "Tom",  "I'm FIVE minutes late Karen,\nI drove the whole way here.",
          TOM_COLOR, 0, false },
        { "Karen", "Jimmy waited.\nBy himself.\nAgain.",
          KAREN_COLOR, 1, false },
        { "Tom",  "There are TEACHERS here,\nhe wasn't ALONE --",
          TOM_COLOR, 0, true },
        { "Jimmy",   "Hi Dad! I lost a tooth!",
          JIMMY_COLOR, 2, false },
        { "Tom",  "Oh buddy! Which one?",
          TOM_COLOR, 0, false },
        { "Jimmy",   "This one! Mom says the fairy\ngives five dollars now.",
          JIMMY_COLOR, 2, false },
        { "Tom",  "...Five dollars.",
          TOM_COLOR, 0, false },
        { "Karen", "Inflation, Tom.",
          KAREN_COLOR, 1, false },
        { "Tom",  "I know what inflation IS Karen.\nI AM experiencing it.",
          TOM_COLOR, 0, true },
        { "Narrator", "Tom gives Jimmy the tooth fairy money.\nIt is his last $5 bill.\nHe does not tell Jimmy this.",
          NARRATOR_COLOR, -1, false },
    });
}

void SchoolScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (activeEvent < 0) {
        // Ambient: Tom alone waiting outside, Karen and Jimmy only appear
        // during the scripted pickup itself.
        tomWaitTimer += deltaTime * 1.5f;
        tom->setPosition({440.0f, 400.0f + sinf(tomWaitTimer) * 3.0f});

        if (!getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(480.0f, 320.0f);
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

void SchoolScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
    else drawSchoolYard();
    drawTom(tom->getPosition());
    if (activeEvent >= 0) {
        drawKaren(karen->getPosition());
        drawJimmy(jimmy->getPosition());
    }
    EndMode2D();
}

void SchoolScene::cleanup() {
    Scene::cleanup();
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the event table -- reset so events doesn't accumulate
    // duplicates and a mid-event exit doesn't permanently block triggerEvent().
    events.clear();
    activeEvent = -1;
    lineIndex = 0;
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

    DrawEllipse((int)cx, (int)(cy + 20), 26, 30, TOM_COLOR);
    DrawCircle((int)cx, (int)(cy - 14), 20, TOM_COLOR);

    Color darkTom = {70, 90, 5, 255};
    DrawEllipse((int)(cx - 8), (int)(cy - 14), 4, 2, darkTom);
    DrawEllipse((int)(cx + 8), (int)(cy - 14), 4, 2, darkTom);
    DrawLineEx({cx - 7, cy - 5}, {cx + 7, cy - 3}, 2.0f, darkTom);
    DrawLineEx({cx - 24, cy + 8}, {cx - 32, cy + 26}, 5.0f, TOM_COLOR);
    DrawLineEx({cx + 24, cy + 8}, {cx + 32, cy + 26}, 5.0f, TOM_COLOR);
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

void SchoolScene::drawJimmy(Vector2 pos) {
    float cx = pos.x + 14.0f;
    float cy = pos.y + 18.0f;

    // Small, energetic kid silhouette -- half Tom's height, bright and bouncy
    Color darkJimmy = {20, 70, 100, 255};
    DrawEllipse((int)cx, (int)(cy + 12), 14, 16, JIMMY_COLOR);
    DrawCircle((int)cx, (int)(cy - 8), 12, JIMMY_COLOR);

    // Big excited eyes
    DrawCircle((int)(cx - 5), (int)(cy - 9), 2, darkJimmy);
    DrawCircle((int)(cx + 5), (int)(cy - 9), 2, darkJimmy);
    // Big gap-tooth grin (missing tooth is the whole point of this scene)
    DrawRectangle((int)(cx - 4), (int)(cy - 2), 8, 3, darkJimmy);
    DrawRectangle((int)(cx - 1), (int)(cy - 2), 2, 3, JIMMY_COLOR);  // the gap

    // Backpack straps
    DrawLineEx({cx - 8, cy + 2}, {cx - 8, cy + 20}, 3.0f, darkJimmy);
    DrawLineEx({cx + 8, cy + 2}, {cx + 8, cy + 20}, 3.0f, darkJimmy);
}
