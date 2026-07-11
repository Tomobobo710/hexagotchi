#ifndef ENDING_SCENE_HPP
#define ENDING_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include <vector>
#include <string>

// One line of an ending scenario. Current scene template shape. focusActor
// index scheme: 0=Tom, 1=Loraine, 2=Ronzer, 3=Mark, 4=Karen, -1=none.
//
// The mysterious "???" speaker is handled via `overrideName`: any line can set
// it to replace the name plate for that line only (e.g. "???"), exactly like
// the phone-text insert does. Which CharacterId/portrait the unknown uses is
// TBD -- for now `Narrator` (no portrait) carries the "???" lines.
struct EndingLine {
    CharacterId speaker;
    std::string text;
    int focusActor;
    bool shake;
    bool cutCamera = false;
    PortraitEmotion emotion = PortraitEmotion::Mid;

    // Name-plate override for this line only. Used for character intros AND for
    // the "???" unknown speaker. Empty = normal registry name.
    std::string overrideName;

    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;

    // Per-actor POSE emotion, persists between lines. Indexed like focusActor:
    // [0]=Tom [1]=Loraine [2]=Ronzer [3]=Mark [4]=Karen. Last fields so
    // brace-init isn't disturbed. Default Mid.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion lorainePoseEmotion = PoseEmotion::Mid;
    PoseEmotion ronzerPoseEmotion = PoseEmotion::Mid;
    PoseEmotion markPoseEmotion = PoseEmotion::Mid;
    PoseEmotion karenPoseEmotion = PoseEmotion::Mid;

    // Body-text color for this line (default white). For a colored unknown
    // insert if we want one later.
    Color textColor = {255, 255, 255, 255};
};

// The ENDING. Two scenarios that open the same way and diverge hard on the
// happiness ledger:
//   Scenario 0 = GOOD ending (played when all 3 happiness checkpoints passed)
//   Scenario 1 = BAD ending  (played otherwise)
// ScenarioDirector picks which one via happyCheckpointsPassed(state) == 3.
// The whole cast of Tom's world converges here; a mysterious "???" appears.
class EndingScene : public Scene {
public:
    EndingScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerScenario(int index);
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

    static constexpr float END_FADE_DURATION = 1.5f;

private:
    // Index scheme: 0=Tom 1=Loraine 2=Ronzer 3=Mark 4=Karen.
    SceneActor* tom     = nullptr;
    SceneActor* loraine = nullptr;
    SceneActor* ronzer  = nullptr;
    SceneActor* mark    = nullptr;
    SceneActor* karen   = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    Texture2D tomPoses[4] = {};
    Texture2D lorainePoses[4] = {};
    Texture2D ronzerPoses[4] = {};
    Texture2D markPoses[4] = {};
    Texture2D karenPoses[4] = {};

    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion lorainePoseEmotion = PoseEmotion::Mid;
    PoseEmotion ronzerPoseEmotion = PoseEmotion::Mid;
    PoseEmotion markPoseEmotion = PoseEmotion::Mid;
    PoseEmotion karenPoseEmotion = PoseEmotion::Mid;

    float tomIdleTimer = 0.0f;

    std::vector<std::vector<EndingLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;
    int currentFocusActor = -1;
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const EndingLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake, bool cut = false);
    bool cameraTargetFor(int actorIndex, Vector2& out) const;

    void drawTom(Vector2 pos);
    void drawLoraine(Vector2 pos);
    void drawRonzer(Vector2 pos);
    void drawMark(Vector2 pos);
    void drawKaren(Vector2 pos);
};

#endif // ENDING_SCENE_HPP
