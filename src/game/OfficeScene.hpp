#ifndef OFFICE_SCENE_HPP
#define OFFICE_SCENE_HPP

#include "Scene.hpp"
#include "DialogBox.hpp"
#include "PortalEffect.hpp"
#include <vector>
#include <string>

struct OfficeLine {
    std::string speaker;
    std::string text;
    Color speakerColor;
    int focusActor;   // 0 = Tom, 1 = Boss, -1 = none
    bool shake;
};

// Datatek Solutions -- ported from the JS prototype's "Performance Review"
// and "The Promotion (Sort Of)" episodes. Ambient mode is Tom alone at his
// (nonexistent) desk, i.e. a yoga ball; the two scripted events cover both
// office beats and are selected by index like the other world-scenes.
class OfficeScene : public Scene {
public:
    OfficeScene(DialogBox* sharedDialog);

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

    void triggerEvent(int index);
    bool isPlayingEvent() const;

private:
    Texture2D background = {0};

    SceneActor* tom = nullptr;
    SceneActor* boss  = nullptr;

    DialogBox* dialog = nullptr;  // Not owned -- shared with main.cpp

    float tomWobbleTimer = 0.0f;

    std::vector<std::vector<OfficeLine>> events;
    int activeEvent = -1;
    int lineIndex = 0;

    void advanceLine();
    void playLine(const OfficeLine& line);
    void endEvent();
    void focusCameraOn(int actorIndex, bool shake);

    void drawTom(Vector2 pos);
    void drawBoss(Vector2 pos);
    void drawOffice();

    // The merge-machine/teleporter, drawn in front of the background art but
    // behind the actors (see OfficeScene::draw() -- it's a SceneEffect but
    // called directly here instead of via addEffect()'s normal background/
    // foreground split, since neither of those slots alone gives that
    // ordering).
    PortalEffect* portal = nullptr;
};

#endif // OFFICE_SCENE_HPP
