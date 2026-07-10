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
    PortraitEmotion emotion = PortraitEmotion::Mid;
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

    void triggerScenario(int index);
    void triggerStoryEvent(int scenarioIndex) override;
    bool isPlayingScenario() const override;

private:
    SceneActor* tom  = nullptr;
    SceneActor* judy = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    // Full-body pose art, [0]=sad [1]=mid [2]=happy -- loaded via
    // CharacterRegistry (see init()). Dialog-box portraits are no longer
    // loaded/owned here at all -- DialogBox::setCharacter() loads and
    // caches those itself.
    Texture2D tomPoses[3] = {};
    Texture2D judyPoses[3] = {};

    float tomFidgetTimer = 0.0f;
    float judyNodTimer = 0.0f;

    std::vector<std::vector<TherapistLine>> scenarios;
    int activeScenario = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const TherapistLine& line);
    void endScenario();
    void focusCameraOn(int actorIndex, bool shake);

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
};

#endif // THERAPIST_OFFICE_SCENE_HPP
