#include "CityWindowEffect.hpp"
#include "GameConstants.hpp"
#include "LitShader.hpp"
#include "raymath.h"
#include <cmath>

// Building walls are a lit unit-cube model (shared LitShader/CreateLight0 rig,
// same as SchoolSkyEffect/MoonEffect); windows are flat/unlit quads so they
// read as glowing rather than receiving light.
static const Color WINDOW_LIT  = {235, 200, 120, 255};
static const Color WINDOW_DARK = {35, 38, 48, 255};

// Train path: each car's X is Lerp(TRAIN_X_START, TRAIN_X_END, t), so t=0..1
// spans TRAIN_X_SPAN world units. The second car trails the first by
// TRAIN_CAR_GAP_X, i.e. TRAIN_CAR_GAP_T in t-space. These drive both the
// draw positions and the visible-window/shake timing math, so they live in
// one place.
static const float TRAIN_X_START   = -7.0f;
static const float TRAIN_X_END     =  7.0f;
static const float TRAIN_X_SPAN    = TRAIN_X_END - TRAIN_X_START;  // 14
static const float TRAIN_CAR_GAP_X = 6.5f;
static const float TRAIN_CAR_GAP_T = TRAIN_CAR_GAP_X / TRAIN_X_SPAN;
// The train always rides this many units in front of the camera (so it stays
// "just outside the glass" regardless of the debug camera distance).
static const float TRAIN_CAM_OFFSET_Z = 4.0f;

void CityWindowEffect::init() {
    buildBuildings();

    shader = LoadLitShader();
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int ambLoc = GetShaderLocation(shader, "ambient");
    float amb[4] = {0.35f, 0.35f, 0.4f, 1.0f};
    SetShaderValue(shader, ambLoc, amb, SHADER_UNIFORM_VEC4);

    Image white = GenImageColor(1, 1, WHITE);
    whiteTex = LoadTextureFromImage(white);
    UnloadImage(white);

    wallModel = LoadModelFromMesh(GenMeshCube(1.0f, 1.0f, 1.0f));
    wallModel.materials[0].shader = shader;
    wallModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

    // Mostly down, off to the right, and a little away from the camera --
    // position is up+left+toward-camera, target is down+right+away, so the
    // light vector (position->target) points down-right-away as asked.
    light = CreateLight0(LIGHT_DIRECTIONAL, {-2.0f, 4.0f, 3.0f}, {2.0f, -4.0f, -1.0f}, WHITE, shader);
}

int CityWindowEffect::sideWindowCols(const Building& b) {
    int cols = (int)(b.size.z / 0.5f);
    if (cols < 2) cols = 2;
    return cols;
}

void CityWindowEffect::buildBuildings() {
    buildings.clear();

    // Three parallel streets: a centerline row plus one to each side, so
    // panning the real 2D camera around doesn't reveal a single lonely
    // street. Each row is offset in both X (sideways) and Z (depth): the Z
    // stagger is what keeps rows from sharing a depth plane -- if two rows
    // sat at identical depths, windows from different rows could project to
    // the same pixels at the same distance and z-fight into a torn two-color
    // mess.
    addBuildingRow(0.0f, 0.0f);
    addBuildingRow(6.0f, -2.3f);
    addBuildingRow(-6.0f, -4.7f);

    // Single centered building far down the centerline, capping the view.
    static const float SCALE = 0.5f;
    Building capBuilding;
    capBuilding.basePos = {0.0f, 0.0f, -85.0f};
    capBuilding.size = {9.0f * SCALE, 11.0f * SCALE, 5.0f * SCALE};
    capBuilding.wallColor = {70, 50, 38, 255};
    capBuilding.windowCols = 5;
    capBuilding.windowRows = 6;
    buildings.push_back(capBuilding);

    for (auto& b : buildings) {
        // Front (+Z) grid, plus a second grid on the inner side face if this
        // building has a row-mate beside it (sideWindowSign != 0) -- see the
        // sideWindowCols() helper for how many columns that side grid gets.
        int count = b.windowCols * b.windowRows;
        if (b.sideWindowSign != 0) count += sideWindowCols(b) * b.windowRows;
        b.windowLit.resize(count);
        b.windowTimer.resize(count);
        for (int i = 0; i < count; i++) {
            b.windowLit[i] = GetRandomValue(0, 100) < 55;
            b.windowTimer[i] = (float)GetRandomValue(20, 200) / 10.0f; // 2-20s to first flicker
        }
    }
}

void CityWindowEffect::addBuildingRow(float rowXOffset, float rowZOffset) {
    // One street row: 3 buildings receding in Z on each side. The X spread
    // narrows slightly with depth so the street reads as a converging
    // corridor. Wall browns run from a warm reddish-brown up close to a
    // cooler tan further back.
    static const Color WALL_COLORS[3] = {
        {92, 58, 42, 255}, {78, 56, 40, 255}, {102, 78, 54, 255},
    };

    static const float SCALE = 0.5f;
    for (int i = 0; i < 3; i++) {
        float z = -20.0f - i * 15.0f + rowZOffset;
        float wFull = 5.0f - i * 0.4f;
        float w = wFull * SCALE;
        float x = wFull * 0.5f + 0.15f;
        float depth = 6.0f * SCALE;
        float h = (7.0f + i * 1.4f) * SCALE;

        Building left;
        left.basePos = {rowXOffset - x, 0.0f, z};
        left.size = {w, h, depth};
        left.wallColor = WALL_COLORS[i];
        left.windowCols = 3;
        left.windowRows = 4 + i;
        // Left building sits on the -X side of this row's centerline, so
        // its INNER (toward-center) face is its +X side.
        left.sideWindowSign = 1;
        buildings.push_back(left);

        Building right = left;
        right.basePos = {rowXOffset + x, 0.0f, z};
        // Right building's inner face is its -X side (mirror of left).
        right.sideWindowSign = -1;
        buildings.push_back(right);
    }
}

void CityWindowEffect::updateWindows(Building& b, float deltaTime) {
    for (size_t i = 0; i < b.windowTimer.size(); i++) {
        b.windowTimer[i] -= deltaTime;
        if (b.windowTimer[i] <= 0.0f) {
            b.windowLit[i] = !b.windowLit[i];
            // Lit windows stay lit longer than dark ones flicker back on --
            // a real building at night is mostly-lit with occasional dark
            // windows, not a 50/50 strobe.
            b.windowTimer[i] = b.windowLit[i]
                ? (float)GetRandomValue(50, 400) / 10.0f   // 5-40s lit
                : (float)GetRandomValue(20, 150) / 10.0f;  // 2-15s dark
        }
    }
}

void CityWindowEffect::update(float deltaTime) {
    for (auto& b : buildings) updateWindows(b, deltaTime);

    float camPos[3] = {0.0f, 0.0f, debugCamDist};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
    UpdateLightValues(shader, light);

    trainTimer += deltaTime;
    if (!trainActive && trainTimer >= trainInterval) {
        trainActive = true;
        trainT = 0.0f;
        trainShakeFiredThisPass = false;

        // The train's X path is much wider than the window ever shows, so
        // t=0..1 is NOT "enters..exits frame" -- most of it is off to the
        // sides. Compute the actual visible-t window from the camera
        // geometry (fovy and distance to the train's depth), then time the
        // shake as: lead-in before the lead car appears, hold through both
        // cars crossing, trail-out after the last one leaves.
        float trainZ = debugCamDist - TRAIN_CAM_OFFSET_Z;
        float distToTrain = debugCamDist - trainZ;
        float halfVisibleX = tanf(debugFovyDeg * 0.5f * DEG2RAD) * distToTrain
            * ((float)GAME_W / (float)GAME_H);
        // x = Lerp(START, END, t) => t = (x - START) / SPAN
        float tEnter = (-halfVisibleX - TRAIN_X_START) / TRAIN_X_SPAN;
        float tLeadExit = (halfVisibleX - TRAIN_X_START) / TRAIN_X_SPAN;
        float tTrailExit = tLeadExit + TRAIN_CAR_GAP_T;

        static const float LEAD_IN_SECONDS = 0.6f;
        static const float TRAIL_OUT_SECONDS = 0.8f;
        float tLeadInStart = tEnter - LEAD_IN_SECONDS / trainDuration;
        shakeTriggerT = tLeadInStart;
        shakeDuration = (tTrailExit - tLeadInStart) * trainDuration + TRAIL_OUT_SECONDS;
    }

    if (trainActive) {
        trainT += deltaTime / trainDuration;
        if (!trainShakeFiredThisPass && trainT >= shakeTriggerT) {
            trainShakeRequested = true;
            trainShakeFiredThisPass = true;
        }
        // Pass ends only once the trailing car has also cleared the frame
        // (t = 1 is just the lead car's exit; the trailing car is one gap
        // behind), otherwise the whole train would vanish mid-crossing.
        if (trainT >= 1.0f + TRAIN_CAR_GAP_T) {
            trainActive = false;
            trainTimer = 0.0f;
        }
    }
}

bool CityWindowEffect::consumeTrainShakeRequest() {
    if (trainShakeRequested) {
        trainShakeRequested = false;
        return true;
    }
    return false;
}

void CityWindowEffect::drawTrainCar(float x, float trainZ) const {
    Vector3 pos = {x, 0.0f, trainZ};
    DrawCube(pos, 6.0f, 1.1f, 0.8f, Color{25, 25, 30, 255});
    for (int i = 0; i < 8; i++) {
        float wx = x - 2.6f + i * 0.7f;
        DrawCube({wx, 0.05f, trainZ + 0.41f}, 0.35f, 0.4f, 0.02f, WINDOW_LIT);
    }
}

void CityWindowEffect::drawBuilding(const Building& b) const {
    DrawModelEx(wallModel, b.basePos, {0, 1, 0}, 0.0f, b.size, b.wallColor);

    // Window grid on the front face (+Z, toward camera down the street).
    // The quads sit slightly proud of the wall surface; the inset scales
    // with the building's depth so it stays clear of the wall at every size
    // rather than z-fighting it.
    float faceInset = fmaxf(b.size.z * 0.03f, 0.03f);
    float faceZ = b.basePos.z + b.size.z * 0.5f + faceInset;
    float left = b.basePos.x - b.size.x * 0.5f;
    float top = b.basePos.y + b.size.y * 0.5f;
    float colStep = b.size.x / (b.windowCols + 1);
    float rowStep = b.size.y / (b.windowRows + 1);
    float winW = colStep * 0.55f;
    float winH = rowStep * 0.55f;

    for (int r = 0; r < b.windowRows; r++) {
        for (int c = 0; c < b.windowCols; c++) {
            int idx = r * b.windowCols + c;
            Color col = b.windowLit[idx] ? WINDOW_LIT : WINDOW_DARK;
            float wx = left + colStep * (c + 1);
            float wy = top - rowStep * (r + 1);
            Vector3 p0 = {wx - winW * 0.5f, wy - winH * 0.5f, faceZ};
            Vector3 p1 = {wx + winW * 0.5f, wy - winH * 0.5f, faceZ};
            Vector3 p2 = {wx + winW * 0.5f, wy + winH * 0.5f, faceZ};
            Vector3 p3 = {wx - winW * 0.5f, wy + winH * 0.5f, faceZ};
            DrawTriangle3D(p0, p1, p2, col);
            DrawTriangle3D(p0, p2, p3, col);
        }
    }

    // Inner side face grid (the face pointing toward this row's centerline)
    // -- without this, only the fronts (facing the camera down the street)
    // ever lit up, and the side facing the next row over/the street between
    // rows stayed a flat dark wall even though it's clearly visible once the
    // real 2D camera pans and the angle changes.
    if (b.sideWindowSign != 0) {
        int sideCols = sideWindowCols(b);
        float sideInset = fmaxf(b.size.x * 0.03f, 0.03f);
        float sideFaceX = b.basePos.x + b.sideWindowSign * (b.size.x * 0.5f + sideInset);
        float front = b.basePos.z + b.size.z * 0.5f;
        float sideColStep = b.size.z / (sideCols + 1);
        float sideWinD = sideColStep * 0.55f;
        int frontCount = b.windowCols * b.windowRows;

        for (int r = 0; r < b.windowRows; r++) {
            for (int c = 0; c < sideCols; c++) {
                int idx = frontCount + r * sideCols + c;
                Color col = b.windowLit[idx] ? WINDOW_LIT : WINDOW_DARK;
                float wz = front - sideColStep * (c + 1);
                float wy = top - rowStep * (r + 1);
                Vector3 p0 = {sideFaceX, wy - winH * 0.5f, wz - sideWinD * 0.5f};
                Vector3 p1 = {sideFaceX, wy - winH * 0.5f, wz + sideWinD * 0.5f};
                Vector3 p2 = {sideFaceX, wy + winH * 0.5f, wz + sideWinD * 0.5f};
                Vector3 p3 = {sideFaceX, wy + winH * 0.5f, wz - sideWinD * 0.5f};
                // Winding flips with sideWindowSign so the quad still faces
                // outward (toward the centerline) instead of into the
                // building on whichever side this is.
                if (b.sideWindowSign < 0) {
                    DrawTriangle3D(p0, p1, p2, col);
                    DrawTriangle3D(p0, p2, p3, col);
                } else {
                    DrawTriangle3D(p1, p0, p2, col);
                    DrawTriangle3D(p2, p0, p3, col);
                }
            }
        }
    }
}

void CityWindowEffect::drawBackground() {
    // This renders into the full 1280x720 canvas (same as SchoolSkyEffect);
    // apartmentbg.png's transparent window cutout is what frames it. That
    // cutout sits left-and-up of canvas center (bbox 476,185..772,400 vs the
    // 640,360 center), so the camera target is lens-shifted by that same
    // fraction of the frustum -- a shift of the look direction rather than a
    // world offset, so it stays aligned at every depth.
    float offsetXFrac = -16.0f / 1280.0f;   // window center vs canvas center
    float offsetYFrac = -67.5f / 720.0f;
    float camFovy = debugFovyDeg;
    float camDist = debugCamDist;
    float halfH = tanf(camFovy * 0.5f * DEG2RAD) * camDist;
    float halfW = halfH * ((float)GAME_W / (float)GAME_H);
    float targetX = offsetXFrac * 2.0f * halfW;
    float targetY = -offsetYFrac * 2.0f * halfH; // screen Y-down -> world Y-up

    // Pitch the whole view (debugPitchDeg, positive = down) -- rotate the
    // camera's forward vector (position->target, originally straight down
    // -Z) around the local X axis so it angles downward, instead of just
    // nudging the target point (which only lens-shifts, it doesn't actually
    // tilt the camera). The lens-shift offsets above are computed first,
    // then this rotation is applied to the resulting look vector as a whole.
    Vector3 camPos = {0.0f, 0.0f, camDist};
    Vector3 targetPos = {targetX, targetY, 0.0f};
    Vector3 forward = Vector3Subtract(targetPos, camPos);
    Vector3 right = {1.0f, 0.0f, 0.0f};
    forward = Vector3RotateByAxisAngle(forward, right, debugPitchDeg * DEG2RAD);

    Camera3D cam3d = {};
    cam3d.position   = camPos;
    cam3d.target     = Vector3Add(camPos, forward);
    cam3d.up         = {0.0f, 1.0f, 0.0f};
    cam3d.fovy       = camFovy;
    cam3d.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(cam3d);
        for (const auto& b : buildings) drawBuilding(b);

        if (trainActive) {
            // One two-car train: a lead car and a second trailing it by a
            // fixed physical gap, both at track level (Y=0) just in front of
            // the camera.
            float trainZ = camDist - TRAIN_CAM_OFFSET_Z;
            float leadX = Lerp(TRAIN_X_START, TRAIN_X_END, trainT);
            drawTrainCar(leadX, trainZ);
            drawTrainCar(leadX - TRAIN_CAR_GAP_X, trainZ);
        }
    EndMode3D();
}

void CityWindowEffect::cleanup() {
    buildings.clear();
    UnloadModel(wallModel);
    UnloadTexture(whiteTex);
    UnloadShader(shader);
}
