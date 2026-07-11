#ifndef PIZZA_PARLOR_SCENE_HPP
#define PIZZA_PARLOR_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include "SunEffect.hpp"
#include <vector>
#include <string>

// One line of a scripted exchange -- who's talking, what they say, and
// which character should visibly react (camera push/shake target) when
// this line plays. speaker names the character whose registry entry
// (name/color/portrait/gradient) DialogBox::setCharacter() pulls from --
// speaker == Narrator has no on-screen actor and just clears the portrait.
struct PizzaLine {
    CharacterId speaker;
    std::string text;
    // Scene actor index the camera favors, or -1 for none. Index scheme:
    // 0=Tom, 1=Karen, 2=Ronzer, 3=Jimmy, 4=Bimmy, 5=Mark. (0-2 preserved from
    // the original scene so Scenario 0's focus targets still line up.)
    int focusActor;
    bool shake;        // punch-in beat: small camera shake when this line shows

    // Camera style for this line's focus shot. false (default) = smooth
    // followPosition ease -- the normal look. true = instant setPosition cut,
    // reserved for dramatic beats. See focusCameraOn().
    bool cutCamera = false;

    PortraitEmotion emotion = PortraitEmotion::Mid;

    // Marks this line as the character's introduction -- playLine() passes
    // firstTimeName to DialogBox::setCharacter() as the name-plate override,
    // just for this one line (e.g. "Karen (Tom's ex-wife)" instead of
    // "Karen"). Set this on whichever line in the scenario is that
    // character's actual first line -- there's no automatic
    // first-appearance tracking.
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
    // focusActor: [0]=Tom, [1]=Karen, [2]=Ronzer, [3]=Jimmy, [4]=Bimmy,
    // [5]=Mark. Kept immediately after movesAtEnd so the original Scenario 0
    // brace-inits (which end at ronzerPoseEmotion) are unaffected. Default Mid.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion karenPoseEmotion = PoseEmotion::Mid;
    PoseEmotion ronzerPoseEmotion = PoseEmotion::Mid;
    PoseEmotion jimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion bimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion markPoseEmotion = PoseEmotion::Mid;

    // Body-text color for this line. LAST field so no brace-init is disturbed.
    // Default white. Persists until changed, so a line that sets it (e.g. the
    // orange "???" phone-text insert) must be followed by one resetting it to
    // white. playLine() applies it every line so there's no cross-line leak.
    Color textColor = {255, 255, 255, 255};
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
    // Actor index scheme (used by focusActor / actorsByIndex / cameraTargetFor):
    // 0=Tom, 1=Karen, 2=Ronzer, 3=Jimmy, 4=Bimmy, 5=Mark. 0-2 kept from the
    // original scene so Scenario 0 is untouched; 3-5 added for the birthday.
    SceneActor* tom    = nullptr;
    SceneActor* karen  = nullptr;
    SceneActor* ronzer = nullptr;
    SceneActor* jimmy  = nullptr;
    SceneActor* bimmy  = nullptr;
    SceneActor* mark   = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    // Full-body pose art, indexed by PoseEmotion: [0]=sad [1]=mid [2]=happy
    // [3]=scared -- loaded via CharacterRegistry (see init()). Dialog-box
    // portraits are no longer loaded/owned here at all -- DialogBox::
    // setCharacter() loads and caches those itself.
    Texture2D tomPoses[4] = {};
    Texture2D karenPoses[4] = {};
    Texture2D ronzerPoses[4] = {};
    Texture2D jimmyPoses[4] = {};
    Texture2D bimmyPoses[4] = {};
    Texture2D markPoses[4] = {};

    // Which pose emotion each on-screen actor is currently drawn with. Set
    // per-line in playLine() from PizzaLine's *PoseEmotion fields, persists
    // between lines. drawX() indexes its pose array with this.
    PoseEmotion tomPoseEmotion = PoseEmotion::Mid;
    PoseEmotion karenPoseEmotion = PoseEmotion::Mid;
    PoseEmotion ronzerPoseEmotion = PoseEmotion::Mid;
    PoseEmotion jimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion bimmyPoseEmotion = PoseEmotion::Mid;
    PoseEmotion markPoseEmotion = PoseEmotion::Mid;

    // --- Ambient behavior ---
    float ambientTimer = 0.0f;
    float nextQuipTime = 0.0f;
    float quipTimer = 0.0f;
    float tomBobTimer = 0.0f;
    float karenTapTimer = 0.0f;
    float ronzerHopTimer = 0.0f;

    // --- Scenario playback ---
    std::vector<std::vector<PizzaLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    // Which actor the camera is currently tracking (0=Tom,1=Karen,2=Ronzer,
    // -1=none). update() re-follows this actor's LIVE position every frame so
    // the camera keeps up while they moveTo()-walk, instead of easing once to
    // where they were at line start.
    int currentFocusActor = -1;

    // Counts UP from 0 once endScenario() fires, 0..END_FADE_DURATION.
    // Negative (-1) means no fade-out is in progress.
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const PizzaLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake, bool cut = false);
    bool cameraTargetFor(int actorIndex, Vector2& out) const;

    void drawTom(Vector2 pos);
    void drawKaren(Vector2 pos);
    void drawRonzer(Vector2 pos);
    void drawJimmy(Vector2 pos);
    void drawBimmy(Vector2 pos);
    void drawMark(Vector2 pos);

    // A sunset sun glimpsed through the transparent glass cutout in the
    // parlor's front door (parlorbg.png) -- see SunEffect.hpp.
    SunEffect* sunEffect = nullptr;
};

#endif // PIZZA_PARLOR_SCENE_HPP
