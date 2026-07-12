#include "TherapistOfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "AudioManager.hpp"
#include "SceneDebugCamera.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

// Top-left placement (editor convention), read well at zoom 2: Tom on the left
// facing right, Judy on the right facing left. Static -- a seated session.
static const Vector2 TOM_POS  = {126.0f, 351.0f};
static const Vector2 JUDY_POS = {913.0f, 361.0f};

// Defined in main.cpp -- true for the frame in which a click was consumed by
// the global Tom-world pause button/menu.
extern bool IsPauseUiClaimingClick();

TherapistOfficeScene::TherapistOfficeScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {202, 232, 250, 255}), dialog(sharedDialog) {
}

void TherapistOfficeScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    windowEffect = new TherapistWindowEffect();
    addEffect(windowEffect);

    tom = new SceneActor(TOM_POS, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    judy = new SceneActor(JUDY_POS, 48.0f, 72.0f);
    judy->setTag("judy");
    judy->setVisible(false);
    addActor(judy);

    background = AssetPack::loadTexture("backgrounds/therapistbg.png");

    for (int e = 0; e < 4; e++) {
        tomPoses[e]  = CharacterRegistry::loadPose(CharacterId::Tom,  (PoseEmotion)e);
        judyPoses[e] = CharacterRegistry::loadPose(CharacterId::Judy, (PoseEmotion)e);
    }

    // Ported from the JS prototype's "The Last Session" episode -- the digital-
    // pet metaphor breakdown, ending on the copay hike. NARRATOR lines have no
    // speaking actor. Modernized onto the current template (pose emotions,
    // zoom-2 camera, per-frame follow, end-fade) but the writing is unchanged.
    // JUDY IS CLUELESS, NOT MALICIOUS. She's a bad therapist -- she keeps
    // confusing Tom, reflexively reframes everything as a metaphor, misdiagnoses,
    // and "helps" with quips that don't help. She (like everyone) is also a
    // tomagotchi, but she cannot comprehend that Tom's toy-hood is literal. The
    // scene talks about the divorce for real: Tom is honest and open about
    // Ronzer (a digital MONSTER -- edgy, not one of the cute pets) and about not
    // getting what Karen sees in him. Jealousy simmers but Tom never says it
    // outright. It ends the way therapy with a clock-watcher ends: right as Tom
    // is about to actually realize something, Judy cuts him off -- "time's up,"
    // "copay expected at the front desk."
    scenarios.push_back({
        { CharacterId::Narrator, "Tom's last covered therapy session.\nHe has been saving the hard stuff for today.",
          -1, false },
        // Judy opens warm and professional; Tom already braced/uneasy.
        { CharacterId::Judy, "So Tom, how have you been\nprocessing the divorce?",
          1, false, false, PortraitEmotion::Happy, "Judy (Tom's therapist)",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "I... okay so you know how I'm\nsometimes a digital pet?",
          0, false, false, PortraitEmotion::Mid, "Tom Gatchi",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Judy, "We've talked about this metaphor.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        // Tom insists, getting agitated -- the literal reality no one believes.
        { CharacterId::Tom, "It's not a metaphor.\nSomebody watches me.\nI have to poop in front of them.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Judy, "...Tom.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Tom, "I have to perform happiness.\nOn demand.\nWhile they press little buttons at me.",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Judy, "I think that IS a metaphor Tom.\nFor work maybe? For the marriage?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },

        // Tom gives up correcting her -- deflated -- and just does the divorce.
        { CharacterId::Tom, "You know what, forget it. The divorce.\nLet's do the divorce.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Judy, "Wonderful. And when you picture Karen\nnow, what's the first feeling that comes up?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "Confused, mostly. She left me for Ronzer.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Judy, "And Ronzer is... your inner critic?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "No, Ronzer's a guy. A digital monster, technically.\nBig. Red. Has like... claws.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Judy, "Mm. And the claws represent?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        // Deadpan exasperation, not sadness -- the comedy beat.
        { CharacterId::Tom, "The claws represent that he has claws, Judy.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },

        // Tom opens up honestly -- the Ronzer stuff, jealousy under the surface.
        { CharacterId::Tom, "And here's the thing. He's not even one of\nthe CUTE ones. The nice little pets.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "He's an EDGY one. All spikes and attitude.\nA marketing category, basically.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Judy, "It sounds like Ronzer makes you feel\nsmall. Let's sit with 'small.'",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        // Flustered/defensive -- the lady doth protest.
        { CharacterId::Tom, "I don't feel small. I just don't GET it.\nWhat does a guy like that even offer?",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Tom, "His whole deal is being popular with,\nlike, twelve-year-old boys. That's the demographic.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Judy, "And you were never popular with\ntwelve-year-old boys?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        // Caps WANT -- flustered outrage.
        { CharacterId::Tom, "That's not -- I don't WANT to be popular\nwith twelve-year-old boys, Judy.",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Tom, "I just don't understand what Karen sees.\nHe evolves into something with a chainsaw.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        // Judy's useless bright quip -- false cheer at its peak.
        { CharacterId::Judy, "Have you considered that YOU could\nevolve? Have you tried journaling?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "I can't evolve, Judy. I'm a base-tier pet.\nThat's not -- that's not how I was designed.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },

        // The near-realization -- Tom builds toward it, rising intensity.
        { CharacterId::Tom, "...Maybe that's it, though. Maybe Karen wanted\nsomething that could become more, and I just...",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Tom, "I just stay the same. I get fed and I get\nwatched and I never turn into anything, and --",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        // The cruel cutoff -- Judy falsely bright, oblivious to the breakthrough.
        { CharacterId::Judy, "And -- oh! That's our time.",
          1, true, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy },
        { CharacterId::Tom, "Wait -- no, I think I was actually\ngetting somewhere just then.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Judy, "Big breakthroughs next week!\nCopay's expected at the front desk.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Judy, "Which, reminder, has increased\nto $200 starting today.",
          1, true, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        // Beaten, deadpan-defeated final button.
        { CharacterId::Tom, "Of course it has.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad },
    });
}

void TherapistOfficeScene::update(float deltaTime) {
    Scene::update(deltaTime);
    if (isPaused()) return;

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (windowEffect) windowEffect->update(deltaTime);

    if (getEntrySceneName() == "scene_select") {
        updateSceneDebugCamera(windowEffect, getCamera(), deltaTime);

        if (windowEffect) {
            // TAB cycles which origin the I/J/K/L/U/O controls below move --
            // backdrop/road/tree each need independent placement (see
            // TherapistWindowEffect.hpp), previously only backdrop was reachable.
            if (IsKeyPressed(KEY_TAB)) debugOriginTarget = (debugOriginTarget + 1) % 3;

            // Dial-in controls for whichever origin is selected -- I/K: Y, J/L: X,
            // U/O: Z. Temporary, for finding placement to bake into the scene.
            Vector3 origin = debugOriginTarget == 0 ? windowEffect->getBackdropOrigin()
                            : debugOriginTarget == 1 ? windowEffect->getRoadOrigin()
                                                      : windowEffect->getTreeOrigin();
            float moveSpeed = 4.0f * deltaTime;
            if (IsKeyDown(KEY_K)) origin.y -= moveSpeed;
            if (IsKeyDown(KEY_I)) origin.y += moveSpeed;
            if (IsKeyDown(KEY_J)) origin.x -= moveSpeed;
            if (IsKeyDown(KEY_L)) origin.x += moveSpeed;
            if (IsKeyDown(KEY_U)) origin.z -= moveSpeed;
            if (IsKeyDown(KEY_O)) origin.z += moveSpeed;
            if (debugOriginTarget == 0) windowEffect->setBackdropOrigin(origin);
            else if (debugOriginTarget == 1) windowEffect->setRoadOrigin(origin);
            else windowEffect->setTreeOrigin(origin);
        }
    }

    if (activeScenario < 0) {
        // Ambient: both sitting, small idle motion -- Tom fidgets slightly more
        // than Judy, who stays composed/still.
        tomFidgetTimer += deltaTime * 2.5f;
        judyNodTimer += deltaTime * 0.8f;

        tom->setPosition({TOM_POS.x, TOM_POS.y + sinf(tomFidgetTimer) * 3.0f});
        judy->setPosition({JUDY_POS.x, JUDY_POS.y + sinf(judyNodTimer) * 1.5f});

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
            bool manualAdvance = ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE) ||
                                        (ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT) && !IsPauseUiClaimingClick()));
            if (manualAdvance) AudioManager::Get().playClick();  // no click on auto-advance
            if (dialog->consumeAutoAdvance() || manualAdvance) {
                advanceLine();
            }
        }
    }
}

void TherapistOfficeScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
    else drawOffice();

    // Stop drawing actors once the end fade is ramping (same as OfficeScene).
    if (endElapsed < 0.0f) {
        drawTom(tom->getPosition());
        drawJudy(judy->getPosition());
    }
    EndMode2D();

    // y=40, not 16 -- avoids overlapping other on-screen debug text.
    if (getEntrySceneName() == "scene_select") {
        drawSceneDebugCameraReadout(windowEffect, 16, 40);

        if (windowEffect) {
            const char* label = debugOriginTarget == 0 ? "backdropOrigin"
                               : debugOriginTarget == 1 ? "roadOrigin" : "treeOrigin";
            Vector3 origin = debugOriginTarget == 0 ? windowEffect->getBackdropOrigin()
                            : debugOriginTarget == 1 ? windowEffect->getRoadOrigin()
                                                      : windowEffect->getTreeOrigin();
            const char* txt = TextFormat("%s: (%.2f, %.2f, %.2f)  I/K: Y  J/L: X  U/O: Z  (TAB to cycle)",
                label, origin.x, origin.y, origin.z);
            DrawRectangle(10, 84, MeasureText(txt, 18) + 12, 26, Color{0, 0, 0, 160});
            DrawText(txt, 16, 88, 18, Color{140, 255, 160, 255});
        }
    }

    if (endElapsed >= 0.0f) {
        float t = endElapsed / END_FADE_DURATION;
        if (t > 1.0f) t = 1.0f;
        unsigned char alpha = (unsigned char)(t * 255.0f);
        DrawRectangle(0, 0, (int)getWidth(), (int)getHeight(), Color{0, 0, 0, alpha});
    }
}

void TherapistOfficeScene::cleanup() {
    Scene::cleanup();
    windowEffect = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 4; i++) {
        if (tomPoses[i].id != 0)  UnloadTexture(tomPoses[i]);
        if (judyPoses[i].id != 0) UnloadTexture(judyPoses[i]);
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

void TherapistOfficeScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void TherapistOfficeScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool TherapistOfficeScene::isPlayingScenario() const {
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void TherapistOfficeScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[2] = {tom, judy};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 2);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void TherapistOfficeScene::playLine(const TherapistLine& line) {
    dialog->setCharacter(line.speaker, line.emotion, line.firstTimeName);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    tomPoseEmotion = line.tomPoseEmotion;
    judyPoseEmotion = line.judyPoseEmotion;

    SceneActor* actorsByIndex[2] = {tom, judy};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 2);
}

void TherapistOfficeScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void TherapistOfficeScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
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
// top-left. Returns false for Narrator / actorIndex -1 so the camera holds.
bool TherapistOfficeScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = judy;
    if (!target) return false;

    Vector2 p = target->getPosition();
    out = { p.x + 160.0f, p.y + 210.0f };
    return true;
}

// Draw a pose top-left at pos, native size * scale, flipX via negative source
// width -- exactly like the editor and the other scenes.
static void drawTherapistPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void TherapistOfficeScene::drawTom(Vector2 pos) {
    drawTherapistPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ true, 1.0f);   // on the left, faces right toward Judy
}

void TherapistOfficeScene::drawJudy(Vector2 pos) {
    drawTherapistPose(judyPoses[(int)judyPoseEmotion], pos, /*flipX*/ false, 1.0f);  // on the right, faces left toward Tom
}

// --- Set dressing (fallback only, if therapistbg.png is missing) ----------
void TherapistOfficeScene::drawOffice() {
    // Bookshelf behind Judy
    Color shelfWood = {90, 65, 45, 255};
    DrawRectangle(700, 100, 220, 260, shelfWood);
    Color shelfDark = {60, 42, 30, 255};
    for (int y = 130; y < 360; y += 65) {
        DrawRectangle(700, y, 220, 6, shelfDark);
    }
    Color bookColors[] = {{160, 60, 60, 255}, {60, 90, 140, 255}, {80, 140, 90, 255}, {150, 130, 60, 255}};
    for (int i = 0; i < 4; i++) {
        DrawRectangle(715 + i * 45, 100, 30, 30, bookColors[i]);
    }

    Color windowFrame = {70, 60, 55, 255};
    Color windowSky = {130, 150, 170, 255};
    DrawRectangle(60, 90, 160, 150, windowFrame);
    DrawRectangle(70, 100, 140, 130, windowSky);
    DrawLine(140, 100, 140, 230, windowFrame);
    DrawLine(70, 165, 210, 165, windowFrame);

    Color table = {80, 60, 50, 255};
    DrawRectangle(495, 470, 60, 40, table);
    Color tissueBox = {230, 230, 220, 255};
    DrawRectangle(505, 455, 40, 20, tissueBox);

    Color rug = {110, 55, 55, 255};
    DrawEllipse(512, 470, 260, 60, rug);
}
