#include "ApartmentScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SceneDebugCamera.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

ApartmentScene::ApartmentScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {22, 18, 26, 255}), dialog(sharedDialog) {
}

void ApartmentScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    background = AssetPack::loadTexture("backgrounds/apartmentbg.png");

    for (int e = 0; e < 4; e++) {
        tomPoses[e]  = CharacterRegistry::loadPose(CharacterId::Tom,  (PoseEmotion)e);
        markPoses[e] = CharacterRegistry::loadPose(CharacterId::Mark, (PoseEmotion)e);
    }

    cityWindow = new CityWindowEffect();
    cityWindow->setDebugPitch(-15.0f);   // city-through-window 3D camera pitch
    addEffect(cityWindow);

    // Positions from the scene editor's layout.json: Tom on the left facing
    // right (flipped), Mark at the door on the right facing left. Both static
    // -- this is a doorway confrontation, no walking.
    tom = new SceneActor({315.0f, 288.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    mark = new SceneActor({698.0f, 218.0f}, 50.0f, 76.0f);
    mark->setTag("mark");
    mark->setVisible(false);
    addActor(mark);

    // --- Scenario 0: "The Heating Situation" ------------------------------
    // Tom's heat has been out for weeks; Mark the maintenance guy is at the
    // door, deadpan and useless. It slides from the busted heat into the
    // busted toilet, then into Tom being behind on rent -- Mark stays sunny,
    // Tom sinks. Tom is pathetic, Mark is cheerfully unhelpful.
    scenarios.push_back({
        { CharacterId::Narrator, "It is 48 degrees inside Tom's apartment.\nThis is December.",
          -1, false },
        { CharacterId::Tom, "Mark. I've called you six times.\nThe heat's been out for THREE WEEKS.",
          0, false, false, PortraitEmotion::Sad, "Tom Gatchi",
          {}, {}, PoseEmotion::Sad },
        { CharacterId::Mark, "Yeah, so, the part's on order.\nNot really much I can do.",
          1, false, false, PortraitEmotion::Mid, "Mark (the maintenance guy)",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "I can SEE MY BREATH.\nIn my BEDROOM.",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Mark, "Have you tried layering up?",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "I sleep in three sweaters, Mark.\nTHREE SWEATERS.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Mark, "See, that's resourceful.\nThat's problem solving.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },

        // Slide into the toilet.
        { CharacterId::Tom, "The toilet won't flush either.\nIt just... makes a noise now.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Mark, "Oh yeah, the noise. That's normal.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "It is NOT normal, Mark.\nIt sounds like it's in pain.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Mark, "Honestly? Landlord can't authorize\nany of this till the account's current.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },

        // The rent turn.
        { CharacterId::Tom, "Right. The rent. I'll have it soon.\nI'm getting money together.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Mark, "You're two weeks late.\nOn your second month.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "That's... a technicality.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Mark, "Hey, don't shoot the messanger.\nI don't make the rules.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Tom, "I am paying $1,400 a month.\nFOR AN ICE CUBE.",
          0, true, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Mark, "Fourteen hundred's a great rate\nfor the neighborhood, though.",
          1, false, false, PortraitEmotion::Happy, "",
          {}, {}, PoseEmotion::Scared, PoseEmotion::Mid },
        { CharacterId::Mark, "Anyway. Get the account current,\nI'll put a ticket in on the heat.",
          1, false, false, PortraitEmotion::Mid, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
        { CharacterId::Narrator, "Mark says he'll look into it.\nTom knows Mark will not look into it.",
          -1, false },
        { CharacterId::Tom, "...Can you at least close the door?\nYou're letting the cold in.",
          0, false, false, PortraitEmotion::Sad, "",
          {}, {}, PoseEmotion::Sad, PoseEmotion::Mid },
    });
}

void ApartmentScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (endElapsed >= 0.0f) {
        endElapsed += deltaTime;
        if (endElapsed > END_FADE_DURATION) endElapsed = END_FADE_DURATION;
    }

    if (getEntrySceneName() == "scene_select") updateSceneDebugCamera(cityWindow, getCamera(), deltaTime);

    if (cityWindow && cityWindow->consumeTrainShakeRequest()) {
        getCamera()->shake(6.0f, cityWindow->getShakeDuration());
    }

    if (activeScenario < 0) {
        // Ambient: Tom slumped at his editor spot with a slow sway, Mark
        // absent (only appears during the scripted scenario).
        tomSlumpTimer += deltaTime * 1.2f;
        tom->setPosition({315.0f, 288.0f + sinf(tomSlumpTimer) * 3.0f});

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

void ApartmentScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) {
        DrawTexture(background, 0, 0, WHITE);
    } else {
        drawApartment();
    }
    // Stop drawing actors once the end fade is ramping, so none show under or
    // after the fade (same as OfficeScene).
    if (endElapsed < 0.0f) {
        drawTom(tom->getPosition());
        if (activeScenario >= 0) drawMark(mark->getPosition());
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

void ApartmentScene::cleanup() {
    Scene::cleanup();
    cityWindow = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 4; i++) {
        if (tomPoses[i].id != 0) UnloadTexture(tomPoses[i]);
        if (markPoses[i].id != 0) UnloadTexture(markPoses[i]);
    }
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = -1.0f;
}

void ApartmentScene::triggerScenario(int index) {
    if (activeScenario >= 0) return;
    if (index < 0 || index >= (int)scenarios.size()) return;

    activeScenario = index;
    lineIndex = 0;
    dialog->setAutoContinueEnabled(true);
    playLine(scenarios[activeScenario][lineIndex]);
}

void ApartmentScene::triggerStoryEvent(int scenarioIndex) {
    triggerScenario(scenarioIndex);
}

bool ApartmentScene::isPlayingScenario() const {
    return activeScenario >= 0 || (endElapsed >= 0.0f && endElapsed < END_FADE_DURATION);
}

void ApartmentScene::advanceLine() {
    if (activeScenario < 0) return;

    auto& seq = scenarios[activeScenario];
    SceneActor* actorsByIndex[2] = {tom, mark};
    triggerActorMoves(seq[lineIndex].movesAtEnd, actorsByIndex, 2);

    lineIndex++;
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void ApartmentScene::playLine(const ApartmentLine& line) {
    dialog->setCharacter(line.speaker, line.emotion, line.firstTimeName);
    dialog->setText(line.text);
    dialog->show();
    focusCameraOn(line.focusActor, line.shake, line.cutCamera);

    tomPoseEmotion = line.tomPoseEmotion;
    markPoseEmotion = line.markPoseEmotion;

    SceneActor* actorsByIndex[2] = {tom, mark};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 2);
}

void ApartmentScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    currentFocusActor = -1;
    endElapsed = 0.0f;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void ApartmentScene::focusCameraOn(int actorIndex, bool shake, bool cut) {
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

// See OfficeScene::cameraTargetFor -- same recipe: aim into the visible body
// from the pose's top-left, poses are 0.5x native (256 -> 128px tall).
bool ApartmentScene::cameraTargetFor(int actorIndex, Vector2& out) const {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = mark;
    if (!target) return false;

    Vector2 p = target->getPosition();
    // Offsets are large because poses draw at ~1.0-1.3x here (256px native ->
    // ~256-330px tall). Aim right and low into the body so the camera frames
    // the torso/feet, not the head, and doesn't shove them to the edge.
    out = { p.x + 160.0f, p.y + 210.0f };
    return true;
}

// --- Set dressing ---------------------------------------------------------
// Per-actor scale (from the scene editor's layout.json) rather than one
// scene-wide POSE_SCALE -- Tom and Mark are placed at different scales here.
static void drawApartmentPose(Texture2D pose, Vector2 pos, bool flipX, float scale) {
    if (pose.id == 0) return;
    Rectangle src = { 0.0f, 0.0f, (flipX ? -1.0f : 1.0f) * (float)pose.width, (float)pose.height };
    Rectangle dest = { pos.x, pos.y, pose.width * scale, pose.height * scale };
    DrawTexturePro(pose, src, dest, {0.0f, 0.0f}, 0.0f, WHITE);
}

void ApartmentScene::drawTom(Vector2 pos) {
    drawApartmentPose(tomPoses[(int)tomPoseEmotion], pos, /*flipX*/ true, 1.0f);   // faces right toward Mark
}

void ApartmentScene::drawMark(Vector2 pos) {
    drawApartmentPose(markPoses[(int)markPoseEmotion], pos, /*flipX*/ false, 1.3f);  // at the door, faces left
}

void ApartmentScene::drawApartment() {
    // Fallback vector set-dressing, only used if apartmentbg.png is missing.
    Color bedFrame = {90, 70, 50, 255};
    Color bedSheet = {70, 90, 110, 255};
    DrawRectangle(80, 340, 220, 120, bedFrame);
    DrawRectangle(90, 330, 200, 40, bedSheet);

    Color windowFrame = {60, 50, 60, 255};
    Color windowSky = {40, 55, 80, 255};
    DrawRectangle(720, 100, 180, 160, windowFrame);
    DrawRectangle(730, 110, 160, 140, windowSky);
}

