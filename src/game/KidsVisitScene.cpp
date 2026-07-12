#include "KidsVisitScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "AudioManager.hpp"
#include "SceneDebugCamera.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

// Positions from the scene editor's layout.json. Tom on the right, the two
// boys on the left. All static -- a living-room scene, no walking. flipX is
// hardcoded per-actor in the draw fns.
static const Vector2 TOM_POS   = {735.0f, 309.0f};
static const Vector2 BIMMY_POS = { 92.0f, 361.0f};
static const Vector2 JIMMY_POS = {358.0f, 315.0f};

// Defined in main.cpp -- true for the frame in which a click was consumed by
// the global Tom-world pause button/menu.
extern bool IsPauseUiClaimingClick();

KidsVisitScene::KidsVisitScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {22, 18, 26, 255}), dialog(sharedDialog) {
}

void KidsVisitScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    background = AssetPack::loadTexture("backgrounds/apartmentbg.png");

    for (int e = 0; e < 4; e++) {
        tomPoses[e]   = CharacterRegistry::loadPose(CharacterId::Tom,   (PoseEmotion)e);
        bimmyPoses[e] = CharacterRegistry::loadPose(CharacterId::Bimmy, (PoseEmotion)e);
        jimmyPoses[e] = CharacterRegistry::loadPose(CharacterId::Jimmy, (PoseEmotion)e);
    }

    cityWindow = new CityWindowEffect();
    cityWindow->setDebugPitch(-15.0f);
    addEffect(cityWindow);

    tom = new SceneActor(TOM_POS, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    bimmy = new SceneActor(BIMMY_POS, 44.0f, 60.0f);
    bimmy->setTag("bimmy");
    bimmy->setVisible(false);
    addActor(bimmy);

    jimmy = new SceneActor(JIMMY_POS, 44.0f, 60.0f);
    jimmy->setTag("jimmy");
    jimmy->setVisible(false);
    addActor(jimmy);

    // --- Scenario 0: "Tom's Weekend With The Kids" ------------------------
    // The boys are at Tom's cold apartment on his custody turn. They gripe
    // about the bus and the place, favorably rate Ronzer's car, and land on
    // the question every divorced dad dreads. Tom fumbles it honestly. Kids
    // are blunt and loving at the same time.
    scenarios.push_back({
        // Kids report the cold matter-of-factly (Mid), not sad -- they're just
        // stating facts. Tom overcompensates cheerfully.
        { CharacterId::Bimmy, "Dad, why's it so COLD in here?\nI can see my breath.",
          1, false, false, PortraitEmotion::Mid, "Bimmy (Tom's son)",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "It's -- it's bracing! It's good for you.\nBuilds character.",
          0, false, false, PortraitEmotion::Happy, "Tom Gatchi",
          {}, {}, PoseEmotion::Happy, PoseEmotion::Mid, PoseEmotion::Mid },
        // The Ronzer comparisons -- kids genuinely delighted, oblivious to the sting.
        { CharacterId::Jimmy, "Ronzer's apartment has a HOT TUB.",
          2, false, false, PortraitEmotion::Happy, "Jimmy (Tom's other son)",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Bimmy, "And the bus here SMELLED.\nA man was eating soup on it.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Jimmy, "Ronzer picks us up in the car.\nIt has the screens in the seats.",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        // Tom spinning it -- putting on a brave face.
        { CharacterId::Tom, "The bus is an adventure, guys.\nYou meet... interesting people.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Bimmy, "The soup man asked if I was\nalso 'running from someone.'",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Scared, PoseEmotion::Happy },
        // Deadpan defeat -- that one lands.
        { CharacterId::Tom, "...Okay, that one's fair.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Scared, PoseEmotion::Happy },

        // The comparison sharpens.
        { CharacterId::Jimmy, "Ronzer can lift the whole couch.\nWith ONE arm.",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Bimmy, "Can you lift the couch, Dad?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Tom, "I don't NEED to lift the couch, Bimmy.\nI... rearrange it with my mind.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Jimmy, "That's not real.",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "Neither is a promotion at a gym\nyou don't work at, but here we are.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Bimmy, "What?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "Nothing. Grown-up nothing.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },

        // The turn: they still love him -- warm, sincere.
        { CharacterId::Jimmy, "We still like it here, Dad.\nEven if it's cold.",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Bimmy, "Yeah. You're funnier than Ronzer.\nRonzer doesn't get jokes.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Mid },
        // Genuinely touched.
        { CharacterId::Tom, "...Thanks, buddy. That -- that means a lot.",
          0, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Mid },
        // The question every divorced dad dreads -- the child asks it innocently
        // (Mid), which is exactly what makes it land. Tom's pose drops to Scared.
        { CharacterId::Bimmy, "So when are you moving back home?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Jimmy, "Mom said you needed 'space.'\nIs the space almost done?",
          2, true, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid, PoseEmotion::Mid },

        // Tom fumbles it the way a real dad would.
        { CharacterId::Tom, "It's -- so, sometimes grown-ups...\nthey need to be apart for a while to...",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "It's not that anyone did anything...\nwrong, exactly, it's just...",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Sad },
        { CharacterId::Bimmy, "Just what?",
          1, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Sad },
        { CharacterId::Tom, "...It's complicated, guys.\nI'm sorry. I know that's the worst answer.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Sad },
        { CharacterId::Jimmy, "That IS the worst answer.",
          2, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Sad },
        // The recovery -- Tom pulls it back with warmth. Kids melt.
        { CharacterId::Tom, "Yeah. I'm working on a better one.\nCome here. You're not too cold to hug your old man?",
          0, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Happy, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Bimmy, "Only because it's freezing and\nyou're the warmest thing in here.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Narrator, "It is the best Tom will feel all week.\nThe bus home leaves in twenty minutes.",
          -1, false },
    });
}

void KidsVisitScene::update(float deltaTime) {
    Scene::update(deltaTime);
    if (isPaused()) return;

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (getEntrySceneName() == "scene_select") updateSceneDebugCamera(cityWindow, getCamera(), deltaTime);

    if (cityWindow && cityWindow->consumeTrainShakeRequest()) {
        getCamera()->shake(6.0f, cityWindow->getShakeDuration());
    }

    if (activeScenario < 0) {
        // Ambient: Tom slumped at his spot with a slow sway; kids absent.
        tomSlumpTimer += deltaTime * 1.2f;
        tom->setPosition({TOM_POS.x, TOM_POS.y + sinf(tomSlumpTimer) * 3.0f});

        if (!getCamera()->isShaking() && !getCamera()->isWideViewEnabled()) {
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
        if (ih && ih->isActionPressed(INPUT_ACTION_SKIP)) {
            endScenario();
            return;
        }
        if (dialog->isFinished()) {
            bool manualAdvance = ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE) ||
                                        (ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsPauseUiClaimingClick()));
            if (manualAdvance) AudioManager::Get().playClick();  // no click on auto-advance
            if (dialog->consumeAutoAdvance() || manualAdvance) {
                advanceLine();
            }
        }
    }
}

void KidsVisitScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);

    // Stop drawing actors once the end fade is ramping (same as OfficeScene).
    if (endElapsed < 0.0f) {
        drawTom(tom->getPosition());
        if (activeScenario >= 0) {
            drawBimmy(bimmy->getPosition());
            drawJimmy(jimmy->getPosition());
        }
    }
    EndMode2D();

    if (getEntrySceneName() == "scene_select") drawSceneDebugCameraReadout(cityWindow, 16, 16);

    if (endElapsed >= 0.0f) {
        float t = endElapsed / END_FADE_DURATION;
        if (t > 1.0f) t = 1.0f;
        unsigned char alpha = (unsigned char)(t * 255.0f);
        DrawRectangle(0, 0, (int)getWidth(), (int)getHeight(), Color{0, 0, 0, alpha});
    }
}

void KidsVisitScene::cleanup() {
    Scene::cleanup();
    cityWindow = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 4; i++) {
        if (tomPoses[i].id != 0)   UnloadTexture(tomPoses[i]);
        if (bimmyPoses[i].id != 0) UnloadTexture(bimmyPoses[i]);
        if (jimmyPoses[i].id != 0) UnloadTexture(jimmyPoses[i]);
    }
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = -1.0f;
}

void KidsVisitScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void KidsVisitScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool KidsVisitScene::isPlayingScenario() const {
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void KidsVisitScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[3] = {tom, bimmy, jimmy};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 3);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void KidsVisitScene::playLine(const KidsLine& line) {
    dialog->setCharacter(line.speaker, line.emotion, line.firstTimeName);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    tomPoseEmotion = line.tomPoseEmotion;
    bimmyPoseEmotion = line.bimmyPoseEmotion;
    jimmyPoseEmotion = line.jimmyPoseEmotion;

    SceneActor* actorsByIndex[3] = {tom, bimmy, jimmy};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 3);
}

void KidsVisitScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void KidsVisitScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
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

// Same recipe as OfficeScene/ApartmentScene: aim into the visible body from the
// pose's top-left. Returns false for Narrator / actorIndex -1 so the camera
// holds where it is.
bool KidsVisitScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = bimmy;
    else if (actorIndex == 2) target = jimmy;
    if (!target) return false;

    Vector2 p = target->getPosition();
    out = { p.x + 160.0f, p.y + 210.0f };
    return true;
}

// Draw a pose top-left at pos, native size * scale, flipX via negative source
// width -- exactly like the editor and the other scenes.
static void drawKidsPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void KidsVisitScene::drawTom(Vector2 pos) {
    drawKidsPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ false, 1.0f);   // on the right, faces left toward the kids
}

void KidsVisitScene::drawBimmy(Vector2 pos) {
    drawKidsPose(bimmyPoses[(int)bimmyPoseEmotion], pos, /*flipX*/ true, 1.0f);   // faces right toward Dad
}

void KidsVisitScene::drawJimmy(Vector2 pos) {
    drawKidsPose(jimmyPoses[(int)jimmyPoseEmotion], pos, /*flipX*/ true, 1.0f);   // faces right toward Dad
}
