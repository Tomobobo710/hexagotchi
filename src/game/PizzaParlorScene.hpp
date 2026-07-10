#ifndef PIZZA_PARLOR_SCENE_HPP
#define PIZZA_PARLOR_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "SunEffect.hpp"
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
// triggerScenario() to play a scripted story beat instead. This scene owns
// no tomagotchi stat logic itself; it only reacts to being told to play a
// scenario.
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

    // Starts scripted scenario `index` (0-based into the internal scenario
    // table). Safe to call while ambient; ignored if a scenario is already
    // playing.
    void triggerScenario(int index);

    // Scene override: calls triggerScenario
    void triggerStoryEvent(int scenarioIndex) override;

    bool isPlayingScenario() const override;

    // The instant the last dialog line is dismissed, this scene's own black
    // overlay starts ramping 0->255 and keeps ramping continuously for
    // END_FADE_DURATION seconds -- no dead/held time before it starts.
    // isPlayingScenario() stays true for that whole span, so StorySequencer
    // doesn't react (and start its own scene-switch FADE) until the screen
    // here is already fully black.
    static constexpr float END_FADE_DURATION = 1.5f;

private:
    SceneActor* tom    = nullptr;
    SceneActor* wife     = nullptr;
    SceneActor* pokemon  = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    // Portraits per actor per emotion: [0]=Tom, [1]=wife/Karen, [2]=pokemon/Ronzer,
    // each with [0]=sad, [1]=mid, [2]=happy -- loaded via CharacterRegistry
    // (see init()), which owns the actual asset paths/colors per character.
    Texture2D portraits[3][3] = {};

    // Full-body pose art, same [actor][emotion] indexing as portraits --
    // loaded via CharacterRegistry::loadPose(). Only "mid" is used for now
    // (see drawTom/drawWife/drawPokemon); the other two slots are loaded
    // too so swapping which emotion's pose is drawn later is just an index
    // change, not a re-load. id == 0 means no pose art for that character,
    // and drawX() falls back to its procedural shapes in that case.
    Texture2D poses[3][3] = {};

    // --- Ambient behavior ---
    float ambientTimer = 0.0f;
    float nextQuipTime = 0.0f;
    float quipTimer = 0.0f;
    float tomBobTimer = 0.0f;
    float wifeTapTimer = 0.0f;
    float pokemonHopTimer = 0.0f;

    // --- Scenario playback ---
    std::vector<std::vector<PizzaLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    // Counts UP from 0 once endScenario() fires, 0..END_FADE_DURATION.
    // Negative (-1) means no fade-out is in progress.
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const PizzaLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake);

    // Draws this character's pose art (poses[actor][1] == mid) if loaded,
    // falling back to the original hand-drawn shape silhouette otherwise.
    void drawTom(Vector2 pos);
    void drawWife(Vector2 pos);
    void drawPokemon(Vector2 pos);

    // A sunset sun glimpsed through the transparent glass cutout in the
    // parlor's front door (parlorbg.png) -- see SunEffect.hpp.
    SunEffect* sunEffect = nullptr;
};

#endif // PIZZA_PARLOR_SCENE_HPP
