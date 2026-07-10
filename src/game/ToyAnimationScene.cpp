#include "ToyAnimationScene.hpp"
#include "GameConstants.hpp"
#include "TomagotchiToyMesh.hpp"
#include "UnlitShader.hpp"
#include "SceneManager.hpp"
#include "raymath.h"
#include <cmath>

ToyAnimationScene::ToyAnimationScene()
    : Scene((float)GAME_W, (float)GAME_H, BLACK) {
}

void ToyAnimationScene::init() {
    unlitShader = LoadUnlitShader();

    Image white = GenImageColor(1, 1, WHITE);
    whiteTex = LoadTextureFromImage(white);
    UnloadImage(white);

    Mesh toyMesh = BuildTomagotchiToyMesh();
    toyModel = LoadModelFromMesh(toyMesh);
    toyModel.materials[0].shader = unlitShader;
    toyModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    toyModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    elapsed = 0.0f;
    switched = false;
}

void ToyAnimationScene::startIntro(const std::string& nextScene) {
    nextSceneName = nextScene;
    elapsed = 0.0f;
    switched = false;
}

void ToyAnimationScene::applyCamera(Camera3D& cam) const {
    float t = elapsed / INTRO_DURATION;
    if (t > 1.0f) t = 1.0f;

    // Three phases: SLIDE (camera tracks in laterally from off to the side
    // at X=SLIDE_X_START to dead-center X=0, at the fixed starting
    // distance/pitch/fovy -- makes the toy's entrance itself a beat instead
    // of just appearing already centered), then on the X=0 plane (no yaw/
    // side motion at all from here on): RISE, orbiting from dead-on-the-
    // front straight up over the top (pitch 0 -> 90) at a constant distance;
    // only once at the apex does the ZOOM phase pull the camera in tight on
    // the screen. Splitting rise/zoom (instead of lerping pitch and distance
    // together) is what makes it read as "orbit over the top, THEN zoom in"
    // rather than one blended swoop.
    float camX;
    float pitchDeg, dist, fovy;
    if (t < SLIDE_FRACTION) {
        float slideT = t / SLIDE_FRACTION;
        float eased = 1.0f - (1.0f - slideT) * (1.0f - slideT); // ease-out
        camX = Lerp(SLIDE_X_START, 0.0f, eased);
        pitchDeg = PITCH_START_DEG;
        dist = DIST_START;
        fovy = FOVY_START;
    } else {
        camX = 0.0f;
        float postSlideT = (t - SLIDE_FRACTION) / (1.0f - SLIDE_FRACTION);
        if (postSlideT < RISE_FRACTION) {
            float riseT = postSlideT / RISE_FRACTION;
            float eased = 1.0f - (1.0f - riseT) * (1.0f - riseT); // ease-out
            pitchDeg = Lerp(PITCH_START_DEG, PITCH_END_DEG, eased);
            dist = DIST_START;
            fovy = FOVY_START;
        } else {
            float zoomT = (postSlideT - RISE_FRACTION) / (1.0f - RISE_FRACTION);
            float eased = 1.0f - (1.0f - zoomT) * (1.0f - zoomT); // ease-out
            pitchDeg = PITCH_END_DEG;
            dist = Lerp(DIST_START, DIST_END, eased);
            fovy = Lerp(FOVY_START, FOVY_END, eased);
        }
    }

    // The toy's screen face (see TomagotchiToyMesh's black primitive) sits on
    // TOP of the sphere, facing +Y -- but the camera targets the toy's own
    // origin (0,0,0), not the screen's off-center height, so the orbit/zoom
    // stays centered on the model itself. pitch=0 (the start) puts the camera
    // straight out on
    // +Z looking dead-on at the toy's front with zero side angle; pitch then
    // rises toward 90 (straight down) while staying on the X=0 plane once the
    // slide finishes, i.e. a single-axis orbit with no yaw/twist.
    //
    // target.x tracks camX during the slide (both move together) instead of
    // staying fixed at 0 -- a look-at camera pointed at a FIXED target from
    // an off-axis position is inherently an angled/arcing shot, not a
    // straight one. Moving position and target in lockstep makes it a pure
    // lateral dolly instead: the view direction stays dead-on (-Z) the whole
    // slide, only the camera's X position (and the toy sliding across frame
    // with it) changes.
    Vector3 target = {camX, 0.0f, 0.0f};
    float camY = target.y + dist * sinf(pitchDeg * DEG2RAD);
    float camZ = dist * cosf(pitchDeg * DEG2RAD);

    cam.position = {camX, camY, camZ};
    cam.target = target;
    // A fixed world-up {0,1,0} degenerates (view direction and up become
    // parallel) right as pitch approaches 90 and the camera looks straight
    // down -- swap to an up vector that rotates WITH the orbit (always
    // perpendicular to the view direction, on the same X=0 plane) so the
    // toy doesn't visibly snap/flip orientation as the camera passes the
    // apex.
    cam.up = {0.0f, cosf(pitchDeg * DEG2RAD), -sinf(pitchDeg * DEG2RAD)};
    cam.fovy = fovy;
    cam.projection = CAMERA_PERSPECTIVE;
}

void ToyAnimationScene::update(float deltaTime) {
    Scene::update(deltaTime);

    elapsed += deltaTime;

    if (!switched && elapsed >= INTRO_DURATION) {
        switched = true;
        if (getSceneManager()) {
            SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
            mgr->switchScene(nextSceneName);
        }
    }
}

void ToyAnimationScene::draw() {
    Scene::draw();

    Camera3D cam3d = {};
    applyCamera(cam3d);

    BeginMode3D(cam3d);
        DrawModelEx(toyModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
    EndMode3D();
}

void ToyAnimationScene::cleanup() {
    Scene::cleanup();
    UnloadModel(toyModel);
    UnloadTexture(whiteTex);
    UnloadShader(unlitShader);
}
