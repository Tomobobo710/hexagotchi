#include "EndingScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

// Positions/scales/flips/waypoints from the scene editor's layout.json.
// Index scheme: 0=Tom 1=Loraine 2=Ronzer 3=Mark 4=Karen.
static const Vector2 TOM_POS       = {833.0f, 243.0f};
static const Vector2 TOM_WP        = {420.0f, 339.0f};    // good ending: Tom steps in to deck Ronzer
static const Vector2 LORAINE_SPAWN = { 50.0f, 788.0f};
static const Vector2 LORAINE_WP    = {192.0f, 335.0f};
static const Vector2 RONZER_SPAWN  = {305.0f, 811.0f};
static const Vector2 RONZER_WP1    = {313.0f, 346.0f};
static const Vector2 RONZER_WP2    = {694.0f, 1079.0f};   // exits bottom
static const Vector2 MARK_SPAWN    = {732.0f, 772.0f};
static const Vector2 MARK_WP       = {402.0f, 311.0f};
static const Vector2 KAREN_SPAWN   = {1440.0f, 312.0f};   // off right edge
static const Vector2 KAREN_WP      = {742.0f, 315.0f};

static const float TOM_SCALE   = 0.8f;
static const float SIDE_SCALE  = 0.8f;   // Loraine, Ronzer, Karen
static const float MARK_SCALE  = 1.0f;

EndingScene::EndingScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {16, 16, 22, 255}), dialog(sharedDialog) {
}

void EndingScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    background = AssetPack::loadTexture("backgrounds/parkinglotbg.png");

    for (int e = 0; e < 4; e++) {
        tomPoses[e]     = CharacterRegistry::loadPose(CharacterId::Tom,     (PoseEmotion)e);
        lorainePoses[e] = CharacterRegistry::loadPose(CharacterId::Loraine, (PoseEmotion)e);
        ronzerPoses[e]  = CharacterRegistry::loadPose(CharacterId::Ronzer,  (PoseEmotion)e);
        markPoses[e]    = CharacterRegistry::loadPose(CharacterId::Mark,    (PoseEmotion)e);
        karenPoses[e]   = CharacterRegistry::loadPose(CharacterId::Karen,   (PoseEmotion)e);
    }

    tom = new SceneActor(TOM_POS, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    loraine = new SceneActor(LORAINE_SPAWN, 50.0f, 76.0f);
    loraine->setTag("loraine");
    loraine->setVisible(false);
    addActor(loraine);

    ronzer = new SceneActor(RONZER_SPAWN, 50.0f, 76.0f);
    ronzer->setTag("ronzer");
    ronzer->setVisible(false);
    addActor(ronzer);

    mark = new SceneActor(MARK_SPAWN, 50.0f, 76.0f);
    mark->setTag("mark");
    mark->setVisible(false);
    addActor(mark);

    karen = new SceneActor(KAREN_SPAWN, 48.0f, 72.0f);
    karen->setTag("karen");
    karen->setVisible(false);
    addActor(karen);

    // Both endings are the same Back-to-the-Future parking-lot beat -- Ronzer
    // (the Biff) gets too frisky with Loraine, someone decks him, Karen catches
    // Ronzer cheating and dumps him. They are DELIBERATELY separate scenarios
    // (not a shared opening + tails): the GOOD one plays out with TOM as the
    // hero, the BAD one with Mark. Some duplication is fine and intended. The
    // "???" is Loraine's scream heard before we cut to her -- overrideName
    // "???", no portrait -- so the player doesn't know it's her yet.
    // ScenarioDirector picks which scenario loads off the happiness ledger.

    // --- Scenario 0: GOOD ending (all 3 happiness checkpoints passed) ------
    // TOM steps up and decks Ronzer himself. Mark isn't in this scenario at
    // all. Loraine -- who has read Tom's performance metrics aloud all game and
    // LOVES his stats -- recognizes him and asks HIM out. Ends on Tom bashful.
    scenarios.push_back({
        { CharacterId::Tom, "Huh?",
          0, false, false, PortraitEmotion::Mid, "Tom Gatchi",
          /*movesAtStart*/ {
              ActorMove{2, {RONZER_WP1}, 240.0f},   // Ronzer up
              ActorMove{1, {LORAINE_WP}, 240.0f},   // Loraine up
          }, {},
          PoseEmotion::Mid, PoseEmotion::Scared, PoseEmotion::Mid,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Narrator, "AHHHHHH GET AWAY FROM ME\nYOU FREAK!",
          0, true, false, PortraitEmotion::Mid, "???",
          {}, {},
          PoseEmotion::Scared, PoseEmotion::Scared, PoseEmotion::Mid,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Ronzer, "RONZER RONZER!",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Scared, PoseEmotion::Scared, PoseEmotion::Happy,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Loraine, "I'm not THAT kind of girl,\nyou weirdo!",
          1, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ { ActorMove{0, {TOM_WP}, 300.0f} }, {},   // TOM charges in
          PoseEmotion::Mid, PoseEmotion::Scared, PoseEmotion::Happy,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Tom, "Get your hands off of her!",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Sad, PoseEmotion::Scared, PoseEmotion::Happy,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Narrator, "Tom connects a right hook.",
          0, true, false, PortraitEmotion::Mid, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Scared, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Karen, "Baby?! What's going on out here?!",
          4, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ { ActorMove{4, {KAREN_WP}, 300.0f} }, {},
          PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Loraine, "Is that your WOMAN? Oooooh, I just\nKNEW you were a slimeball.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Karen, "I can't BELIEVE it! I never want to\nsee you again! You can have these BACK!",
          4, true, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Narrator, "Karen throws a stack of gift cards at Ronzer.",
          2, true, false, PortraitEmotion::Mid, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Ronzer, "Ronzer...",
          2, false, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Karen, "I don't want to HEAR it.\nGet out of my sight!",
          4, false, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Ronzer, "Ronzer Ronzer...",
          2, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ {
              ActorMove{2, {RONZER_WP2}, 320.0f},        // Ronzer slinks off (exits bottom)
              ActorMove{4, {KAREN_SPAWN}, 320.0f},       // Karen retreats to her start
          }, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
        // Loraine recognizes Tom -- she's read his stats all game and loves them.
        { CharacterId::Loraine, "Wait... Tom? Tom Gatchi?\nFrom the reports?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, "I read your numbers every single day.\nI always KNEW you were a stand-up guy.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, "Say -- you wanna get out of here?\nI know a nice place for a drink, right down the road.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "Aw, shucks... me?\nI mean -- yeah. Yeah, I'd like that.",
          0, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Happy, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
    });

    // --- Scenario 1: BAD ending (checkpoints NOT all passed) ---------------
    // Same beat, but TOM just watches. MARK is the one who steps up and decks
    // Ronzer, and Loraine leaves with MARK. Tom's world rearranges itself
    // around him and he's left saying "that was awkward."
    scenarios.push_back({
        { CharacterId::Tom, "Huh?",
          0, false, false, PortraitEmotion::Mid, "Tom Gatchi",
          /*movesAtStart*/ {
              ActorMove{2, {RONZER_WP1}, 240.0f},   // Ronzer up
              ActorMove{1, {LORAINE_WP}, 240.0f},   // Loraine up
          }, {},
          PoseEmotion::Mid, PoseEmotion::Scared, PoseEmotion::Mid,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Narrator, "AHHHHHH GET AWAY FROM ME\nYOU FREAK!",
          0, true, false, PortraitEmotion::Mid, "???",
          {}, {},
          PoseEmotion::Scared, PoseEmotion::Scared, PoseEmotion::Mid,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Ronzer, "RONZER RONZER!",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Scared, PoseEmotion::Scared, PoseEmotion::Happy,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Loraine, "I'm not THAT kind of girl,\nyou weirdo!",
          1, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ { ActorMove{3, {MARK_WP}, 260.0f} }, {},   // Mark charges in
          PoseEmotion::Scared, PoseEmotion::Scared, PoseEmotion::Happy,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Mark, "Get your hands off of her!",
          3, false, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Scared, PoseEmotion::Happy,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Narrator, "Mark connects a right hook.",
          3, true, false, PortraitEmotion::Mid, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Scared, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Karen, "Baby?! What's going on out here?!",
          4, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ { ActorMove{4, {KAREN_WP}, 300.0f} }, {},
          PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Scared },
        { CharacterId::Loraine, "Is that your WOMAN? Oooooh, I just\nKNEW you were a slimeball.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Karen, "I can't BELIEVE it! I never want to\nsee you again! You can have these BACK!",
          4, true, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Narrator, "Karen throws a stack of gift cards at Ronzer.",
          2, true, false, PortraitEmotion::Mid, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Scared,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Ronzer, "Ronzer...",
          2, false, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Karen, "I don't want to HEAR it.\nGet out of my sight!",
          4, false, false, PortraitEmotion::Sad, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Sad },
        { CharacterId::Ronzer, "Ronzer Ronzer...",
          2, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ {
              ActorMove{2, {RONZER_WP2}, 320.0f},
              ActorMove{4, {KAREN_SPAWN}, 320.0f},
          }, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, "Uh -- thanks. Who ARE you?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Mark, "Oh. I'm Mark. I do maintenance\non Tom's apartment.",
          3, false, false, PortraitEmotion::Mid, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Sad,
          PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, "Maintenance, huh?\nI LOVE maintenance.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Loraine, "Say -- you wanna get out of here?\nI know a nice place for a drink, right down the road.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "...That was awkward.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {},
          PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Sad,
          PoseEmotion::Happy, PoseEmotion::Mid },
    });
}

void EndingScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (activeScenario < 0) {
        tomIdleTimer += deltaTime * 1.5f;
        tom->setPosition({TOM_POS.x, TOM_POS.y + sinf(tomIdleTimer) * 3.0f});

        if (!getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(512.0f, 360.0f);
            getCamera()->setZoom(1.0f);
        }
    }

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
            if (dialog->consumeAutoAdvance() || (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE) || ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT)))) {
                advanceLine();
            }
        }
    }
}

void EndingScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);

    if (endElapsed < 0.0f) {
        drawTom(tom->getPosition());
        if (activeScenario >= 0) {
            drawLoraine(loraine->getPosition());
            drawRonzer(ronzer->getPosition());
            // Mark only exists in the BAD ending (scenario 1). In the GOOD
            // ending Tom is the hero and Mark is absent, so don't draw him.
            if (activeScenario == 1) drawMark(mark->getPosition());
            drawKaren(karen->getPosition());
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

void EndingScene::cleanup() {
    Scene::cleanup();
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 4; i++) {
        if (tomPoses[i].id != 0)     UnloadTexture(tomPoses[i]);
        if (lorainePoses[i].id != 0) UnloadTexture(lorainePoses[i]);
        if (ronzerPoses[i].id != 0)  UnloadTexture(ronzerPoses[i]);
        if (markPoses[i].id != 0)    UnloadTexture(markPoses[i]);
        if (karenPoses[i].id != 0)   UnloadTexture(karenPoses[i]);
    }
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = -1.0f;
}

void EndingScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void EndingScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool EndingScene::isPlayingScenario() const {
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void EndingScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[5] = {tom, loraine, ronzer, mark, karen};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 5);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void EndingScene::playLine(const EndingLine& line) {
    dialog->setCharacter(line.speaker, line.emotion, line.overrideName);
    dialog->setText(line.text);
    dialog->setTextColor(line.textColor);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    tomPoseEmotion = line.tomPoseEmotion;
    lorainePoseEmotion = line.lorainePoseEmotion;
    ronzerPoseEmotion = line.ronzerPoseEmotion;
    markPoseEmotion = line.markPoseEmotion;
    karenPoseEmotion = line.karenPoseEmotion;

    SceneActor* actorsByIndex[5] = {tom, loraine, ronzer, mark, karen};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 5);
}

void EndingScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    dialog->clearCharacter();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void EndingScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
    currentFocusActor = actorIndex;

    Vector2 t;
    if (cameraTargetFor(actorIndex, t)) {
        if (cut) getCamera()->setPosition(t.x, t.y);
        else getCamera()->followPosition(t, 8.0f);
        getCamera()->zoomTo(3.0f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(5.0f, 0.3f);
    }
}

// Aim into the visible body from the pose top-left. Most of this cast is at
// 0.8 scale, so the aim offset is scaled to match (~205px pose vs ~256 native).
bool EndingScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    float scale = SIDE_SCALE;
    if (actorIndex == 0)      { target = tom;     scale = TOM_SCALE; }
    else if (actorIndex == 1) { target = loraine; scale = SIDE_SCALE; }
    else if (actorIndex == 2) { target = ronzer;  scale = SIDE_SCALE; }
    else if (actorIndex == 3) { target = mark;    scale = MARK_SCALE; }
    else if (actorIndex == 4) { target = karen;   scale = SIDE_SCALE; }
    if (!target) return false;

    Vector2 p = target->getPosition();
    // Smaller Y offset = camera aims HIGHER up the figure (frames the head/torso
    // instead of the feet). 240/280/340 all sat too low (feet-centered, actor
    // cut off at the waist); 170 lifts the frame up onto the body.
    out = { p.x + 160.0f * scale, p.y + 170.0f * scale };
    return true;
}

// Draw a pose top-left at pos, native size * scale, flipX via negative source
// width -- same convention as the editor and every other scene.
static void drawEndingPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

// flipX from layout.json.
void EndingScene::drawTom(Vector2 pos) {
    drawEndingPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ false, TOM_SCALE);
}
void EndingScene::drawLoraine(Vector2 pos) {
    drawEndingPose(lorainePoses[(int)lorainePoseEmotion], pos, /*flipX*/ true, SIDE_SCALE);
}
void EndingScene::drawRonzer(Vector2 pos) {
    drawEndingPose(ronzerPoses[(int)ronzerPoseEmotion], pos, /*flipX*/ false, SIDE_SCALE);
}
void EndingScene::drawMark(Vector2 pos) {
    drawEndingPose(markPoses[(int)markPoseEmotion], pos, /*flipX*/ false, MARK_SCALE);
}
void EndingScene::drawKaren(Vector2 pos) {
    drawEndingPose(karenPoses[(int)karenPoseEmotion], pos, /*flipX*/ false, SIDE_SCALE);
}
