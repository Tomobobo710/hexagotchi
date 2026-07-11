#include "OfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

OfficeScene::OfficeScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {20, 22, 28, 255}), dialog(sharedDialog) {
}

void OfficeScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    background = AssetPack::loadTexture("backgrounds/officebg.png");

    for (int e = 0; e < 4; e++) {
        tomPoses[e]     = CharacterRegistry::loadPose(CharacterId::Tom,     (PoseEmotion)e);
        larryPoses[e]   = CharacterRegistry::loadPose(CharacterId::Larry,   (PoseEmotion)e);
        lorainePoses[e] = CharacterRegistry::loadPose(CharacterId::Loraine, (PoseEmotion)e);
    }

    // Positions from the scene editor's layout.json. Tom stands at open
    // (left), the scenario zooms on him then walks him to (546,450). Larry
    // starts off the bottom-right and walks up to Tom. Loraine waits off the
    // bottom until Larry calls her, then walks up to Tom's left.
    tom = new SceneActor({90.0f, 306.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    larry = new SceneActor({1175.0f, 861.0f}, 50.0f, 76.0f);
    larry->setTag("larry");
    larry->setVisible(false);
    addActor(larry);

    loraine = new SceneActor({170.0f, 919.0f}, 50.0f, 76.0f);
    loraine->setTag("loraine");
    loraine->setVisible(false);
    addActor(loraine);

    // --- Scenario 0 (Scenario A): the first merge into Tom's world ---------
    // Player merges in, Tom's already standing there feeling sick from it,
    // Larry strides up to greet him. Camera is smooth (followPosition) by
    // default; cutCamera=true is reserved for dramatic beats.
    scenarios.push_back({
        { CharacterId::Tom, "Ugh, that always makes me nauseous.",
          0, false, false, PortraitEmotion::Sad, "Tom Gatchi",
          {}, {}, PoseEmotion::Sad },

        // Larry's greeting: fire BOTH walks the instant this line starts, so
        // they stroll into position while he's talking.
        { CharacterId::Larry, "Tom A. Gotchi!\nJust the man I was looking for!",
          1, false, false, PortraitEmotion::Mid, "Larry (Tom's boss)",
          /*movesAtStart*/ {
              ActorMove{0, {{371.0f, 309.0f}}, 140.0f},
              ActorMove{1, {{581.0f, 304.0f}}, 260.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Mid },

        { CharacterId::Larry, "How was the merge?\nDid you remember the three E's?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "Engagement. Engagement.\nEngagement.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        // Larry lights up here -- pose flips Mid -> Happy for the rest of A.
        { CharacterId::Larry, "Exactly! Did the kid engage?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "I don't know.\nI can't perform right now.\nSo much in my head.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },

        // Larry bellows for his secretary; fire her walk-in the instant he
        // starts yelling so she strides up from the bottom to Tom's left
        // while he's still shouting.
        { CharacterId::Larry, "LORAAAINE!!!!",
          1, true, false, PortraitEmotion::Happy, "",
          /*movesAtStart*/ {
              ActorMove{2, {{206.0f, 342.0f}}, 280.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Loraine, "Yes sir?",
          2, false, false, PortraitEmotion::Mid, "Loraine (the secretary)",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Happy },

        // --- Larry asks for Tom's merge metrics; Loraine reads the stats ----
        { CharacterId::Larry, "Loraine, pull up the numbers\non Tom's latest merge.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },
        { CharacterId::Loraine, "[PLAYER STAT MESSAGE]",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Happy },

        // --- Comedy beat: lands on Tom coming in 15 minutes earlier ---------
        { CharacterId::Larry, "Mm. The numbers don't lie, Tom.\nAnd the numbers are...\nHard to read...",
          1, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "I'll do better next time. I just need to get a good night's sleep.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "And that's the beauty of accountability!\nWe grow together.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Larry, "Loraine, put Tom down for the\nEarly Bird Optimization Initiative.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        { CharacterId::Tom, "The what?",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Happy },
        // Loraine drops from Happy to Mid as she delivers the bad news.
        { CharacterId::Loraine, "EBOI. It just means you come in fifteen\nminutes earlier. Every day.",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "Fifteen minutes earlier.\nForever?",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Larry, "That's the spirit!\nWelcome to the winning team, Tom.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "...I need to sit down.\nWe don't even have chairs here.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
    });

    // --- Scenario 1 (Scenario B): Tom's two minutes late -------------------
    // A quick back-and-forth: Tom slinks in apologetic, Larry hammers him and
    // shoves him back out to "make the kid engage." Ends with Tom heading
    // back to merge out. Larry oblivious-Happy throughout, Tom sad/scared.
    scenarios.push_back({
        // Place both at their talking spots the instant the scenario opens.
        { CharacterId::Tom, "Sorry! Sorry. I'm so sorry.\nI'm two minutes late, I know.",
          0, false, false, PortraitEmotion::Sad, "",
          /*movesAtStart*/ {
              ActorMove{0, {{371.0f, 309.0f}}, 900.0f},
              ActorMove{1, {{581.0f, 304.0f}}, 900.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "Two minutes, Tom.\nDo you know what two minutes IS?",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "...A hundred and twenty seconds?",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "It's a MINDSET, Tom.\nIt's a hundred and twenty seconds of\nnot being a team player.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "The train was --",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "Winners don't take the train, Tom.\nThe winners ARE the train.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "That doesn't make any sense.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Larry, "Now get out there and make that\nkid ENGAGE. Engagement, Tom!",
          1, true, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Happy },
        { CharacterId::Tom, "...Right. Engaging.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Narrator, "Tom goes back out there.",
          -1, false },
    });
}

void OfficeScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (activeScenario < 0) {
        // Ambient (pre-scenario / after it ends): Tom stands at his open spot
        // where the scenario begins, with a gentle idle bob. Not the yoga-ball
        // wobble anymore -- Scenario A opens on Tom already standing here and
        // then walks him in, so pinning him elsewhere would teleport him on
        // the first move. Camera is left alone; the scenario's first line
        // zooms in on him itself.
        tomWobbleTimer += deltaTime * 3.0f;
        tom->setPosition({90.0f, 306.0f + sinf(tomWobbleTimer) * 4.0f});
    }

    // Keep the camera locked on the speaking actor's LIVE position every
    // frame, so it tracks them smoothly as they moveTo()-walk into place
    // rather than easing once to where they stood when the line started.
    // Skipped in wide view (debug) so the numpad-0 override isn't fought.
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

void OfficeScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();

    // Background art first (its own 2D pass).
    BeginMode2D(cam);
    drawOffice();
    EndMode2D();

    // Actors last. Once the scenario is ending (the
    // black fade is ramping), stop drawing every actor so none of them show
    // under/after the fade -- the scene reads as fully cleared before it
    // hands off.
    bool ending = endElapsed >= 0.0f;
    BeginMode2D(cam);
    if (!ending) {
        drawTom(tom->getPosition());
        if (activeScenario >= 0) drawLarry(larry->getPosition());
        // Loraine starts off the bottom edge and is only walked in on the
        // "LORAAAINE!!!!" beat, so drawing her whenever the scenario is live is
        // fine -- she's simply off-screen until she strides up.
        if (activeScenario >= 0) drawLoraine(loraine->getPosition());
    }
    EndMode2D();

    // Starts ramping immediately when endScenario() fires (endElapsed jumps
    // to 0.0f right then) and climbs continuously to fully opaque over
    // END_FADE_DURATION seconds -- no held/dead time before it starts.
    // isPlayingScenario() (below) stays true for that whole span, so
    // StorySequencer doesn't react and start its own scene-switch FADE until
    // this screen is already fully black -- that transition's fade-in half
    // then continues seamlessly straight out of this one.
    if (endElapsed >= 0.0f) {
        float t = endElapsed / END_FADE_DURATION;
        if (t > 1.0f) t = 1.0f;
        unsigned char alpha = (unsigned char)(t * 255.0f);
        DrawRectangle(0, 0, (int)getWidth(), (int)getHeight(), Color{0, 0, 0, alpha});
    }
}

void OfficeScene::cleanup() {
    Scene::cleanup();
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the scenario table -- reset so scenarios doesn't
    // accumulate duplicates and a mid-scenario exit doesn't permanently
    // block triggerScenario().
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = -1.0f;

    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 4; i++) {
        if (tomPoses[i].id != 0) UnloadTexture(tomPoses[i]);
        if (larryPoses[i].id != 0) UnloadTexture(larryPoses[i]);
        if (lorainePoses[i].id != 0) UnloadTexture(lorainePoses[i]);
    }
}

void OfficeScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void OfficeScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool OfficeScene::isPlayingScenario() const {
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void OfficeScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[3] = {tom, larry, loraine};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 3);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void OfficeScene::playLine(const OfficeLine& line) {
    // setCharacter() naturally handles Narrator too -- no portrait art
    // registered for it, so the portrait clears on its own; only the name
    // plate and its color show.
    dialog->setCharacter(line.speaker, line.emotion, line.firstTimeName);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    // Pose emotions persist between lines -- each line carries the full set,
    // so a line just restates whatever should currently be showing.
    tomPoseEmotion = line.tomPoseEmotion;
    larryPoseEmotion = line.larryPoseEmotion;
    lorainePoseEmotion = line.lorainePoseEmotion;

    SceneActor* actorsByIndex[3] = {tom, larry, loraine};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 3);
}

void OfficeScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

// The point the camera aims at for a given actor, from its LIVE position.
// Actor position is now the pose's top-left (editor convention), so offset
// into the visible body rather than using getCenter() (which is based on the
// small 48x64 placeholder box, not the drawn pose). Returns false if there's
// no such actor (e.g. Narrator, actorIndex -1) so the caller leaves the
// camera where it is.
bool OfficeScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = larry;
    else if (actorIndex == 2) target = loraine;
    if (!target) return false;

    Vector2 p = target->getPosition();
    // Same recipe as ApartmentScene: poses draw at ~full native size, so aim
    // well right and low into the body to frame the torso (not the head) and
    // keep the figure off the frame edge.
    out = { p.x + 160.0f, p.y + 240.0f };
    return true;
}

void OfficeScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
    currentFocusActor = actorIndex;  // update() keeps following this every frame

    Vector2 t;
    if (cameraTargetFor(actorIndex, t)) {
        // Smooth ease (PizzaParlorScene's normal look) unless the line asks
        // for a hard cut for a dramatic beat. Either way update()'s per-frame
        // follow takes over from here to track the actor as they walk.
        if (cut) getCamera()->setPosition(t.x, t.y);
        else getCamera()->followPosition(t, 8.0f);
        getCamera()->zoomTo(2.0f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(5.0f, 0.3f);
    }
}

// --- Set dressing ---------------------------------------------------------
void OfficeScene::drawOffice() {
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
}

// Draw a pose EXACTLY the way tools/scene_editor.cpp draws it: texture
// top-left pinned at the actor's position, native size scaled by POSE_SCALE,
// origin {0,0}. This is the shared convention that keeps the editor and the
// game aligned -- an actor's (x,y) from the editor JSON drops straight into
// its SceneActor construction with no anchor translation. flipX mirrors
// horizontally (negative source width), same as the editor's flipX toggle.
// Per-actor scale (from the scene editor's layout.json), same as
// ApartmentScene -- pose top-left at pos, native size * scale, flipX via
// negative source width.
static void drawPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void OfficeScene::drawTom(Vector2 pos) {
    drawPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ true, 1.0f);   // faces right
}

void OfficeScene::drawLarry(Vector2 pos) {
    drawPose(larryPoses[(int)larryPoseEmotion], pos, /*flipX*/ false, 1.0f);  // faces left
}

void OfficeScene::drawLoraine(Vector2 pos) {
    drawPose(lorainePoses[(int)lorainePoseEmotion], pos, /*flipX*/ true, 1.0f);  // to Tom's left, faces right toward him
}
