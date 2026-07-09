#include "MergeScene.hpp"
#include "GameConstants.hpp"
#include "raymath.h"

MergeScene::MergeScene()
    : Scene((float)GAME_W, (float)GAME_H, WHITE) {
}

void MergeScene::init() {
    portal = new PortalEffect();
    portal->init();
}

void MergeScene::startMerge(Mode m) {
    mode = m;
    elapsed = 0.0f;
}

void MergeScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (IsKeyPressed(KEY_TAB)) {
        startMerge(mode == Mode::MERGE_IN ? Mode::MERGE_OUT : Mode::MERGE_IN);
    }

    elapsed += deltaTime;
    float t = elapsed / MERGE_DURATION;
    if (t > 1.0f) t = 1.0f;

    float dist, colorT;
    if (mode == Mode::MERGE_IN) {
        // far -> near, black -> white
        dist = Lerp(CAM_DIST_FAR, CAM_DIST_NEAR, t);
        colorT = 1.0f - t;
    } else {
        // near -> far, white -> black
        dist = Lerp(CAM_DIST_NEAR, CAM_DIST_FAR, t);
        colorT = t;
    }

    portal->setDebugCamDist(dist);
    unsigned char shade = (unsigned char)(colorT * 255.0f);
    setBackgroundColor({shade, shade, shade, 255});

    if (portal) portal->update(deltaTime);
}

void MergeScene::draw() {
    Scene::draw();
    if (portal) portal->drawBackground();
}

void MergeScene::cleanup() {
    Scene::cleanup();
    if (portal) { portal->cleanup(); delete portal; portal = nullptr; }
}
