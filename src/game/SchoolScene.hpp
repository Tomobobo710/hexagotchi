#ifndef SCHOOL_SCENE_HPP
#define SCHOOL_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include "SchoolSkyEffect.hpp"
#include <vector>
#include <string>

// One line of the school scenario. Current scene template (matches
// OfficeLine/ApartmentLine): speaker/text/focus/shake/cut/emotion/firstTimeName,
// scripted walks, per-actor pose emotions. focusActor: 0=Tom, 1=Karen,
// 2=Jimmy, 3=Bimmy, -1=none.
struct SchoolLine {
    CharacterId speaker;
    std::string text;
    int focusActor;
    bool shake;
    bool cutCamera = false;
    PortraitEmotion emotion = PortraitEmotion::Mid;
    std::string firstTimeName;
    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;

    // Per-actor POSE emotion, persists between lines. Indexed like focusActor:
    // [0]=Tom, [1]=Karen, [2]=Jimmy, [3]=Bimmy. Last fields so existing
    // brace-init lines are unaffected. Default Mid.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion karenPoseEmotion = PoseEmotion::Mid;
    PoseEmotion jimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion bimmyPoseEmotion = PoseEmotion::Mid;
};

// Jimmy's school pickup -- the tooth-fairy inflation beat, reworked: Karen
// needles Tom for being late, Jimmy lost a tooth (the fairy now pays $20),
// Bimmy chimes in, Karen tells Tom to just go home and take a break, then
// reveals Ronzer got the boys $100 Candy City gift cards each. The kids are
// hyped; Tom is quietly flattened. On the current scene template.
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

    static constexpr float END_FADE_DURATION = 1.5f;

private:
    SceneActor* tom   = nullptr;
    SceneActor* karen = nullptr;
    SceneActor* jimmy = nullptr;
    SceneActor* bimmy = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    // Full-body pose art, indexed by PoseEmotion [0]=sad [1]=mid [2]=happy
    // [3]=scared. Dialog-box portraits are loaded/cached by DialogBox itself.
    Texture2D tomPoses[4] = {};
    Texture2D karenPoses[4] = {};
    Texture2D jimmyPoses[4] = {};
    Texture2D bimmyPoses[4] = {};

    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion karenPoseEmotion = PoseEmotion::Mid;
    PoseEmotion jimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion bimmyPoseEmotion = PoseEmotion::Mid;

    float tomWaitTimer = 0.0f;

    std::vector<std::vector<SchoolLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;
    int currentFocusActor = -1;
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const SchoolLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake, bool cut = false);
    bool cameraTargetFor(int actorIndex, Vector2& out) const;

    void drawTom(Vector2 pos);
    void drawKaren(Vector2 pos);
    void drawJimmy(Vector2 pos);
    void drawBimmy(Vector2 pos);
    void drawSchoolYard();
};

#endif // SCHOOL_SCENE_HPP
