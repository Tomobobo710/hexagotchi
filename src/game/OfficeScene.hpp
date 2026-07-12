#ifndef OFFICE_SCENE_HPP
#define OFFICE_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include <vector>
#include <string>

class GameState;

struct OfficeLine {
    CharacterId speaker;
    std::string text;
    int focusActor;   // 0 = Tom, 1 = Larry, 2 = Loraine, -1 = none
    bool shake;

    // Camera style for this line's focus shot. false (default) = smooth
    // followPosition ease, like PizzaParlorScene -- the normal look. true =
    // instant setPosition cut, reserved for dramatic beats (the direct-cut
    // feel Scenario C used everywhere). See focusCameraOn().
    bool cutCamera = false;

    PortraitEmotion emotion = PortraitEmotion::Mid;

    // Marks this line as the character's introduction -- playLine() passes
    // firstTimeName to DialogBox::setCharacter() as the name-plate override,
    // just for this one line (e.g. "Larry (Tom's boss)" instead of "Larry").
    // Set this on whichever line in the scenario is that character's actual
    // first line -- there's no automatic first-appearance tracking.
    std::string firstTimeName;

    // Scripted actor walks (see ActorMove in Scene.hpp) fired the instant
    // this line starts/ends. Empty by default -- most lines have no
    // movement at all.
    std::vector<ActorMove> movesAtStart;
    std::vector<ActorMove> movesAtEnd;

    // Per-actor POSE emotion for the on-screen figures during this line.
    // Separate from the portrait `emotion` above (portrait = dialog-box
    // headshot; these = full-body poses). Each persists until a later line
    // changes it, so set only the ones that CHANGE on a line. Indexed like
    // focusActor: [0]=Tom, [1]=Larry, [2]=Loraine. Last fields so existing
    // brace-init lines are unaffected. Default Mid.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion larryPoseEmotion = PoseEmotion::Mid;
    PoseEmotion lorainePoseEmotion = PoseEmotion::Mid;
};

// Datatek Solutions -- ported from the JS prototype's "Performance Review"
// and "The Promotion (Sort Of)" episodes. Ambient mode is Tom alone at his
// (nonexistent) desk, i.e. a yoga ball; the two scripted scenarios cover
// both office beats and are selected by index like the other world-scenes.
class OfficeScene : public Scene {
public:
    OfficeScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerScenario(int index) override;
    int getScenarioCount() const override { return (int)scenarios.size(); }
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

    // Shared live gotchi vitals (owned by GameState). Loraine's "reports"
    // beat in Scenario A reads real stats off this so the numbers on screen
    // are the player's actual creature. Set once in main.cpp; may be null in
    // isolated tests, in which case the stat lines fall back to placeholders.
    void setGameState(GameState* state) { gameState_ = state; }

    // The instant the last dialog line is dismissed, this scene's own black
    // overlay starts ramping 0->255 and keeps ramping continuously for
    // END_FADE_DURATION seconds -- no dead/held time before it starts.
    // isPlayingScenario() stays true for that whole span, so StorySequencer
    // doesn't react (and start its own scene-switch FADE) until the screen
    // here is already fully black. Same pattern as PizzaParlorScene.
    static constexpr float END_FADE_DURATION = 1.5f;

    // Poses draw at 3/4 of native texture size (never 1.0) -- per-scene knob,
    // shared by drawTom/drawLarry via drawPose(). Matches the scale field the
    // scene editor exports, so what's placed there is what renders here.
    static constexpr float POSE_SCALE = 0.5f;

private:
    Texture2D background = {0};

    SceneActor* tom = nullptr;
    SceneActor* larry  = nullptr;
    SceneActor* loraine = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp
    GameState* gameState_ = nullptr;  // Not owned -- live vitals for stat lines

    // Full-body pose art, indexed by PoseEmotion: [0]=sad [1]=mid [2]=happy
    // [3]=scared -- loaded via CharacterRegistry (see init()). Dialog-box
    // portraits are no longer loaded/owned here at all -- DialogBox::
    // setCharacter() loads and caches those itself.
    Texture2D tomPoses[4] = {};
    Texture2D larryPoses[4] = {};
    Texture2D lorainePoses[4] = {};

    // Which pose emotion each on-screen actor is currently drawn with. Set
    // per-line in playLine() from OfficeLine's *PoseEmotion fields, persists
    // between lines. drawTom/drawLarry/drawLoraine index their pose array
    // with this.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion larryPoseEmotion = PoseEmotion::Mid;
    PoseEmotion lorainePoseEmotion = PoseEmotion::Mid;

    float tomWobbleTimer = 0.0f;

    std::vector<std::vector<OfficeLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    // Which actor the camera is currently tracking (0=Tom, 1=Larry, -1=none).
    // Set by focusCameraOn() when a line starts; update() re-follows this
    // actor's LIVE position every frame so the camera keeps up while they
    // moveTo()-walk, instead of easing once to where they were at line start.
    int currentFocusActor = -1;

    // A line's shake is DEFERRED until the camera has settled on that line's
    // speaker -- firing it at line-start punches mid-pan (often while still
    // aimed at the previous speaker), which reads as janky. focusCameraOn()
    // sets this true for a shake line; update() fires the shake once the camera
    // is within the settle radius of the focus target, then clears it.
    bool pendingShake_ = false;

    // Counts UP from 0 once endScenario() fires, 0..END_FADE_DURATION.
    // Negative (-1) means no fade-out is in progress.
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const OfficeLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake, bool cut = false);
    bool cameraTargetFor(int actorIndex, Vector2& out) const;

    void drawTom(Vector2 pos);
    void drawLarry(Vector2 pos);
    void drawLoraine(Vector2 pos);
    void drawOffice();
};

#endif // OFFICE_SCENE_HPP
