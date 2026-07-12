#ifndef THERAPIST_OFFICE_SCENE_HPP
#define THERAPIST_OFFICE_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include "TherapistWindowEffect.hpp"
#include <vector>
#include <string>

struct TherapistLine {
    CharacterId speaker;
    std::string text;
    int focusActor;   // 0 = Tom, 1 = Judy, -1 = none
    bool shake;

    // false (default) = smooth followPosition ease; true = instant setPosition
    // cut for dramatic beats. See focusCameraOn().
    bool cutCamera = false;

    PortraitEmotion emotion = PortraitEmotion::Mid;

    std::string firstTimeName;

    // Scripted actor walks (see ActorMove in Scene.hpp) fired the instant
    // this line starts/ends. Empty by default -- most lines have no
    // movement at all.
    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;

    // Per-actor POSE emotion for the on-screen figures, persists between lines.
    // Indexed like focusActor: [0]=Tom, [1]=Judy. Last fields so existing
    // brace-init lines are unaffected. Default Mid.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion judyPoseEmotion = PoseEmotion::Mid;
};

// Tom's therapist's office -- ported from the JS prototype's "The Last
// Session" episode. Ambient mode is just the two of them sitting quietly;
// the scripted scenario is the ported session almost line-for-line (the
// digital-pet metaphor breakdown, ending on the copay hike).
class TherapistOfficeScene : public Scene {
public:
    TherapistOfficeScene(DialogBox* sharedDialog);

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
    SceneActor* tom  = nullptr;
    SceneActor* judy = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    // Full-body pose art, indexed by PoseEmotion [0]=sad [1]=mid [2]=happy
    // [3]=scared. Dialog-box portraits are loaded/cached by DialogBox itself.
    Texture2D tomPoses[4] = {};
    Texture2D judyPoses[4] = {};

    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion judyPoseEmotion = PoseEmotion::Mid;

    float tomFidgetTimer = 0.0f;
    float judyNodTimer = 0.0f;

    std::vector<std::vector<TherapistLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;
    int currentFocusActor = -1;
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const TherapistLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake, bool cut = false);
    bool cameraTargetFor(int actorIndex, Vector2& out) const;

    void drawTom(Vector2 pos);
    void drawJudy(Vector2 pos);
    void drawOffice();

    // The two windows baked into therapistbg.png are flat opaque blue
    // rectangles with no depth -- this draws a little 3D world glimpsed
    // through each one (trees on the left, a hill road with traffic on the
    // right). See TherapistWindowEffect.hpp for why this is called directly
    // (with computed screen rects) instead of through the normal
    // background/foreground SceneEffect split.
    TherapistWindowEffect* windowEffect = nullptr;

    // Which of windowEffect's three dial-in origins (backdrop/road/tree) the
    // I/J/K/L/U/O debug controls currently move -- cycled with TAB. Previously
    // only backdropOrigin was wired, so road/tree could only be fixed by
    // hand-editing constants in TherapistWindowEffect.hpp.
    int debugOriginTarget = 0;  // 0=backdrop, 1=road, 2=tree
};

#endif // THERAPIST_OFFICE_SCENE_HPP
