#include "PizzaParlorScene.hpp"
#include "GameConstants.hpp"
#include "AssetPack.hpp"
#include <cstdlib>
#include <cmath>

static const Color TOM_COLOR    = {139, 172, 15, 255};
static const Color WIFE_COLOR    = {200, 60, 90, 255};
static const Color POKEMON_COLOR = {250, 200, 40, 255};

// Ambient-only lines the Pokemon shouts unprompted, popped through the same
// shared DialogBox as scripted events. Its entire personality is its own name.
static const char* POKEMON_QUIPS[] = {
    "Ronzer!",
    "Ronzer Ronzer!",
    "RONZER.",
    "Ronzer?",
    "Ron... zer!",
};
static const float POKEMON_QUIP_DURATION = 1.8f;

PizzaParlorScene::PizzaParlorScene(DialogBox* sharedDialog)
    : Scene(1280.0f, 720.0f, {30, 20, 20, 255}), dialog(sharedDialog) {
}

void PizzaParlorScene::init() {
    getCamera()->setBoundary(0.0f, 0.0f, 1280.0f, 720.0f);

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

    // Portraits: [actor][emotion], sad/mid/happy. Ronzer only has a happy
    // shot, so its sad/mid slots just reuse that texture.
    portraits[0][0] = AssetPack::loadTexture("portraits/gotchiportraitsad.png");
    portraits[0][1] = AssetPack::loadTexture("portraits/gotchiportraitmid.png");
    portraits[0][2] = AssetPack::loadTexture("portraits/gotchiportraithappy.png");
    portraits[1][0] = AssetPack::loadTexture("portraits/karensad.png");
    portraits[1][1] = AssetPack::loadTexture("portraits/karenmid.png");
    portraits[1][2] = AssetPack::loadTexture("portraits/karenhappy.png");
    portraits[2][2] = AssetPack::loadTexture("portraits/ronzerhappy.png");
    portraits[2][0] = portraits[2][1] = portraits[2][2];

    // --- Event 0: the wife needling Tom while the Pokemon heckles from the sideline ---
    events.push_back({
        { "Tom",    "Oh -- hey. I didn't know you two ate here too.",
          TOM_COLOR, 0, false, 0, 1 },
        { "Karen",   "We come here every Tuesday, Tom. We've come here every\nTuesday for six years.",
          WIFE_COLOR, 1, false, 1, 1 },
        { "Ronzer",  "Ronzer!", POKEMON_COLOR, 2, false, 2, 2 },
        { "Tom",    "Right. Tuesdays. I knew that.",
          TOM_COLOR, 0, false, 0, 0 },
        { "Karen",   "Did you get my email about Jimmy's recital?",
          WIFE_COLOR, 1, false, 1, 1 },
        { "Tom",    "I -- yeah, I'm looking into it. Work's been --",
          TOM_COLOR, 0, false, 0, 0 },
        { "Karen",   "You're a digital pet, Tom. What work.",
          WIFE_COLOR, 1, true, 1, 0 },
        { "Ronzer",  "RONZER RONZER RONZER.", POKEMON_COLOR, 2, false, 2, 2 },
        { "Tom",    "...I have a job. I make kids happy...",
          TOM_COLOR, 0, false, 0, 0 },
        { "Karen",   "Ronzer got a promotion at the gym he doesn't even work at.\nThey just gave it to him.",
          WIFE_COLOR, 1, false, 1, 2 },
        { "Ronzer",  "Ronzer.", POKEMON_COLOR, 2, false, 2, 2 },
        { "Tom",    "That's not -- that isn't how promotions --",
          TOM_COLOR, 0, false, 0, 0 },
        { "Karen",   "Get the pizza to go next time, Tom. It's for the kids.",
          WIFE_COLOR, 1, false, 1, 0 },
        { "Tom",    "It's always been for the kids.",
          TOM_COLOR, 0, false, 0, 0 },
    });
}

void PizzaParlorScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (activeEvent < 0) {
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
                dialog->setSpeakerName("Ronzer");
                dialog->setSpeakerColor(POKEMON_COLOR);
                dialog->setText(POKEMON_QUIPS[idx]);
                dialog->setPortraitTexture(portraits[2][2]);
                dialog->setPortraitGradient({200, 40, 40, 255}, {90, 10, 10, 255});
                dialog->show();
                quipTimer = POKEMON_QUIP_DURATION;
            }
        }

        // Slow establishing drift across the whole set
        float t = (float)GetTime() * 0.05f;
        getCamera()->setPosition(512.0f + sinf(t) * 60.0f, 288.0f);
        getCamera()->setZoom(1.0f);
    }

    if (activeEvent >= 0 && dialog->isVisible() && dialog->isFinished()) {
        SceneInputHandler* ih = getInputHandler();
        if (ih && (ih->isActionPressed(INPUT_ACTION_ACCEPT) || IsKeyPressed(KEY_SPACE))) {
            advanceLine();
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
}

void PizzaParlorScene::cleanup() {
    Scene::cleanup();
    if (background.id != 0) { UnloadTexture(background); background = {0}; }
    if (portraits[0][0].id != 0) UnloadTexture(portraits[0][0]);
    if (portraits[0][1].id != 0) UnloadTexture(portraits[0][1]);
    if (portraits[0][2].id != 0) UnloadTexture(portraits[0][2]);
    if (portraits[1][0].id != 0) UnloadTexture(portraits[1][0]);
    if (portraits[1][1].id != 0) UnloadTexture(portraits[1][1]);
    if (portraits[1][2].id != 0) UnloadTexture(portraits[1][2]);
    if (portraits[2][2].id != 0) UnloadTexture(portraits[2][2]);

    // init() re-runs on every re-entry to this scene (SceneManager re-inits
    // scenes on switch) and unconditionally push_back()s the event table --
    // without resetting these, events accumulates duplicates and activeEvent
    // can be left >= 0 if the player left mid-event, permanently blocking
    // triggerEvent()'s guard on every future visit.
    events.clear();
    activeEvent = -1;
    lineIndex = 0;
}

void PizzaParlorScene::triggerEvent(int index) {
    if (activeEvent >= 0) return;
    if (index < 0 || index >= (int)events.size()) return;

    quipTimer = 0.0f;
    activeEvent = index;
    lineIndex = 0;
    playLine(events[activeEvent][lineIndex]);
}

void PizzaParlorScene::triggerStoryEvent(int eventIndex) {
    triggerEvent(eventIndex);
}

bool PizzaParlorScene::isPlayingEvent() const {
    return activeEvent >= 0;
}

void PizzaParlorScene::advanceLine() {
    if (activeEvent < 0) return;

    lineIndex++;
    auto& seq = events[activeEvent];
    if (lineIndex >= (int)seq.size()) {
        endEvent();
        return;
    }
    playLine(seq[lineIndex]);
}

void PizzaParlorScene::playLine(const PizzaLine& line) {
    dialog->setSpeakerName(line.speaker);
    dialog->setSpeakerColor(line.speakerColor);
    dialog->setText(line.text);

    int portraitActor = line.portraitActor >= 0 ? line.portraitActor : line.focusActor;
    if (portraitActor >= 0 && portraitActor < 3) {
        dialog->setPortraitTexture(portraits[portraitActor][line.emotion]);
        // Per-character gradient behind the portrait -- Tom green, Karen
        // yellow, Ronzer red -- to contrast against each one's own color.
        if (portraitActor == 0) dialog->setPortraitGradient({40, 160, 60, 255}, {15, 60, 25, 255});
        else if (portraitActor == 1) dialog->setPortraitGradient({230, 200, 40, 255}, {120, 100, 10, 255});
        else dialog->setPortraitGradient({200, 40, 40, 255}, {90, 10, 10, 255});
    } else {
        dialog->clearPortraitTexture();
    }

    dialog->show();
    focusCameraOn(line.focusActor, line.shake);
}

void PizzaParlorScene::endEvent() {
    activeEvent = -1;
    lineIndex = 0;
    dialog->hide();
    dialog->clearPortraitTexture();
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

    // Slouched round body -- low energy, deflated posture
    DrawEllipse((int)cx, (int)(cy + 20), 26, 30, TOM_COLOR);
    // Head, tilted down slightly
    DrawCircle((int)cx, (int)(cy - 14), 20, TOM_COLOR);
    // Droopy tired eyes
    Color darkTom = {70, 90, 5, 255};
    DrawEllipse((int)(cx - 8), (int)(cy - 14), 4, 2, darkTom);
    DrawEllipse((int)(cx + 8), (int)(cy - 14), 4, 2, darkTom);
    // Sad mouth
    DrawLineEx({cx - 7, cy - 5}, {cx + 7, cy - 3}, 2.0f, darkTom);
    // Limp little arms
    DrawLineEx({cx - 24, cy + 8}, {cx - 32, cy + 26}, 5.0f, TOM_COLOR);
    DrawLineEx({cx + 24, cy + 8}, {cx + 32, cy + 26}, 5.0f, TOM_COLOR);
}

void PizzaParlorScene::drawWife(Vector2 pos) {
    float cx = pos.x + 24.0f;
    float cy = pos.y + 36.0f;

    // Sharp, upright silhouette -- angular shoulders, arms crossed
    Color darkWife = {110, 20, 40, 255};
    // Body: trapezoid via triangle-ish rectangle for sharp shoulders
    DrawTriangle({cx - 22, cy + 40}, {cx + 22, cy + 40}, {cx, cy - 4}, WIFE_COLOR);
    DrawRectangle((int)(cx - 16), (int)(cy - 4), 32, 20, WIFE_COLOR);
    // Head, held high
    DrawCircle((int)cx, (int)(cy - 26), 16, WIFE_COLOR);
    // Narrowed, unimpressed eyes
    DrawRectangle((int)(cx - 10), (int)(cy - 28), 6, 2, darkWife);
    DrawRectangle((int)(cx + 4), (int)(cy - 28), 6, 2, darkWife);
    // Flat, unimpressed mouth
    DrawLineEx({cx - 6, cy - 18}, {cx + 6, cy - 18}, 2.0f, darkWife);
    // Arms crossed (two diagonal bars over the chest)
    DrawLineEx({cx - 16, cy + 2}, {cx + 12, cy + 12}, 6.0f, darkWife);
    DrawLineEx({cx + 16, cy + 2}, {cx - 12, cy + 12}, 6.0f, darkWife);
}

void PizzaParlorScene::drawPokemon(Vector2 pos) {
    float cx = pos.x + 20.0f;
    float cy = pos.y + 24.0f;

    // Round, bouncy little creature -- pointed ears, big excited eyes
    Color darkMon = {160, 120, 10, 255};
    DrawCircle((int)cx, (int)cy, 18, POKEMON_COLOR);
    // Ears
    DrawTriangle({cx - 14, cy - 10}, {cx - 20, cy - 28}, {cx - 6, cy - 16}, POKEMON_COLOR);
    DrawTriangle({cx + 14, cy - 10}, {cx + 20, cy - 28}, {cx + 6, cy - 16}, POKEMON_COLOR);
    // Big round eyes
    DrawCircle((int)(cx - 7), (int)(cy - 2), 5, darkMon);
    DrawCircle((int)(cx + 7), (int)(cy - 2), 5, darkMon);
    DrawCircle((int)(cx - 6), (int)(cy - 3), 2, WHITE);
    DrawCircle((int)(cx + 8), (int)(cy - 3), 2, WHITE);
    // Open, shouting mouth
    DrawCircle((int)cx, (int)(cy + 8), 5, darkMon);
    // Tiny stub feet
    DrawEllipse((int)(cx - 8), (int)(cy + 18), 6, 4, POKEMON_COLOR);
    DrawEllipse((int)(cx + 8), (int)(cy + 18), 6, 4, POKEMON_COLOR);
}
