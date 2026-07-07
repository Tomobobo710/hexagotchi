#ifndef APARTMENT_SCENE_HPP
#define APARTMENT_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include <vector>
#include <string>

// One line of a scripted exchange. Same shape as PizzaParlorScene's
// PizzaLine -- kept as a separate struct per scene for now since each scene
// owns its own small cast/focus-target list; worth sharing later if a third
// scene needs the identical shape.
struct ApartmentLine {
    std::string speaker;
    std::string text;
    Color speakerColor;
    int focusActor;   // index into this scene's actor slots, or -1 for none
    bool shake;
};

// Gary alone in his apartment -- the other recurring "check in on the
// gotchi's world" location, ported from the JS prototype's "Monday Morning"
// intro episode. Ambient mode is just Gary being pathetic by himself with
// no one else around; the scripted event is the ported episode almost
// line-for-line (alarm, mirror, Karen's text, the broken coffee machine).
class ApartmentScene : public Scene {
public:
    ApartmentScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerEvent(int index);
    bool isPlayingEvent() const;

private:
    SceneActor* gary = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    // --- Ambient behavior ---
    float garySlumpTimer = 0.0f;

    // --- Event playback ---
    std::vector<std::vector<ApartmentLine>> events;
    int activeEvent = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const ApartmentLine& line);
    void endEvent();
    void focusCameraOn(int actorIndex, bool shake);

    void drawGary(Vector2 pos);
    void drawApartment();
};

#endif // APARTMENT_SCENE_HPP
