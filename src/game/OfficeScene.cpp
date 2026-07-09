#include "OfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SceneDebugCamera.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

static const Color TOM_COLOR      = CharacterRegistry::get(CharacterId::Tom).nameColor;
static const Color BOSS_COLOR     = CharacterRegistry::get(CharacterId::Boss).nameColor;
static const Color NARRATOR_COLOR = CharacterRegistry::get(CharacterId::Narrator).nameColor;

// The portal's anchor in this scene's 2D world space (same coordinate space
// as tom/boss's positions) -- where in the room it should visually sit.
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

    portal = new PortalEffect();
    portal->init();
    portal->setDebugCamDist(PORTAL_CAM_DIST);
    portal->setObjectScale(0.6f);
    // objectPosition/fovy are recomputed every frame in draw() from
    // PORTAL_WORLD_2D and the live 2D camera (exact projection sync, not an
    // initial placement) -- see the comment there.

    tom = new SceneActor({420.0f, 400.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    boss = new SceneActor({650.0f, 380.0f}, 50.0f, 76.0f);
    boss->setTag("boss");
    boss->setVisible(false);
    addActor(boss);

    // --- Scenario 0: "Performance Review", ported almost line-for-line ---
    scenarios.push_back({
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

    // --- Scenario 1 (Scenario C): "The Promotion (Sort Of)", ported almost line-for-line ---
    scenarios.push_back({
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
        // Ambient: Tom balancing/wobbling on the yoga ball, boss absent
        // (only appears during the scripted scenarios, called into "his office").
        tomWobbleTimer += deltaTime * 3.0f;
        tom->setPosition({420.0f, 400.0f + sinf(tomWobbleTimer) * 6.0f});

        if (!getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(460.0f, 320.0f);
            getCamera()->setZoom(1.0f);
        }
    }

    if (activeScenario >= 0 && dialog->isVisible() && dialog->isFinished()) {
        SceneInputHandler* ih = getInputHandler();
        if (dialog->consumeAutoAdvance() || (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE)))) {
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
    if (activeScenario >= 0) drawBoss(boss->getPosition());
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
    endElapsed = -1.0f;

    if (background.id != 0) { UnloadTexture(background); background = {0}; }

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

    lineIndex++;
    auto& seq = scenarios[activeScenario];
    if (lineIndex >= (int)seq.size()) {
        endScenario();
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

void OfficeScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    endElapsed = 0.0f;
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
