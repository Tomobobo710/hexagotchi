#include "TherapistOfficeScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include "SceneDebugCamera.hpp"
#include "CharacterRegistry.hpp"
#include <cmath>

static const Color TOM_COLOR      = CharacterRegistry::get(CharacterId::Tom).nameColor;
static const Color JUDY_COLOR     = CharacterRegistry::get(CharacterId::Judy).nameColor;
static const Color NARRATOR_COLOR = CharacterRegistry::get(CharacterId::Narrator).nameColor;

TherapistOfficeScene::TherapistOfficeScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {202, 232, 250, 255}), dialog(sharedDialog) {
}

void TherapistOfficeScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    windowEffect = new TherapistWindowEffect();
    addEffect(windowEffect);

    // Tom at 1/8 of the scene's width, Judy at 7/8 -- scene is 1280 wide.
    tom = new SceneActor({160.0f, 400.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    judy = new SceneActor({1120.0f, 390.0f}, 48.0f, 72.0f);
    judy->setTag("judy");
    judy->setVisible(false);
    addActor(judy);

    background = AssetPack::loadTexture("backgrounds/therapistbg.png");

    tomPoses[0] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Sad);
    tomPoses[1] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Mid);
    tomPoses[2] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Happy);
    judyPoses[0] = CharacterRegistry::loadPose(CharacterId::Judy, PoseEmotion::Sad);
    judyPoses[1] = CharacterRegistry::loadPose(CharacterId::Judy, PoseEmotion::Mid);
    judyPoses[2] = CharacterRegistry::loadPose(CharacterId::Judy, PoseEmotion::Happy);

    tomPortraits[0] = CharacterRegistry::loadPortrait(CharacterId::Tom, PortraitEmotion::Sad);
    tomPortraits[1] = CharacterRegistry::loadPortrait(CharacterId::Tom, PortraitEmotion::Mid);
    tomPortraits[2] = CharacterRegistry::loadPortrait(CharacterId::Tom, PortraitEmotion::Happy);
    judyPortraits[0] = CharacterRegistry::loadPortrait(CharacterId::Judy, PortraitEmotion::Sad);
    judyPortraits[1] = CharacterRegistry::loadPortrait(CharacterId::Judy, PortraitEmotion::Mid);
    judyPortraits[2] = CharacterRegistry::loadPortrait(CharacterId::Judy, PortraitEmotion::Happy);

    // Ported from the JS prototype's "The Last Session" episode almost
    // line-for-line -- the digital-pet metaphor breakdown, ending on the
    // copay hike. NARRATOR lines have no speaking actor, matching how the
    // apartment scene handles them.
    scenarios.push_back({
        { "Narrator",  "Tom's last covered therapy session.\nHe has been saving the hard stuff for today.",
          NARRATOR_COLOR, -1, false },
        { "Judy", "So Tom, how have you been\nprocessing the divorce?",
          JUDY_COLOR, 1, false },
        { "Tom",      "I... okay so you know how I'm\nsometimes a digital pet?",
          TOM_COLOR, 0, false },
        { "Judy", "We've talked about this metaphor.",
          JUDY_COLOR, 1, false },
        { "Tom",      "It's not a metaphor.\nSomebody watches me.\nI have to poop in front of them.",
          TOM_COLOR, 0, false },
        { "Judy", "...Tom.",
          JUDY_COLOR, 1, false },
        { "Tom",      "I have to perform happiness.\nOn demand.\nWhile they press little buttons at me.",
          TOM_COLOR, 0, false },
        { "Judy", "I think that IS a metaphor Tom.\nFor work maybe? For the marriage?",
          JUDY_COLOR, 1, false },
        { "Tom",      "Last week they made me eat\nthree meals in a row.\nI was not hungry.",
          TOM_COLOR, 0, true },
        { "Judy", "...Your copay has increased\nto $200 starting next session.",
          JUDY_COLOR, 1, true },
        { "Tom",      "Of course it has.",
          TOM_COLOR, 0, false },
    });
}

void TherapistOfficeScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (windowEffect) windowEffect->update(deltaTime);

    if (getEntrySceneName() == "scene_select") {
        updateSceneDebugCamera(windowEffect, getCamera(), deltaTime);

        if (windowEffect) {
            // Dial-in controls for the backdrop quad's center -- I/K: Y, J/L: X,
            // U/O: Z. Temporary, for finding the right placement to bake into
            // the scene as a fixed value once dialed in (roadOrigin/treeOrigin
            // already went through this same process and are now baked
            // constants in TherapistWindowEffect.hpp).
            Vector3 origin = windowEffect->getBackdropOrigin();
            float moveSpeed = 4.0f * deltaTime;
            if (IsKeyDown(KEY_K)) origin.y -= moveSpeed;
            if (IsKeyDown(KEY_I)) origin.y += moveSpeed;
            if (IsKeyDown(KEY_J)) origin.x -= moveSpeed;
            if (IsKeyDown(KEY_L)) origin.x += moveSpeed;
            if (IsKeyDown(KEY_U)) origin.z -= moveSpeed;
            if (IsKeyDown(KEY_O)) origin.z += moveSpeed;
            windowEffect->setBackdropOrigin(origin);
        }
    }

    if (activeScenario < 0) {
        // Ambient: both sitting, small idle motion -- Tom fidgets slightly
        // more than Judy, who stays composed/still.
        tomFidgetTimer += deltaTime * 2.5f;
        judyNodTimer += deltaTime * 0.8f;

        tom->setPosition({160.0f, 400.0f + sinf(tomFidgetTimer) * 3.0f});
        judy->setPosition({1120.0f, 390.0f + sinf(judyNodTimer) * 1.5f});

        if (!getCamera()->isWideViewEnabled()) {
            getCamera()->setPosition(512.0f, 288.0f);
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

void TherapistOfficeScene::draw() {
    Scene::draw();

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);
    if (background.id != 0) DrawTexture(background, 0, 0, WHITE);
    else drawOffice();
    drawTom(tom->getPosition());
    drawJudy(judy->getPosition());
    EndMode2D();

    // y=40, not 16 -- avoids overlapping other on-screen debug text at the
    // usual y=16 offset.
    if (getEntrySceneName() == "scene_select") {
        drawSceneDebugCameraReadout(windowEffect, 16, 40);

        if (windowEffect) {
            Vector3 origin = windowEffect->getBackdropOrigin();
            const char* txt = TextFormat("backdropOrigin: (%.2f, %.2f, %.2f)  I/K: Y  J/L: X  U/O: Z",
                origin.x, origin.y, origin.z);
            DrawRectangle(10, 84, MeasureText(txt, 18) + 12, 26, Color{0, 0, 0, 160});
            DrawText(txt, 16, 88, 18, Color{140, 255, 160, 255});
        }
    }
}

void TherapistOfficeScene::cleanup() {
    Scene::cleanup();
    windowEffect = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int i = 0; i < 3; i++) {
        if (tomPoses[i].id != 0) UnloadTexture(tomPoses[i]);
        if (judyPoses[i].id != 0) UnloadTexture(judyPoses[i]);
        if (tomPortraits[i].id != 0) UnloadTexture(tomPortraits[i]);
        if (judyPortraits[i].id != 0) UnloadTexture(judyPortraits[i]);
    }
    // init() re-runs on every re-entry to this scene and unconditionally
    // push_back()s the scenario table -- reset so scenarios doesn't
    // accumulate duplicates and a mid-scenario exit doesn't permanently
    // block triggerScenario().
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
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
    return activeScenario >= 0;
}

void TherapistOfficeScene::advanceLine() {
    if (activeScenario < 0) return;

    lineIndex++;
    auto& seq = scenarios[activeScenario];
    if (lineIndex >= (int)seq.size()) {
        endScenario();
        return;
    }
    playLine(seq[lineIndex]);
}

void TherapistOfficeScene::playLine(const TherapistLine& line) {
    dialog->setSpeakerName(line.speaker);
    dialog->setSpeakerColor(line.speakerColor);
    dialog->setText(line.text);

    if (line.focusActor == 0) {
        dialog->setPortraitTexture(tomPortraits[line.emotion]);
        dialog->setPortraitGradient({40, 160, 60, 255}, {15, 60, 25, 255});
    } else if (line.focusActor == 1) {
        dialog->setPortraitTexture(judyPortraits[line.emotion]);
        dialog->setPortraitGradient({20, 140, 120, 255}, {8, 60, 50, 255});
    } else {
        dialog->clearPortraitTexture();
    }

    dialog->show();
    focusCameraOn(line.focusActor, line.shake);
}

void TherapistOfficeScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    dialog->hide();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void TherapistOfficeScene::focusCameraOn(int actorIndex, bool shake) {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = judy;

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
void TherapistOfficeScene::drawOffice() {
    // Bookshelf behind Judy
    Color shelfWood = {90, 65, 45, 255};
    DrawRectangle(700, 100, 220, 260, shelfWood);
    Color shelfDark = {60, 42, 30, 255};
    for (int y = 130; y < 360; y += 65) {
        DrawRectangle(700, y, 220, 6, shelfDark);
    }
    // Books, mixed colors
    Color bookColors[] = {{160, 60, 60, 255}, {60, 90, 140, 255}, {80, 140, 90, 255}, {150, 130, 60, 255}};
    for (int i = 0; i < 4; i++) {
        DrawRectangle(715 + i * 45, 100, 30, 30, bookColors[i]);
    }

    // Window with soft daylight, opposite side
    Color windowFrame = {70, 60, 55, 255};
    Color windowSky = {130, 150, 170, 255};
    DrawRectangle(60, 90, 160, 150, windowFrame);
    DrawRectangle(70, 100, 140, 130, windowSky);
    DrawLine(140, 100, 140, 230, windowFrame);
    DrawLine(70, 165, 210, 165, windowFrame);

    // Small side table with a box of tissues between the two chairs
    Color table = {80, 60, 50, 255};
    DrawRectangle(495, 470, 60, 40, table);
    Color tissueBox = {230, 230, 220, 255};
    DrawRectangle(505, 455, 40, 20, tissueBox);

    // Rug beneath both chairs
    Color rug = {110, 55, 55, 255};
    DrawEllipse(512, 470, 260, 60, rug);
}

void TherapistOfficeScene::drawTom(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    Texture2D pose = tomPoses[1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 30.0f - pose.height), WHITE);
}

void TherapistOfficeScene::drawJudy(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 36.0f;

    Texture2D pose = judyPoses[1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 34.0f - pose.height), WHITE);
}
