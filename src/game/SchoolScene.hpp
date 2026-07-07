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
    int focusActor;   // 0 = Gary, 1 = Karen, 2 = Zap, -1 = none
    bool shake;
};

// Zap's school pickup -- ported from the JS prototype's "The School Pickup
// Incident" episode. Ambient mode is Gary waiting alone outside; the
// scripted event is the ported pickup almost line-for-line (Karen needling
// him for being late, Zap's lost tooth, the inflation joke).
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
    SceneActor* gary  = nullptr;
    SceneActor* karen = nullptr;
    SceneActor* zap   = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    float garyWaitTimer = 0.0f;

    std::vector<std::vector<SchoolLine>> events;
    int activeEvent = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const SchoolLine& line);
    void endEvent();
    void focusCameraOn(int actorIndex, bool shake);

    void drawGary(Vector2 pos);
    void drawKaren(Vector2 pos);
    void drawZap(Vector2 pos);
    void drawSchoolYard();
};

#endif // SCHOOL_SCENE_HPP
