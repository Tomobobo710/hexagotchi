#include "MergeScene.hpp"
#include "GameConstants.hpp"
#include "CharacterRegistry.hpp"
#include "raymath.h"
#include <cmath>

MergeScene::MergeScene()
    : Scene((float)GAME_W, (float)GAME_H, WHITE) {
}

void MergeScene::init() {
    portal = new PortalEffect();
    portal->init();

    // Tom's spiralling sprite. Scared feels right for being flung through a
    // portal; falls back to Mid/null inside loadPose if that art's missing.
    tomPose = CharacterRegistry::loadPose(CharacterId::Tom, PoseEmotion::Scared);

    // Sized to the pose art so scale/rotation in draw() pivot around its
    // center. If there's no pose art (id==0), SceneActor draws a placeholder
    // rectangle at this size instead, which still shows the spiral.
    float w = tomPose.id != 0 ? (float)tomPose.width  : 96.0f;
    float h = tomPose.id != 0 ? (float)tomPose.height : 128.0f;
    tom = new SceneActor({(float)GAME_W / 2.0f, (float)GAME_H / 2.0f}, w, h);
    if (tomPose.id != 0) tom->setTexture(tomPose);
    tom->setTag("tom");
    // Not added via addActor(): we drive/draw it by hand in update()/draw() so
    // it lands on top of the portal (same reason OfficeScene draws its actors
    // after portal->drawBackground()).
    placeTomOnSpiral(0.0f);
}

void MergeScene::placeTomOnSpiral(float p) {
    if (!tom) return;

    // Radius: 0 at the center (p=1), SPIRAL_RADIUS_MAX at the outer end (p=0).
    // outward^RADIUS_POW stays small across most of the run (tight coil near
    // the center) and only ramps wide toward the outer end -- a long spiral
    // that reads tight in the middle and flares waaay out right at the end,
    // rather than a couple of loops. The curve still climbs (not flat) as p->0
    // so MERGE_OUT keeps flinging outward instead of settling to a rest frame;
    // with SPIRAL_RADIUS_MAX past the screen corner, that end lands offscreen.
    // Angle sweeps SPIRAL_TURNS full loops across the whole run.
    float outward = 1.0f - p;                 // 0 at center, 1 at outer end
    float radius = SPIRAL_RADIUS_MAX * powf(outward, RADIUS_POW);
    float angle = p * SPIRAL_TURNS * 2.0f * PI;

    float cx = (float)GAME_W / 2.0f;
    float cy = (float)GAME_H / 2.0f;
    Vector2 center = {
        cx + cosf(angle) * radius,
        cy + sinf(angle) * radius
    };

    // setPosition treats position as the sprite's top-left (draw() offsets by
    // half-size for the pivot), so back the center out to a top-left.
    // Scale rides SCALE_MAX->0 (not SCALE_MIN) so he fully disappears into the
    // portal's center rather than leaving an upright speck behind. Once he's
    // basically gone, hide him outright so nothing lingers on the last frame.
    float s = Lerp(SCALE_MAX, 0.0f, p);
    tom->setVisible(s > 0.01f);
    tom->setScale({s, s});
    tom->setPosition({center.x - tom->getWidth() * s / 2.0f,
                      center.y - tom->getHeight() * s / 2.0f});

    // Tumble on top of the orbit for extra spin -- keeps whirling right up
    // until he vanishes, so he's never frozen upright in frame.
    tom->setRotation(p * SPIN_TURNS * 360.0f);
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

    // Spiral progress: MERGE_IN runs 0->1 (out -> merged into center),
    // MERGE_OUT runs 1->0 (bursts back out of center).
    float p = (mode == Mode::MERGE_IN) ? t : (1.0f - t);
    placeTomOnSpiral(p);
    if (tom) tom->update(deltaTime);

    if (portal) portal->update(deltaTime);
}

void MergeScene::draw() {
    Scene::draw();
    if (portal) portal->drawBackground();
    // Tom on top of the portal.
    if (tom) tom->draw();
}

void MergeScene::cleanup() {
    Scene::cleanup();
    if (portal) { portal->cleanup(); delete portal; portal = nullptr; }
    if (tom) { delete tom; tom = nullptr; }
    if (tomPose.id != 0) { UnloadTexture(tomPose); tomPose = {0}; }
}
