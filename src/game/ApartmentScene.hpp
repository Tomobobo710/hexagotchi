#ifndef APARTMENT_SCENE_HPP
#define APARTMENT_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include "CityWindowEffect.hpp"
#include <vector>
#include <string>

// One line of a scripted exchange. Same shape as PizzaParlorScene's
// PizzaLine -- kept as a separate struct per scene for now since each scene
// owns its own small cast/focus-target list; worth sharing later if a third
// scene needs the identical shape.
struct ApartmentLine {
    CharacterId speaker;
    std::string text;
    int focusActor;   // index into this scene's actor slots, or -1 for none
    bool shake;
    PortraitEmotion emotion = PortraitEmotion::Mid;

    // Scripted actor walks (see ActorMove in Scene.hpp) fired the instant
    // this line starts/ends. Empty by default -- most lines have no
    // movement at all.
    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;
};

// Tom alone in his apartment -- the other recurring "check in on the
// gotchi's world" location, ported from the JS prototype's "Monday Morning"
// intro episode. Ambient mode is just Tom being pathetic by himself with
// no one else around; the scripted scenario is the ported episode almost
// line-for-line (alarm, mirror, Karen's text, the broken coffee machine).
class ApartmentScene : public Scene {
public:
    ApartmentScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerScenario(int index);
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

private:
    SceneActor* tom = nullptr;
    Texture2D background = {0};
    CityWindowEffect* cityWindow = nullptr;

    // Full-body pose art for Tom, [0]=sad [1]=mid [2]=happy -- loaded via
    // CharacterRegistry (see init()). Dialog-box portraits are no longer
    // loaded/owned here at all -- DialogBox::setCharacter() loads and
    // caches those itself.
    Texture2D tomPoses[3] = {};

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    // --- Ambient behavior ---
    float tomSlumpTimer = 0.0f;

    // --- Scenario playback ---
    std::vector<std::vector<ApartmentLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const ApartmentLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake);

    void drawTom(Vector2 pos);
    void drawApartment();
};

#endif // APARTMENT_SCENE_HPP
