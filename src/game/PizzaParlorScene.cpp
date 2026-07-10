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

PizzaParlorScene::PizzaParlorScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {225, 120, 60, 255}), dialog(sharedDialog) {
}

void PizzaParlorScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

    sunEffect = new SunEffect();
    addEffect(sunEffect);

    // Actors are invisible bounds/position holders -- drawTom/drawWife/
    // drawPokemon render the actual silhouettes on top, keyed by tag (same
    // pattern BossScene uses for its boss shape).
    tom = new SceneActor({160.0f, 380.0f}, 48.0f, 64.0f);
    tom->setTag("tom");
    tom->setVisible(false);
    addActor(tom);

    wife = new SceneActor({520.0f, 360.0f}, 48.0f, 72.0f);
    wife->setTag("wife");
    wife->setVisible(false);
    addActor(wife);

    pokemon = new SceneActor({620.0f, 400.0f}, 40.0f, 40.0f);
    pokemon->setTag("pokemon");
    pokemon->setVisible(false);
    addActor(pokemon);

    nextQuipTime = 3.0f + (float)(rand() % 400) / 100.0f;

    background = AssetPack::loadTexture("backgrounds/parlorbg.png");

    // Full-body pose art, [actor][emotion].
    poses[0][0] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Sad);
    poses[0][1] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Mid);
    poses[0][2] = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Happy);
    poses[1][0] = CharacterRegistry::loadPose(CharacterId::Karen, PoseEmotion::Sad);
    poses[1][1] = CharacterRegistry::loadPose(CharacterId::Karen, PoseEmotion::Mid);
    poses[1][2] = CharacterRegistry::loadPose(CharacterId::Karen, PoseEmotion::Happy);
    poses[2][0] = CharacterRegistry::loadPose(CharacterId::Ronzer, PoseEmotion::Sad);
    poses[2][1] = CharacterRegistry::loadPose(CharacterId::Ronzer, PoseEmotion::Mid);
    poses[2][2] = CharacterRegistry::loadPose(CharacterId::Ronzer, PoseEmotion::Happy);

    // --- Scenario 0: the wife needling Tom while the Pokemon heckles from the sideline ---
    scenarios.push_back({
        { CharacterId::Tom,    "Oh -- hey. I didn't know you two ate here too.",
          0, false, PortraitEmotion::Mid },
        { CharacterId::Karen,  "We come here every Tuesday, Tom. We've come here every\nTuesday for six weeks.",
          1, false, PortraitEmotion::Mid, "Karen (Tom's ex-wife)" },
        { CharacterId::Ronzer, "Ronzer.",
          2, false, PortraitEmotion::Happy, "Ronzer (her new boyfriend)" },
        { CharacterId::Tom,    "Right. Tuesdays. I knew that.",
          0, false, PortraitEmotion::Sad },
        { CharacterId::Karen,  "Did you get my email about Jimmy's recital?",
          1, false, PortraitEmotion::Mid },
        { CharacterId::Tom,    "I -- yeah, I'm looking into it. Work's been --",
          0, false, PortraitEmotion::Sad },
        { CharacterId::Karen,  "You're a digital pet, Tom. What work.",
          1, true, PortraitEmotion::Sad },
        { CharacterId::Ronzer, "RONZER RONZER RONZER.",
          2, false, PortraitEmotion::Happy },
        { CharacterId::Tom,    "...I have a job. I make kids happy...",
          0, false, PortraitEmotion::Sad },
        { CharacterId::Karen,  "Ronzer got a promotion at the gym he doesn't even work at.\nThey just gave it to him.",
          1, false, PortraitEmotion::Happy },
        { CharacterId::Ronzer, "Ronzer.",
          2, false, PortraitEmotion::Happy },
        { CharacterId::Tom,    "That's not -- that isn't how promotions --",
          0, false, PortraitEmotion::Sad },
        { CharacterId::Karen,  "Get the pizza to go next time, Tom. It's for the kids.",
          1, false, PortraitEmotion::Sad },
        { CharacterId::Tom,    "It's always been for the kids.",
          0, false, PortraitEmotion::Sad },
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

        // Dial-in controls, now targeting the pine tree's position -- I/K: Y,
        // J/L: X, U/O: Z. Temporary, for finding the right placement to bake
        // into the scene as a fixed value once dialed in (same workflow
        // TherapistWindowEffect's roadOrigin/treeOrigin/backdropOrigin went
        // through, and sunOrigin already went through here -- see that
        // scene's update()/draw()). Repurposed from sunOrigin now that the
        // sun's position is baked. Gated to the scene_select debug hub so it
        // doesn't eat I/J/K/L/U/O during real sequencer-driven playthroughs.
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
        wifeTapTimer += deltaTime * 3.0f;
        pokemonHopTimer += deltaTime * 4.0f;

        tom->setPosition({160.0f, 380.0f + sinf(tomBobTimer) * 4.0f});
        wife->setPosition({520.0f, 360.0f + sinf(wifeTapTimer) * 2.0f});
        pokemon->setPosition({620.0f, 400.0f - fabsf(sinf(pokemonHopTimer)) * 14.0f});

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
    drawTom(tom->getPosition());
    drawWife(wife->getPosition());
    drawPokemon(pokemon->getPosition());
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

void PizzaParlorScene::cleanup() {
    Scene::cleanup();
    sunEffect = nullptr;  // owned by Scene::effects, already deleted above
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    for (int actor = 0; actor < 3; actor++) {
        for (int emotion = 0; emotion < 3; emotion++) {
            if (poses[actor][emotion].id != 0) UnloadTexture(poses[actor][emotion]);
        }
    }

    // init() re-runs on every re-entry to this scene (SceneManager re-inits
    // scenes on switch) and unconditionally push_back()s the scenario table
    // -- without resetting these, scenarios accumulates duplicates and
    // activeScenario can be left >= 0 if the player left mid-scenario,
    // permanently blocking triggerScenario()'s guard on every future visit.
    scenarios.clear();
    activeScenario = -1;
    lineIndex = 0;
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
    SceneActor* actorsByIndex[3] = {tom, wife, pokemon};
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
    focusCameraOn(line.focusActor, line.shake);

    SceneActor* actorsByIndex[3] = {tom, wife, pokemon};
    triggerActorMoves(line.movesAtStart, actorsByIndex, 3);
}

void PizzaParlorScene::endScenario() {
    activeScenario = -1;
    lineIndex = 0;
    endElapsed = 0.0f;
    dialog->hide();
    dialog->clearCharacter();
    getCamera()->zoomTo(1.0f, 0.6f);
}

void PizzaParlorScene::focusCameraOn(int actorIndex, bool shake) {
    SceneActor* target = nullptr;
    if (actorIndex == 0) target = tom;
    else if (actorIndex == 1) target = wife;
    else if (actorIndex == 2) target = pokemon;

    if (target) {
        Vector2 pos = target->getCenter();
        getCamera()->followPosition({pos.x, pos.y - 40.0f}, 8.0f);
        getCamera()->zoomTo(1.4f, 0.5f);
    }

    if (shake) {
        getCamera()->shake(6.0f, 0.35f);
    }
}

// --- Character silhouettes -----------------------------------------------
// All shape-only placeholders. Distinct silhouettes so the cast reads apart
// even before real art lands: Tom is a slouched blob, Karen is sharp and
// upright, Ronzer is a round creature with ears -- nothing here is final art
// direction, just enough shape language to tell them apart at a glance.

void PizzaParlorScene::drawTom(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 32.0f;

    Texture2D pose = poses[0][1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 30.0f - pose.height), WHITE);
}

void PizzaParlorScene::drawWife(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 36.0f;

    Texture2D pose = poses[1][1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 34.0f - pose.height), WHITE);
}

void PizzaParlorScene::drawPokemon(Vector2 pos) {
    float cx = pos.x + 20.0f;
    float cy = pos.y + 24.0f;

    Texture2D pose = poses[2][1];
    DrawTexture(pose, (int)(cx - pose.width / 2.0f), (int)(cy + 22.0f - pose.height), WHITE);
}

