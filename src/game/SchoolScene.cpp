#include "SchoolScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "AudioManager.hpp"
#include "SchoolSkyEffect.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

// Positions/scales/flips from the scene editor's layout.json. Tom on the far
// left facing right; Karen center; the two boys on the right at 0.7 scale
// (they're kids). flipX hardcoded per-actor in the draw fns.
static const Vector2 TOM_POS   = {104.0f, 278.0f};
static const Vector2 KAREN_POS = {594.0f, 357.0f};
static const Vector2 JIMMY_POS = {760.0f, 379.0f};
static const Vector2 BIMMY_POS = {928.0f, 428.0f};
static const float KID_SCALE = 0.7f;

// Matches the opaque sky color painted in schoolbg.png's daytime sky.
SchoolScene::SchoolScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {202, 232, 250, 255}), dialog(sharedDialog) {
}

void SchoolScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    addEffect(new SchoolSkyEffect());

    tom = new SceneActor(TOM_POS, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    karen = new SceneActor(KAREN_POS, 48.0f, 72.0f);
    karen->setTag("karen");
    karen->setVisible(false);
    addActor(karen);

    jimmy = new SceneActor(JIMMY_POS, 28.0f, 36.0f);
    jimmy->setTag("jimmy");
    jimmy->setVisible(false);
    addActor(jimmy);

    bimmy = new SceneActor(BIMMY_POS, 28.0f, 36.0f);
    bimmy->setTag("bimmy");
    bimmy->setVisible(false);
    addActor(bimmy);

    background = AssetPack::loadTexture("backgrounds/schoolbg.png");

    for (int e = 0; e < 4; e++) {
        tomPoses[e]   = CharacterRegistry::loadPose(CharacterId::Tom,   (PoseEmotion)e);
        karenPoses[e] = CharacterRegistry::loadPose(CharacterId::Karen, (PoseEmotion)e);
        jimmyPoses[e] = CharacterRegistry::loadPose(CharacterId::Jimmy, (PoseEmotion)e);
        bimmyPoses[e] = CharacterRegistry::loadPose(CharacterId::Bimmy, (PoseEmotion)e);
    }

    // --- Scenario 0: "The School Pickup Incident" (reworked) --------------
    // Karen needles Tom for being late; Jimmy lost a tooth (the fairy pays $20
    // now); Bimmy chimes in; Karen tells Tom to just go home and take a break,
    // he needs one; then reveals Ronzer bought the boys $100 Candy City gift
    // cards each. Kids hyped, Tom quietly flattened.
    scenarios.push_back({
        { CharacterId::Narrator, "Tom arrives at the school.\n3:35 PM. Pickup was at 3:30.",
          -1, false },
        { CharacterId::Karen, "You're late.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "I'm FIVE minutes late, Karen.\nI walked the whole way here.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Karen, "The boys waited.\nBy themselves.\nAgain.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "There are TEACHERS here,\nthey weren't ALONE --",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Jimmy, "Hi Dad! I lost a tooth!",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Tom, "Oh, buddy! Which one?",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Jimmy, "This one! Mom says the fairy\ngives TWENTY dollars now.",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Tom, "...Twenty dollars.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Bimmy, "The fairy's rich, Dad.\nEveryone knows that.",
          3, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Karen, "Inflation, Tom.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Tom, "I KNOW what inflation is, Karen.\nI am actively EXPERIENCING it.",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Bimmy, "What does experiencing inflation mean?",
          3, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "It means your Dad needs to extend his\ncredit limit, sweetie.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },

        // Karen softens -- or seems to. "Go home, take a break."
        { CharacterId::Karen, "You know what, Tom? Just go home.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Karen, "Take a break. Seriously.\nYou look like you need one.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "...I'm fine. I want the time\nwith the boys, Karen.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Karen, "Well -- about that. Boys?\nTell your father the good news.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Jimmy, "RONZER got us gift cards!!",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Bimmy, "A HUNDRED DOLLARS. EACH.\nTo CANDY CITY!!",
          3, true, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Jimmy, "Can we go NOW? Can we go RIGHT NOW?",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Karen, "We're going right now.\nSay bye to your dad.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Bimmy, "Bye Dad! Love you!\nRonzer's the BEST!",
          3, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Tom, "...Love you too, guys.\nGet the sour ones.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Narrator, "Tom walked the whole way here.\nHe waves until the car is out of sight.",
          -1, false },
    });
}

void SchoolScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (activeScenario < 0) {
        // Ambient: Tom alone waiting outside; the family only appears during
        // the scripted pickup itself.
        tomWaitTimer += deltaTime * 1.5f;
        tom->setPosition({TOM_POS.x, TOM_POS.y + sinf(tomWaitTimer) * 3.0f});

        if (!getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(512.0f, 360.0f);
            getCamera()->setZoom(1.0f);
        }
    }

    // Keep the camera on the live speaker every frame (matches OfficeScene).
    if (activeScenario >= 0 && currentFocusActor >= 0
        && !getCamera()->isShaking() && !getCamera()->isWideViewEnabled()) {
        Vector2 t;
        if (cameraTargetFor(currentFocusActor, t)) {
            getCamera()->followPosition(t, 8.0f);
        }
    }

    if (activeScenario >= 0 && dialog->isVisible()) {
        SceneInputHandler* ih = getInputHandler();
        // Check for skip action (jump to end of scenario)
        if (ih && ih->isActionPressed(INPUT_ACTION_SKIP)) {
            endScenario();
            return;
        }
        // Check for normal advance (next line)
        if (dialog->isFinished()) {
            bool manualAdvance = ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE) || ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT));
            if (manualAdvance) AudioManager::Get().playClick();  // no click on auto-advance
            if (dialog->consumeAutoAdvance() || manualAdvance) {
                advanceLine();
            }
        }
    }
}

void SchoolScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
    else drawSchoolYard();

    // Stop drawing actors once the end fade is ramping (same as OfficeScene).
    if (endElapsed < 0.0f) {
        drawTom(tom->getPosition());
        if (activeScenario >= 0) {
            drawKaren(karen->getPosition());
            drawJimmy(jimmy->getPosition());
            drawBimmy(bimmy->getPosition());
        }
    }
    EndMode2D();

    if (endElapsed >= 0.0f) {
        float t = endElapsed / END_FADE_DURATION;
        if (t > 1.0f) t = 1.0f;
        unsigned char alpha = (unsigned char)(t * 255.0f);
        DrawRectangle(0, 0, (int)getWidth(), (int)getHeight(), Color{0, 0, 0, alpha});
    }
}

void SchoolScene::cleanup() {
    Scene::cleanup();
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 4; i++) {
        if (tomPoses[i].id != 0)   UnloadTexture(tomPoses[i]);
        if (karenPoses[i].id != 0) UnloadTexture(karenPoses[i]);
        if (jimmyPoses[i].id != 0) UnloadTexture(jimmyPoses[i]);
        if (bimmyPoses[i].id != 0) UnloadTexture(bimmyPoses[i]);
    }
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the scenario table -- reset so scenarios doesn't accumulate
    // duplicates and a mid-scenario exit doesn't permanently block trigger.
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = -1.0f;
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
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void SchoolScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[4] = {tom, karen, jimmy, bimmy};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 4);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void SchoolScene::playLine(const SchoolLine& line) {
    dialog->setCharacter(line.speaker, line.emotion, line.firstTimeName);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    tomPoseEmotion = line.tomPoseEmotion;
    karenPoseEmotion = line.karenPoseEmotion;
    jimmyPoseEmotion = line.jimmyPoseEmotion;
    bimmyPoseEmotion = line.bimmyPoseEmotion;

    SceneActor* actorsByIndex[4] = {tom, karen, jimmy, bimmy};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 4);
}

void SchoolScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void SchoolScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
    currentFocusActor = actorIndex;

    Vector2 t;
    if (cameraTargetFor(actorIndex, t)) {
        if (cut) getCamera()->setPosition(t.x, t.y);
        else getCamera()->followPosition(t, 8.0f);
        getCamera()->zoomTo(2.0f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(5.0f, 0.3f);
    }
}

// Same recipe as OfficeScene: aim into the visible body from the pose's
// top-left. Kids are drawn at 0.7 scale, so their aim offset is scaled down to
// match (a 0.7 pose is ~180px tall vs ~256 for the adults). Returns false for
// Narrator / actorIndex -1 so the camera holds.
bool SchoolScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    float aimY = 210.0f;  // adults
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = karen;
    else if (actorIndex == 2) { target = jimmy; aimY = 210.0f * KID_SCALE; }
    else if (actorIndex == 3) { target = bimmy; aimY = 210.0f * KID_SCALE; }
    if (!target) return false;

    Vector2 p = target->getPosition();
    out = { p.x + 160.0f, p.y + aimY };
    return true;
}

// Draw a pose top-left at pos, native size * scale, flipX via negative source
// width -- exactly like the editor and the other scenes.
static void drawSchoolPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void SchoolScene::drawTom(Vector2 pos) {
    drawSchoolPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ true, 1.0f);   // far left, faces right
}

void SchoolScene::drawKaren(Vector2 pos) {
    drawSchoolPose(karenPoses[(int)karenPoseEmotion], pos, /*flipX*/ false, 1.0f);  // center, faces left toward Tom
}

void SchoolScene::drawJimmy(Vector2 pos) {
    drawSchoolPose(jimmyPoses[(int)jimmyPoseEmotion], pos, /*flipX*/ false, KID_SCALE);  // right, kid scale
}

void SchoolScene::drawBimmy(Vector2 pos) {
    drawSchoolPose(bimmyPoses[(int)bimmyPoseEmotion], pos, /*flipX*/ false, KID_SCALE);  // far right, kid scale
}

// --- Set dressing (fallback only, if schoolbg.png is missing) -------------
void SchoolScene::drawSchoolYard() {
    Color grass = {60, 90, 45, 255};
    DrawRectangle(0, 420, 1280, 300, grass);

    Color brick = {150, 90, 70, 255};
    DrawRectangle(600, 100, 380, 320, brick);
    Color roof = {90, 55, 45, 255};
    DrawRectangle(590, 90, 400, 20, roof);
    Color windowGlass = {120, 150, 170, 255};
    for (int i = 0; i < 4; i++) {
        DrawRectangle(630 + i * 85, 140, 50, 60, windowGlass);
        DrawRectangle(630 + i * 85, 230, 50, 60, windowGlass);
    }
    Color doorColor = {70, 45, 35, 255};
    DrawRectangle(760, 340, 60, 80, doorColor);

    Color pole = {120, 120, 120, 255};
    DrawRectangle(90, 150, 6, 270, pole);
    Color flag = {180, 60, 60, 255};
    DrawTriangle({96, 155}, {96, 195}, {150, 175}, flag);
}
