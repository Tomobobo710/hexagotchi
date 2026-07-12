#ifndef APARTMENT_SCENE_HPP
#define APARTMENT_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include "CityWindowEffect.hpp"
#include <vector>
#include <string>

// One line of a scripted exchange. Same shape as OfficeLine -- carries the
// speaker, on-screen focus target, camera style, portrait emotion, optional
// first-time name override, scripted walks, and per-actor pose emotions.
struct ApartmentLine {
    CharacterId speaker;
    std::string text;
    int focusActor;   // 0 = Tom, 1 = Mark, 2 = Loraine, -1 = none
    bool shake;

    // false (default) = smooth followPosition ease (normal look); true =
    // instant setPosition cut, for dramatic beats. See focusCameraOn().
    bool cutCamera = false;

    PortraitEmotion emotion = PortraitEmotion::Mid;

    // Name-plate override for a character's intro line (e.g. "Mark (the
    // maintenance guy)"). Empty = normal name.
    std::string firstTimeName;

    // Scripted actor walks fired the instant this line starts/ends. Empty by
    // default -- this scene is mostly a static doorway confrontation.
    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;

    // Per-actor POSE emotion for the on-screen figures this line. Separate
    // from the portrait `emotion`. Persists until a later line changes it, so
    // set only what changes. Last fields so existing brace-init is unaffected.
    // Loraine (index 2) only appears in the good-ending coda (Scenario 3).
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion markPoseEmotion = PoseEmotion::Mid;
    PoseEmotion lorainePoseEmotion = PoseEmotion::Mid;
};

// Tom's apartment -- the recurring "check in on the gotchi's world" home
// location. This scenario is "The Heating Situation": Mark the maintenance
// guy at the door, Tom pathetic and cold, a losing negotiation over repairs
// and late rent.
class ApartmentScene : public Scene {
public:
    ApartmentScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerScenario(int index) override;
    int getScenarioCount() const override { return (int)scenarios.size(); }
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

    // Poses draw at half native size (never 1.0) -- matches OfficeScene's
    // POSE_SCALE; the shared story-scene framing recipe.
    static constexpr float POSE_SCALE = 1.0f;

    // Scene-owned black fade the instant the last line ends, additive with
    // the following SceneManager transition -- same pattern as OfficeScene.
    static constexpr float END_FADE_DURATION = 1.5f;

private:
    // Index scheme: 0=Tom, 1=Mark, 2=Loraine. Mark and Loraine never share a
    // scene -- Mark is in scenarios 0/1/2, Loraine only in the good-ending
    // coda (scenario 3).
    SceneActor* tom = nullptr;
    SceneActor* mark = nullptr;
    SceneActor* loraine = nullptr;
    Texture2D background = {0};
    CityWindowEffect* cityWindow = nullptr;

    // Full-body pose art, indexed by PoseEmotion [0]=sad [1]=mid [2]=happy
    // [3]=scared. Loaded via CharacterRegistry (see init()).
    Texture2D tomPoses[4] = {};
    Texture2D markPoses[4] = {};
    Texture2D lorainePoses[4] = {};

    // Which pose emotion each on-screen actor is currently drawn with.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion markPoseEmotion = PoseEmotion::Mid;
    PoseEmotion lorainePoseEmotion = PoseEmotion::Mid;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    // --- Ambient behavior ---
    float tomSlumpTimer = 0.0f;

    // --- Scenario playback ---
    std::vector<std::vector<ApartmentLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    // Camera tracks this actor's LIVE position every frame while a scenario
    // plays (0=Tom, 1=Mark, -1=none). Set by focusCameraOn().
    int currentFocusActor = -1;

    // Counts UP 0..END_FADE_DURATION once endScenario() fires; -1 = idle.
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const ApartmentLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake, bool cut = false);
    bool cameraTargetFor(int actorIndex, Vector2& out) const;

    void drawTom(Vector2 pos);
    void drawMark(Vector2 pos);
    void drawLoraine(Vector2 pos);
    void drawApartment();
};

#endif // APARTMENT_SCENE_HPP
