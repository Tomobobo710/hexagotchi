#ifndef SCHOOL_SCENE_HPP
#define SCHOOL_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include <vector>
#include <string>

struct SchoolLine {
    CharacterId speaker;
    std::string text;
    int focusActor;   // 0 = Tom, 1 = Karen, 2 = Jimmy, -1 = none
    bool shake;
    PortraitEmotion emotion = PortraitEmotion::Mid;

    // Scripted actor walks (see ActorMove in Scene.hpp) fired the instant
    // this line starts/ends. Empty by default -- most lines have no
    // movement at all.
    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;
};

// Jimmy's school pickup -- ported from the JS prototype's "The School Pickup
// Incident" episode. Ambient mode is Tom waiting alone outside; the
// scripted scenario is the ported pickup almost line-for-line (Karen needling
// him for being late, Jimmy's lost tooth, the inflation joke).
class SchoolScene : public Scene {
public:
    SchoolScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerScenario(int index);
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

private:
    SceneActor* tom  = nullptr;
    SceneActor* karen = nullptr;
    SceneActor* jimmy   = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    // Full-body pose art, [0]=Tom, [1]=Karen, [2]=Jimmy, each [0]=sad
    // [1]=mid [2]=happy -- loaded via CharacterRegistry (see init()).
    Texture2D poses[3][3] = {};

    float tomWaitTimer = 0.0f;

    std::vector<std::vector<SchoolLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const SchoolLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake);

    void drawTom(Vector2 pos);
    void drawKaren(Vector2 pos);
    void drawJimmy(Vector2 pos);
    void drawSchoolYard();
};

#endif // SCHOOL_SCENE_HPP
