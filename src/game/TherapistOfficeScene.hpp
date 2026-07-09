#ifndef THERAPIST_OFFICE_SCENE_HPP
#define THERAPIST_OFFICE_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "TherapistWindowEffect.hpp"
#include <vector>
#include <string>

struct TherapistLine {
    std::string speaker;
    std::string text;
    Color speakerColor;
    int focusActor;   // 0 = Tom, 1 = Therapist, -1 = none
    bool shake;
};

// Tom's therapist's office -- ported from the JS prototype's "The Last
// Session" episode. Ambient mode is just the two of them sitting quietly;
// the scripted event is the ported session almost line-for-line (the
// digital-pet metaphor breakdown, ending on the copay hike).
class TherapistOfficeScene : public Scene {
public:
    TherapistOfficeScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerEvent(int index);
    void triggerStoryEvent(int eventIndex) override;
    bool isPlayingEvent() const override;

private:
    SceneActor* tom      = nullptr;
    SceneActor* therapist  = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    float tomFidgetTimer = 0.0f;
    float therapistNodTimer = 0.0f;

    std::vector<std::vector<TherapistLine>> events;
    int activeEvent = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const TherapistLine& line);
    void endEvent();
    void focusCameraOn(int actorIndex, bool shake);

    void drawTom(Vector2 pos);
    void drawTherapist(Vector2 pos);
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
