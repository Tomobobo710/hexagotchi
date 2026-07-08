#ifndef SCHOOL_SCENE_HPP
#define SCHOOL_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include <vector>
#include <string>

struct SchoolLine {
    std::string speaker;
    std::string text;
    Color speakerColor;
    int focusActor;   // 0 = Tom, 1 = Karen, 2 = Jimmy, -1 = none
    bool shake;
};

// Jimmy's school pickup -- ported from the JS prototype's "The School Pickup
// Incident" episode. Ambient mode is Tom waiting alone outside; the
// scripted event is the ported pickup almost line-for-line (Karen needling
// him for being late, Jimmy's lost tooth, the inflation joke).
class SchoolScene : public Scene {
public:
    SchoolScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerEvent(int index);
    bool isPlayingEvent() const;

private:
    SceneActor* tom  = nullptr;
    SceneActor* karen = nullptr;
    SceneActor* jimmy   = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    Texture2D background = {0};

    float tomWaitTimer = 0.0f;

    std::vector<std::vector<SchoolLine>> events;
    int activeEvent = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const SchoolLine& line);
    void endEvent();
    void focusCameraOn(int actorIndex, bool shake);

    void drawTom(Vector2 pos);
    void drawKaren(Vector2 pos);
    void drawJimmy(Vector2 pos);
    void drawSchoolYard();
};

#endif // SCHOOL_SCENE_HPP
