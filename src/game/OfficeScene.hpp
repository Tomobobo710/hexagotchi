#ifndef OFFICE_SCENE_HPP
#define OFFICE_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "CharacterRegistry.hpp"
#include "PortalEffect.hpp"
#include <vector>
#include <string>

struct OfficeLine {
    CharacterId speaker;
    std::string text;
    int focusActor;   // 0 = Tom, 1 = Larry, -1 = none
    bool shake;
    PortraitEmotion emotion = PortraitEmotion::Mid;

    // Marks this line as the character's introduction -- playLine() passes
    // firstTimeName to DialogBox::setCharacter() as the name-plate override,
    // just for this one line (e.g. "Larry (Tom's boss)" instead of "Larry").
    // Set this on whichever line in the scenario is that character's actual
    // first line -- there's no automatic first-appearance tracking.
    std::string firstTimeName;
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

    void triggerScenario(int index);
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

    // The instant the last dialog line is dismissed, this scene's own black
    // overlay starts ramping 0->255 and keeps ramping continuously for
    // END_FADE_DURATION seconds -- no dead/held time before it starts.
    // isPlayingScenario() stays true for that whole span, so StorySequencer
    // doesn't react (and start its own scene-switch FADE) until the screen
    // here is already fully black. Same pattern as PizzaParlorScene.
    static constexpr float END_FADE_DURATION = 1.5f;

private:
    Texture2D background = {0};

    SceneActor* tom = nullptr;
    SceneActor* larry  = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    // Full-body pose art, [0]=sad [1]=mid [2]=happy -- loaded via
    // CharacterRegistry (see init()). Dialog-box portraits are no longer
    // loaded/owned here at all -- DialogBox::setCharacter() loads and
    // caches those itself.
    Texture2D tomPoses[3] = {};
    Texture2D larryPoses[3] = {};

    float tomWobbleTimer = 0.0f;

    std::vector<std::vector<OfficeLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    // Counts UP from 0 once endScenario() fires, 0..END_FADE_DURATION.
    // Negative (-1) means no fade-out is in progress.
    float endElapsed = -1.0f;

    void advanceLine();
    void playLine(const OfficeLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake);

    void drawTom(Vector2 pos);
    void drawLarry(Vector2 pos);
    void drawOffice();

    // The merge-machine/teleporter, drawn in front of the background art but
    // behind the actors (see OfficeScene::draw() -- it's a SceneEffect but
    // called directly here instead of via addEffect()'s normal background/
    // foreground split, since neither of those slots alone gives that
    // ordering).
    PortalEffect* portal = nullptr;
};

#endif // OFFICE_SCENE_HPP
