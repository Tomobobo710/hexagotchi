#include "OfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SceneDebugCamera.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

// The portal's anchor in this scene's 2D world space (same coordinate space
// as tom/larry's positions) -- where in the room it should visually sit.
// PortalEffect's own 3D camera is otherwise entirely independent of this
// scene's 2D SceneCamera, so without projecting this point through the 2D
// camera every frame (see the exact-projection math in draw()), the portal
// would either stay glued to screen-center or drift approximately as the 2D
// camera pans/zooms during dialogue instead of staying pixel-locked to a
// fixed spot in the room.
static const Vector2 PORTAL_WORLD_2D = {60.0f, 460.0f};
// Reference 3D camera framing -- a normal, undistorted view of the model at
// its native ~1-3 unit scale (NOT solved from the 2D camera's raw zoom
// value: the model's 3D units and the 2D scene's pixel units are entirely
// different scales, so forcing "1 3D unit = 1 2D pixel" collapses fovy to
// ~180 degrees and the model to a degenerate sliver). This framing defines
// the "1x" pixels-per-3D-unit baseline (see PORTAL_BASE_PPU below); zoom
// changes scale relative to that baseline instead of in raw pixel units.
static const float PORTAL_CAM_DIST = 6.0f;
static const float PORTAL_BASE_FOVY_DEG = 45.0f;

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

    portal = new PortalEffect();
    portal->init();
    portal->setDebugCamDist(PORTAL_CAM_DIST);
    portal->setObjectScale(0.6f);
    // objectPosition/fovy are recomputed every frame in draw() from
    // PORTAL_WORLD_2D and the live 2D camera (exact projection sync, not an
    // initial placement) -- see the comment there.

    // Tom is standing at open (far-left, up high) where the scene editor
    // placed him; the scenario zooms in on him here, then walks him to
    // (451, 480). Larry starts off the bottom-right edge and walks up to Tom.
    tom = new SceneActor({113.0f, 349.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    larry = new SceneActor({1050.0f, 818.0f}, 50.0f, 76.0f);
    larry->setTag("larry");
    larry->setVisible(false);
    addActor(larry);

    // Loraine (Larry's secretary) waits off the bottom edge until Larry calls
    // her in, then walks up to the left of Tom (Tom lands at x=417).
    loraine = new SceneActor({300.0f, 818.0f}, 50.0f, 76.0f);
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
              ActorMove{0, {{417.0f, 441.0f}}, 140.0f},
              ActorMove{1, {{669.0f, 441.0f}}, 220.0f},
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
        // (x=340, left of Tom at 417) while he's still shouting.
        { CharacterId::Larry, "LORAAAINE!!!!",
          1, false, false, PortraitEmotion::Happy, "",
          /*movesAtStart*/ {
              ActorMove{2, {{340.0f, 441.0f}}, 240.0f},
          }, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Loraine, "Yes sir?",
          2, false, false, PortraitEmotion::Mid, "Loraine (the secretary)",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },

        // --- Larry asks for Tom's merge metrics; Loraine reads the stats ----
        { CharacterId::Larry, "Loraine, pull up the numbers\non Tom's latest merge.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },
        { CharacterId::Loraine, "[PLAYER STAT MESSAGE]",
          2, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid, PoseEmotion::Mid },

        // --- Comedy beat: lands on Tom coming in 15 minutes earlier ---------
        { CharacterId::Larry, "Mm. The numbers don't lie, Tom.\nAnd the numbers are... a choice.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "I'll do better next time. I just need to get a good night's sleep.",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Larry, "And that's the beauty of accountability!\nWe grow together.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Larry, "Loraine, put Tom down for the\nEarly Bird Optimization Initiative.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Tom, "The what?",
          0, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Mid, PoseEmotion::Happy, PoseEmotion::Mid },
        { CharacterId::Loraine, "It means you come in fifteen minutes\nearlier. Every day.",
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
              ActorMove{0, {{417.0f, 441.0f}}, 600.0f},
              ActorMove{1, {{669.0f, 441.0f}}, 600.0f},
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
        { CharacterId::Larry, "The winners don't take the train, Tom.\nThe winners ARE the train.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Happy },
        { CharacterId::Tom, "That doesn't mean anything.",
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

    if (portal) portal->update(deltaTime);

    bool fromDebugHub = getEntrySceneName() == "scene_select";
    if (fromDebugHub) {
        updateSceneDebugCamera(portal, getCamera(), deltaTime);

        if (portal) {
            // Numpad +/-: uniform scale. Numpad 4/6: yaw (rotate around Y).
            // PortalEffect-specific (not part of SceneEffect's generic
            // debug-camera interface), so handled directly here rather than
            // through SceneDebugCamera.hpp.
            float scale = portal->getObjectScale();
            if (IsKeyDown(KEY_KP_ADD)) scale += 0.6f * deltaTime;
            if (IsKeyDown(KEY_KP_SUBTRACT)) scale -= 0.6f * deltaTime;
            if (scale < 0.1f) scale = 0.1f;
            portal->setObjectScale(scale);

            float yaw = portal->getObjectYaw();
            if (IsKeyDown(KEY_KP_6)) yaw += 60.0f * deltaTime;
            if (IsKeyDown(KEY_KP_4)) yaw -= 60.0f * deltaTime;
            portal->setObjectYaw(yaw);
        }
    }

    if (activeScenario < 0) {
        // Ambient (pre-scenario / after it ends): Tom stands at his open spot
        // where the scenario begins, with a gentle idle bob. Not the yoga-ball
        // wobble anymore -- Scenario A opens on Tom already standing here and
        // then walks him in, so pinning him elsewhere would teleport him on
        // the first move. Camera is left alone; the scenario's first line
        // zooms in on him itself.
        tomWobbleTimer += deltaTime * 3.0f;
        tom->setPosition({113.0f, 349.0f + sinf(tomWobbleTimer) * 4.0f});
    }

    // Keep the camera locked on the speaking actor's LIVE position every
    // frame, so it tracks them smoothly as they moveTo()-walk into place
    // rather than easing once to where they stood when the line started.
    // Skipped in wide view (debug) so the numpad-0 override isn't fought.
    if (activeScenario >= 0 && currentFocusActor >= 0 && !getCamera()->isWideViewEnabled()) {
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

    // Portal/merge-machine: renders ON TOP of the background art but BEHIND
    // the actors, since the actor draws come after this. Needs its own 3D
    // mode (can't share the 2D camera), so the 2D pass is split around it.
    //
    // PortalEffect's 3D camera is otherwise completely independent of this
    // scene's 2D SceneCamera. To keep the portal pixel-locked to
    // PORTAL_WORLD_2D as the 2D camera pans/zooms during dialogue (rather
    // than approximately drifting), this derives the EXACT 3D placement/fovy
    // that reproduces the 2D camera's own projection at the portal's depth,
    // relative to PORTAL_CAM_DIST/PORTAL_BASE_FOVY_DEG's own natural
    // pixels-per-3D-unit baseline (NOT the 2D camera's raw zoom value --
    // the model's 3D units and the 2D scene's pixel units are unrelated
    // scales, so equating them directly collapses fovy to ~180 degrees):
    //
    //   2D:  screenPos = (worldPos2D - target) * zoom2D + {W/2, H/2}
    //   3D:  screenPos = objectPosition.xy * pixelsPerWorldUnit3D + {W/2, H/2}
    //        (camera at Z=camDist looking at Z=0, object sitting at
    //        Z=-camDist, i.e. exactly on the focal plane)
    //   where pixelsPerWorldUnit3D = (H/2) / (camDist * tan(fovy/2))
    //
    // pixelsPerWorldUnit3D is scaled by zoom2D relative to its baseline value
    // at PORTAL_BASE_FOVY_DEG (zoom2D == 1.0 reproduces that exact framing),
    // fovy is solved to hit that scaled value, and objectPosition.xy is
    // solved from the 2D screen position given that same value -- so the
    // portal's on-screen size and position both track the 2D camera exactly.
    if (portal) {
        float halfHeightWorldBase = PORTAL_CAM_DIST * tanf(PORTAL_BASE_FOVY_DEG * 0.5f * DEG2RAD);
        float basePixelsPerWorldUnit3D = ((float)GAME_H / 2.0f) / halfHeightWorldBase;

        float zoom2D = getCamera()->getZoom();
        float pixelsPerWorldUnit3D = basePixelsPerWorldUnit3D * zoom2D;
        float halfHeightWorld = ((float)GAME_H / 2.0f) / pixelsPerWorldUnit3D;
        float fovyDeg = 2.0f * atanf(halfHeightWorld / PORTAL_CAM_DIST) * RAD2DEG;
        portal->setDebugFovy(fovyDeg);
        portal->setDebugCamDist(PORTAL_CAM_DIST);

        Vector2 screenPos = getCamera()->worldToScreen(PORTAL_WORLD_2D);
        float objX =  (screenPos.x - (float)GAME_W / 2.0f) / pixelsPerWorldUnit3D;
        float objY = -(screenPos.y - (float)GAME_H / 2.0f) / pixelsPerWorldUnit3D;
        portal->setObjectPosition({objX, objY, -PORTAL_CAM_DIST});

        portal->drawBackground();
    }

    // Actors last, on top of the portal.
    BeginMode2D(cam);
    drawTom(tom->getPosition());
    if (activeScenario >= 0) drawLarry(larry->getPosition());
    // Loraine starts off the bottom edge (y=818, below the 720 screen) and is
    // only walked in on the "LORAAAINE!!!!" beat, so drawing her whenever the
    // scenario is live is fine -- she's simply off-screen until she strides up.
    if (activeScenario >= 0) drawLoraine(loraine->getPosition());
    EndMode2D();

    if (getEntrySceneName() == "scene_select") {
        drawSceneDebugCameraReadout(portal, 16, 16);
        if (portal) {
            const char* txt = TextFormat("scale: %.2f (Numpad +/-)   yaw: %.1f deg (Numpad 4/6)",
                portal->getObjectScale(), portal->getObjectYaw());
            DrawText(txt, 16, 60, 18, Color{255, 220, 140, 255});
        }
    }

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

    if (portal) { portal->cleanup(); delete portal; portal = nullptr; }
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
    // Aim near the TOP of the drawn pose (head height), so when the camera
    // centers here the body fills the frame downward and stays in the upper
    // area -- above the dialog box along the bottom edge -- instead of the
    // feet landing at screen-center and the figure hiding behind the box.
    // Pose is 192px tall drawn (256 native x 0.75). At tight zoom the whole
    // body won't fit; aim at the upper torso so the head/face and torso sit
    // in the visible area above the dialog box.
    out = { p.x + 64.0f, p.y + 80.0f };
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
        getCamera()->zoomTo(4.0f, 0.5f);
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
static void drawPose(Texture2D pose, Vector2 pos, bool flipX) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * OfficeScene::POSE_SCALE, pose.height * OfficeScene::POSE_SCALE };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void OfficeScene::drawTom(Vector2 pos) {
    drawPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ true);   // faces right
}

void OfficeScene::drawLarry(Vector2 pos) {
    drawPose(larryPoses[(int)larryPoseEmotion], pos, /*flipX*/ false);  // faces left
}

void OfficeScene::drawLoraine(Vector2 pos) {
    drawPose(lorainePoses[(int)lorainePoseEmotion], pos, /*flipX*/ true);  // to Tom's left, faces right toward him
}
