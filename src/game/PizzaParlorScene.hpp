#ifndef PIZZA_PARLOR_SCENE_HPP
#define PIZZA_PARLOR_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include <vector>
#include <string>

// One line of a scripted exchange -- who's talking, what they say, and
// which character should visibly react (camera push/shake target) when
// this line plays. Shapes/placeholder colors for now; real portraits and
// on-screen character art come later.
struct PizzaLine {
    std::string speaker;
    std::string text;
    Color speakerColor;
    int focusActor;   // index into `actors[]` the camera should favor, or -1 for none
    bool shake;        // punch-in beat: small camera shake when this line shows
    int portraitActor;  // which actor's portrait set to draw, or -1 for none (defaults to focusActor via playLine)
    int emotion;         // 0 = sad, 1 = mid, 2 = happy -- index into that actor's portrait set
};

// Recurring "check in on the gotchi's world" hangout location. Most visits
// are ambient -- the cast just idles and the Pokemon chirps its name -- but
// callers (the tomagotchi/stat side, owned by Bazola) can invoke
// triggerEvent() to play a scripted story beat instead. This scene owns no
// tomagotchi stat logic itself; it only reacts to being told to play an event.
//
// Uses the same shared DialogBox the rest of the game drives (see main.cpp's
// `dialog` global) rather than owning a private one, matching how
// GameScene/BossScene's dialog sequences work.
class PizzaParlorScene : public Scene {
public:
    PizzaParlorScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    // Starts scripted event `index` (0-based into the internal event table).
    // Safe to call while ambient; ignored if an event is already playing.
    void triggerEvent(int index);
    bool isPlayingEvent() const;

private:
    SceneActor* tom    = nullptr;
    SceneActor* wife     = nullptr;
    SceneActor* pokemon  = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    // Portraits per actor per emotion: [0]=Tom, [1]=wife/Karen, [2]=pokemon/Ronzer,
    // each with [0]=sad, [1]=mid, [2]=happy. Ronzer only has a happy portrait,
    // so its sad/mid slots reuse the happy texture.
    Texture2D portraits[3][3] = {};

    // --- Ambient behavior ---
    float ambientTimer = 0.0f;
    float nextQuipTime = 0.0f;
    float quipTimer = 0.0f;
    float tomBobTimer = 0.0f;
    float wifeTapTimer = 0.0f;
    float pokemonHopTimer = 0.0f;

    // --- Event playback ---
    std::vector<std::vector<PizzaLine>> events;
    int activeEvent = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const PizzaLine& line);
    void endEvent();
    void focusCameraOn(int actorIndex, bool shake);

    // Per-character silhouette drawing (shapes only -- no art yet)
    void drawTom(Vector2 pos);
    void drawWife(Vector2 pos);
    void drawPokemon(Vector2 pos);
};

#endif // PIZZA_PARLOR_SCENE_HPP
