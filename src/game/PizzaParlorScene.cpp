#include "PizzaParlorScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "CharacterRegistry.hpp"
#include "SceneDebugCamera.hpp"
#include <cstdlib>
#include <cmath>

// Ambient-only lines the Pokemon shouts unprompted, popped through the same
// shared DialogBox as scripted scenarios. Its entire personality is its own name.
static const char* POKEMON_QUIPS[] = {
    "Ronzer!",
    "Ronzer Ronzer!",
    "RONZER.",
    "Ronzer?",
    "Ron... zer!",
};
static const float POKEMON_QUIP_DURATION = 1.8f;

// Spawn/idle positions from the scene editor's layout.json. Tom starts off the
// left edge and walks in; Karen and Ronzer are static at their table. flipX is
// hardcoded per-actor in the draw fns (Tom faces right, the other two left).
static const Vector2 TOM_SPAWN    = {-134.0f, 322.0f};
static const Vector2 TOM_WALK_TO  = { 205.0f, 317.0f};
static const Vector2 KAREN_POS    = { 700.0f, 298.0f};
static const Vector2 RONZER_POS   = { 850.0f, 343.0f};

PizzaParlorScene::PizzaParlorScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {225, 120, 60, 255}), dialog(sharedDialog) {
}

void PizzaParlorScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    sunEffect = new SunEffect();
    addEffect(sunEffect);

    // Actors are invisible bounds/position holders -- drawTom/drawKaren/
    // drawRonzer render the poses on top, keyed by tag. Position is the pose's
    // TOP-LEFT (editor convention), same as OfficeScene/ApartmentScene.
    tom = new SceneActor(TOM_SPAWN, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    karen = new SceneActor(KAREN_POS, 48.0f, 72.0f);
    karen->setTag("karen");
    karen->setVisible(false);
    addActor(karen);

    ronzer = new SceneActor(RONZER_POS, 40.0f, 40.0f);
    ronzer->setTag("ronzer");
    ronzer->setVisible(false);
    addActor(ronzer);

    nextQuipTime = 3.0f + (float)(rand() % 400) / 100.0f;

    background = AssetPack::loadTexture("backgrounds/parlorbg.png");

    // Full-body pose art, all 4 emotions per actor (incl. Scared), indexed by
    // PoseEmotion.
    for (int e = 0; e < 4; e++) {
        tomPoses[e]    = CharacterRegistry::loadPose(CharacterId::Tom,    (PoseEmotion)e);
        karenPoses[e]  = CharacterRegistry::loadPose(CharacterId::Karen,  (PoseEmotion)e);
        ronzerPoses[e] = CharacterRegistry::loadPose(CharacterId::Ronzer, (PoseEmotion)e);
    }

    // --- Scenario 0: the ex-wife needling Tom while Ronzer heckles from the sideline ---
    // Tom walks in from off the left edge on his first line; Karen and Ronzer
    // are already seated. Camera smooth by default; cutCamera=true for beats.
    scenarios.push_back({
        { CharacterId::Tom,    "Oh -- hey. I didn't know you two ate here too.",
          0, false, false, PortraitEmotion::Mid, "Tom Gatchi",
          /*movesAtStart*/ { ActorMove{0, {TOM_WALK_TO}, 160.0f} }, {},
          PoseEmotion::Mid, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Karen,  "We come here every Tuesday, Tom. We've come here every\nTuesday for six weeks.",
          1, false, false, PortraitEmotion::Mid, "Karen (Tom's ex-wife)",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Ronzer, "Ronzer.",
          2, false, false, PortraitEmotion::Happy, "Ronzer (her new boyfriend)",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Tom,    "Right. Tuesdays. I knew that.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Karen,  "Did you get my email about Jimmy's recital?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Tom,    "I -- yeah, I'm looking into it. Work's been --",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Karen,  "You're a digital pet, Tom. What work.",
          1, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Ronzer, "RONZER RONZER RONZER.",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom,    "...I have a job. I make kids happy...",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Karen,  "Ronzer got a promotion at the gym he doesn't even work at.\nThey just gave it to him.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Ronzer, "Ronzer.",
          2, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Tom,    "That's not -- that isn't how promotions --",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Karen,  "Get the pizza to go next time, Tom. It's for the kids.",
          1, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom,    "It's always been for the kids.",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Happy },
    });
}

void PizzaParlorScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (sunEffect && getEntrySceneName() == "scene_select") {
        updateSceneDebugCamera(sunEffect, getCamera(), deltaTime);

        // Dial-in controls, targeting the pine tree's position -- I/K: Y,
        // J/L: X, U/O: Z. Temporary, for finding placement to bake into the
        // scene. Gated to the scene_select debug hub so it doesn't eat
        // I/J/K/L/U/O during real sequencer-driven playthroughs.
        Vector3 origin = sunEffect->getTreeOrigin();
        float moveSpeed = 4.0f * deltaTime;
        if (IsKeyDown(KEY_K)) origin.y -= moveSpeed;
        if (IsKeyDown(KEY_I)) origin.y += moveSpeed;
        if (IsKeyDown(KEY_J)) origin.x -= moveSpeed;
        if (IsKeyDown(KEY_L)) origin.x += moveSpeed;
        if (IsKeyDown(KEY_U)) origin.z -= moveSpeed;
        if (IsKeyDown(KEY_O)) origin.z += moveSpeed;
        sunEffect->setTreeOrigin(origin);
    }

    if (activeScenario < 0) {
        // --- Ambient idle: gentle bob/sway per character, slow camera drift ---
        tomBobTimer += deltaTime * 2.0f;
        karenTapTimer += deltaTime * 3.0f;
        ronzerHopTimer += deltaTime * 4.0f;

        tom->setPosition({TOM_SPAWN.x, TOM_SPAWN.y + sinf(tomBobTimer) * 4.0f});
        karen->setPosition({KAREN_POS.x, KAREN_POS.y + sinf(karenTapTimer) * 2.0f});
        ronzer->setPosition({RONZER_POS.x, RONZER_POS.y - fabsf(sinf(ronzerHopTimer)) * 14.0f});

        // Ambient quip popup, shown via the shared DialogBox
        if (quipTimer > 0.0f) {
            quipTimer -= deltaTime;
            if (quipTimer <= 0.0f) dialog->hide();
        } else {
            ambientTimer += deltaTime;
            if (ambientTimer >= nextQuipTime) {
                ambientTimer = 0.0f;
                nextQuipTime = 4.0f + (float)(rand() % 500) / 100.0f;

                int idx = rand() % (sizeof(POKEMON_QUIPS) / sizeof(POKEMON_QUIPS[0]));
                dialog->setCharacter(CharacterId::Ronzer, PortraitEmotion::Happy);
                dialog->setText(POKEMON_QUIPS[idx]);
                dialog->show();
                quipTimer = POKEMON_QUIP_DURATION;
            }
        }

        if (!getCamera()->isWideViewEnabled()) {
            // Slow establishing drift across the whole set
            float t = (float)GetTime() * 0.05f;
            getCamera()->setPosition(512.0f + sinf(t) * 60.0f, 360.0f);
            getCamera()->setZoom(1.0f);
        }
    }

    // Keep the camera locked on the speaking actor's LIVE position every frame,
    // so it tracks them smoothly as they moveTo()-walk into place rather than
    // easing once to where they stood when the line started. Skipped while
    // shaking (so shake offset isn't lerped out) and in wide view (debug).
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
            if (dialog->consumeAutoAdvance() || (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE)))) {
                advanceLine();
            }
        }
    }
}

void PizzaParlorScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);

    // Once the scenario is ending (black fade ramping), stop drawing actors so
    // none show under/after the fade -- the scene reads as fully cleared before
    // it hands off.
    bool ending = endElapsed >= 0.0f;
    if (!ending) {
        drawTom(tom->getPosition());
        drawKaren(karen->getPosition());
        drawRonzer(ronzer->getPosition());
    }
    EndMode2D();

    if (getEntrySceneName() == "scene_select") {
        drawSceneDebugCameraReadout(sunEffect, 16, 40);

        if (sunEffect) {
            Vector3 origin = sunEffect->getTreeOrigin();
            const char* txt = TextFormat("treeOrigin: (%.2f, %.2f, %.2f)  I/K: Y  J/L: X  U/O: Z",
                origin.x, origin.y, origin.z);
            DrawRectangle(10, 84, MeasureText(txt, 18) + 12, 26, Color{0, 0, 0, 160});
            DrawText(txt, 16, 88, 18, Color{140, 255, 160, 255});
        }
    }

    // Starts ramping immediately when endScenario() fires (endElapsed jumps to
    // 0.0f right then) and climbs continuously to fully opaque over
    // END_FADE_DURATION seconds. isPlayingScenario() stays true for that whole
    // span, so StorySequencer doesn't start its own scene-switch FADE until
    // this screen is already fully black.
    if (endElapsed >= 0.0f) {
        float t = endElapsed / END_FADE_DURATION;
        if (t > 1.0f) t = 1.0f;
        unsigned char alpha = (unsigned char)(t * 255.0f);
        DrawRectangle(0, 0, (int)getWidth(), (int)getHeight(), Color{0, 0, 0, alpha});
    }
}

void PizzaParlorScene::cleanup() {
    Scene::cleanup();
    sunEffect = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int e = 0; e < 4; e++) {
        if (tomPoses[e].id != 0)    UnloadTexture(tomPoses[e]);
        if (karenPoses[e].id != 0)  UnloadTexture(karenPoses[e]);
        if (ronzerPoses[e].id != 0) UnloadTexture(ronzerPoses[e]);
    }

    // init() re-runs on every re-entry to this scene (SceneManager re-inits
    // scenes on switch) and unconditionally push_back()s the scenario table --
    // without resetting these, scenarios accumulates duplicates and
    // activeScenario can be left >= 0 if the player left mid-scenario,
    // permanently blocking triggerScenario()'s guard on every future visit.
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = -1.0f;
}

void PizzaParlorScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    quipTimer = 0.0f;
    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void PizzaParlorScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool PizzaParlorScene::isPlayingScenario() const {
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void PizzaParlorScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[3] = {tom, karen, ronzer};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 3);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void PizzaParlorScene::playLine(const PizzaLine& line) {
    dialog->setCharacter(line.speaker, line.emotion, line.firstTimeName);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    // Pose emotions persist between lines -- each line carries the full set, so
    // a line just restates whatever should currently be showing.
    tomPoseEmotion = line.tomPoseEmotion;
    karenPoseEmotion = line.karenPoseEmotion;
    ronzerPoseEmotion = line.ronzerPoseEmotion;

    SceneActor* actorsByIndex[3] = {tom, karen, ronzer};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 3);
}

void PizzaParlorScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    dialog->clearCharacter();
    getCamera()->zoomTo(1.0f, 0.6f);
}

// The point the camera aims at for a given actor, from its LIVE position.
// Actor position is the pose's top-left (editor convention), so offset into the
// visible body rather than using getCenter(). Returns false for Narrator /
// actorIndex -1 so the caller leaves the camera where it is. Same recipe as
// OfficeScene: zoom 2, aim {p.x+160, p.y+240}.
bool PizzaParlorScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = karen;
    else if (actorIndex == 2) target = ronzer;
    if (!target) return false;

    Vector2 p = target->getPosition();
    out = { p.x + 160.0f, p.y + 240.0f };
    return true;
}

void PizzaParlorScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
    currentFocusActor = actorIndex;  // update() keeps following this every frame

    Vector2 t;
    if (cameraTargetFor(actorIndex, t)) {
        // Smooth ease (the normal look) unless the line asks for a hard cut for
        // a dramatic beat. Either way update()'s per-frame follow takes over
        // from here to track the actor as they walk.
        if (cut) getCamera()->setPosition(t.x, t.y);
        else getCamera()->followPosition(t, 8.0f);
        getCamera()->zoomTo(2.0f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(5.0f, 0.3f);
    }
}

// Draw a pose EXACTLY the way tools/scene_editor.cpp draws it: texture top-left
// pinned at the actor's position, native size * scale, origin {0,0}. flipX
// mirrors horizontally (negative source width). Per-actor scale/flip hardcoded
// here, matching the editor's layout.json (all three at scale 1).
static void drawPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void PizzaParlorScene::drawTom(Vector2 pos) {
    drawPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ true, 1.0f);   // faces right
}

void PizzaParlorScene::drawKaren(Vector2 pos) {
    drawPose(karenPoses[(int)karenPoseEmotion], pos, /*flipX*/ false, 1.0f);  // faces left
}

void PizzaParlorScene::drawRonzer(Vector2 pos) {
    drawPose(ronzerPoses[(int)ronzerPoseEmotion], pos, /*flipX*/ false, 1.0f);  // faces left
}
