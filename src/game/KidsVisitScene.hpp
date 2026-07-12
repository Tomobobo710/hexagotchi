#ifndef KIDS_VISIT_SCENE_HPP
#define KIDS_VISIT_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include "CityWindowEffect.hpp"
#include <vector>
#include <string>

// One line of the kids-visit scenario. Same shape as OfficeLine/ApartmentLine
// (current scene template): speaker/text/focus/shake/cut/emotion/firstTimeName,
// scripted walks, and per-actor pose emotions. focusActor: 0=Tom, 1=Bimmy,
// 2=Jimmy, -1=none.
struct KidsLine {
    CharacterId speaker;
    std::string text;
    int focusActor;
    bool shake;
    bool cutCamera = false;
    PortraitEmotion emotion = PortraitEmotion::Mid;
    std::string firstTimeName;
    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;

    // Per-actor POSE emotion for the on-screen figures, persists between lines.
    // Indexed like focusActor: [0]=Tom, [1]=Bimmy, [2]=Jimmy. Last fields so
    // existing brace-init lines are unaffected. Default Mid.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion bimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion jimmyPoseEmotion = PoseEmotion::Mid;
};

// "Tom's Weekend With The Kids" -- Tom has the boys (Bimmy and Jimmy) at his
// sad cold apartment on his custody turn. They complain about the place and the
// smelly bus, favorably compare Ronzer's cool car, and ask when Dad's coming
// home. Tom fumbles the answer the way a real dad would. Dark-funny; kids are
// blunt and loving at once.
//
// Same shared DialogBox as the rest of the game (see main.cpp's `dialog`).
class KidsVisitScene : public Scene {
public:
    KidsVisitScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerScenario(int index) override;
    int getScenarioCount() const override { return (int)scenarios.size(); }
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

    static constexpr float END_FADE_DURATION = 1.5f;

private:
    SceneActor* tom   = nullptr;
    SceneActor* bimmy = nullptr;
    SceneActor* jimmy = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    // Full-body pose art, indexed by PoseEmotion [0]=sad [1]=mid [2]=happy
    // [3]=scared. Dialog-box portraits are loaded/cached by DialogBox itself.
    Texture2D tomPoses[4] = {};
    Texture2D bimmyPoses[4] = {};
    Texture2D jimmyPoses[4] = {};

    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion bimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion jimmyPoseEmotion = PoseEmotion::Mid;

    float tomSlumpTimer = 0.0f;

    std::vector<std::vector<KidsLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;
    int currentFocusActor = -1;
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const KidsLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake, bool cut = false);
    bool cameraTargetFor(int actorIndex, Vector2& out) const;

    void drawTom(Vector2 pos);
    void drawBimmy(Vector2 pos);
    void drawJimmy(Vector2 pos);

    // Reuses the apartment's city-through-window 3D backdrop, since this is
    // Tom's apartment too.
    CityWindowEffect* cityWindow = nullptr;
};

#endif // KIDS_VISIT_SCENE_HPP
