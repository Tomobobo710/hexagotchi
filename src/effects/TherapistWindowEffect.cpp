#include "TherapistWindowEffect.hpp"
#include "LitShader.hpp"
#include "VehicleMesh.hpp"
#include "raymath.h"
#include <cmath>
#include <algorithm>

void TherapistWindowEffect::init() {
    shader = LoadLitShader();
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int ambLoc = GetShaderLocation(shader, "ambient");
    float amb[4] = {0.45f, 0.45f, 0.5f, 1.0f};
    SetShaderValue(shader, ambLoc, amb, SHADER_UNIFORM_VEC4);

    Image white = GenImageColor(1, 1, WHITE);
    whiteTex = LoadTextureFromImage(white);
    UnloadImage(white);

    trunkModel = LoadModelFromMesh(GenMeshCylinder(0.5f, 1.0f, 8));
    trunkModel.materials[0].shader = shader;
    trunkModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

    foliageModel = LoadModelFromMesh(GenMeshSphere(1.0f, 10, 10));
    foliageModel.materials[0].shader = shader;
    foliageModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

    light = CreateLight0(LIGHT_DIRECTIONAL, {-2.0f, 4.0f, 5.0f}, {1.5f, -3.0f, -2.0f}, WHITE, shader);

    buildTrees();
}

void TherapistWindowEffect::buildTrees() {
    trees.clear();
    // A small stand, staggered in X/Z so they don't read as a single flat row.
    // Spread widened ~25% from the original clump (same center), so the
    // stand doesn't read as one tight bunch inside the window.
    static const float xs[] = {-7.58f, -6.70f, -8.08f, -6.33f, -7.21f};
    static const float zs[] = {-1.77f, -3.02f, -4.02f, -4.89f, -0.90f};
    for (int i = 0; i < 5; i++) {
        Tree t;
        t.basePos = {xs[i], -6.0f, zs[i]};
        t.trunkHeight = 6.4f + (float)(GetRandomValue(0, 240)) / 100.0f;
        t.foliageRadius = 0.9f + (float)(GetRandomValue(0, 40)) / 100.0f;
        t.swayPhase = (float)(GetRandomValue(0, 628)) / 100.0f;
        t.swaySpeed = 0.5f + (float)(GetRandomValue(0, 40)) / 100.0f;
        trees.push_back(t);
    }
}

void TherapistWindowEffect::update(float deltaTime) {
    time += deltaTime;

    float camPos[3] = {0.0f, 0.0f, debugCamDist};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
    UpdateLightValues(shader, light);

    for (auto& t : trees) t.swayPhase += t.swaySpeed * deltaTime;

    spawnTimer += deltaTime;
    if (spawnTimer >= spawnInterval) {
        spawnTimer = 0.0f;
        spawnInterval = 3.0f + (float)(GetRandomValue(0, 300)) / 100.0f;
        Vehicle v;
        v.isTruck = GetRandomValue(0, 100) < 35;
        v.speed = v.isTruck ? 0.055f : 0.09f;
        // Half oncoming (left lane, driving down) half outbound (right lane,
        // driving up) -- oncoming starts at t=1 and counts down instead of up.
        v.oncoming = GetRandomValue(0, 1) == 1;
        v.t = v.oncoming ? 1.0f : 0.0f;
        // Random paint job per spawn -- random hue, moderate-to-high
        // saturation/value so it reads as a normal car/truck paint color
        // rather than washed out or neon.
        auto randomVehicleColor = [](float satMin, float satMax, float valMin, float valMax) -> Color {
            float hue = (float)GetRandomValue(0, 359);
            float sat = satMin + (float)(GetRandomValue(0, 1000)) / 1000.0f * (satMax - satMin);
            float val = valMin + (float)(GetRandomValue(0, 1000)) / 1000.0f * (valMax - valMin);
            return ColorFromHSV(hue, sat, val);
        };
        v.bodyColor = randomVehicleColor(0.35f, 0.85f, 0.35f, 0.85f);
        // Truck cab stays a fixed neutral grey (real trucks' cabs are far
        // more often grey/white/black than a bold paint color) while the
        // bed/box gets its own independent random color.
        static const Color TRUCK_CAB_COLOR = {110, 112, 116, 255};
        v.model = LoadModelFromMesh(BuildVehicleMesh(v.isTruck, v.bodyColor, TRUCK_CAB_COLOR));
        v.model.materials[0].shader = shader;
        v.model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
        vehicles.push_back(v);
    }

    for (auto& v : vehicles) v.t += (v.oncoming ? -1.0f : 1.0f) * v.speed * deltaTime;
    auto offRoad = [](const Vehicle& v) { return v.oncoming ? v.t <= 0.0f : v.t >= 1.0f; };
    for (auto& v : vehicles) {
        if (offRoad(v)) UnloadModel(v.model);
    }
    vehicles.erase(std::remove_if(vehicles.begin(), vehicles.end(), offRoad), vehicles.end());
}

Vector3 TherapistWindowEffect::roadPointAt(float t) const {
    // Climbs up and away: starts close/low (near the camera, low Y), rises
    // in Y and recedes in Z as it crests the hill, curving slightly in X so
    // it doesn't read as a perfectly straight highway. Offset by roadOrigin
    // so repositioning that one field moves the whole road (and everything
    // built relative to it) together.
    float x = roadOrigin.x + 2.5f + t * 1.5f + sinf(t * 3.0f) * 0.4f;
    float y = roadOrigin.y - 1.0f + t * 4.0f;
    float z = roadOrigin.z - 3.0f - t * 14.0f;
    return {x, y, z};
}

void TherapistWindowEffect::drawTree(const Tree& tree) const {
    float sway = sinf(tree.swayPhase) * 4.0f; // degrees, gentle
    float trunkSway = sway * 0.3f;
    // GenMeshCylinder's origin sits at its own base (not centered), so the
    // draw position IS the trunk's base -- no half-height offset needed.
    // Offset by treeOrigin so repositioning that one field moves the whole
    // stand together, same idea as roadOrigin for the driving scene.
    Vector3 trunkPos = Vector3Add(tree.basePos, treeOrigin);
    DrawModelEx(trunkModel, trunkPos, {0, 0, 1}, trunkSway,
        {0.35f, tree.trunkHeight, 0.35f}, Color{90, 65, 45, 255});

    // Foliage follows the trunk's actual rotated top -- pivoting around the
    // same base, by the same trunkSway angle, so the canopy stays attached
    // to the trunk tip instead of drifting apart from it at this tree height.
    Vector3 trunkTopLocal = {0.0f, tree.trunkHeight, 0.0f};
    Vector3 trunkTopWorld = Vector3Add(trunkPos,
        Vector3RotateByAxisAngle(trunkTopLocal, {0, 0, 1}, trunkSway * DEG2RAD));
    Vector3 foliagePos = {
        trunkTopWorld.x,
        trunkTopWorld.y + tree.foliageRadius * 0.7f,
        trunkTopWorld.z
    };
    DrawModelEx(foliageModel, foliagePos, {0, 1, 0}, sway,
        {tree.foliageRadius, tree.foliageRadius * 0.9f, tree.foliageRadius}, Color{55, 110, 55, 255});
}

void TherapistWindowEffect::drawRoad() const {
    // Road surface: a strip of short segments following roadPointAt() so it
    // can curve/climb, rather than one flat stretched quad.
    static const int SEGMENTS = 24;
    Color roadColor = {60, 60, 65, 255};
    Color laneColor = {220, 200, 120, 255};
    for (int i = 0; i < SEGMENTS; i++) {
        float t0 = (float)i / SEGMENTS;
        float t1 = (float)(i + 1) / SEGMENTS;
        Vector3 p0 = roadPointAt(t0);
        Vector3 p1 = roadPointAt(t1);
        float roadWidth = 1.4f - t0 * 0.9f; // narrows with distance
        Vector3 right = {1.0f, 0.0f, 0.0f};
        Vector3 a0 = Vector3Add(p0, Vector3Scale(right, -roadWidth * 0.5f));
        Vector3 a1 = Vector3Add(p0, Vector3Scale(right, roadWidth * 0.5f));
        Vector3 b0 = Vector3Add(p1, Vector3Scale(right, -roadWidth * 0.5f * 0.9f));
        Vector3 b1 = Vector3Add(p1, Vector3Scale(right, roadWidth * 0.5f * 0.9f));
        DrawTriangle3D(a0, a1, b1, roadColor);
        DrawTriangle3D(a0, b1, b0, roadColor);

        if (i % 2 == 0) {
            Vector3 m0 = Vector3Lerp(a0, a1, 0.5f);
            Vector3 m1 = Vector3Lerp(b0, b1, 0.5f);
            DrawLine3D(m0, m1, laneColor);
        }
    }
}

void TherapistWindowEffect::drawBackdrop() const {
    // A big flat green quad, independent of roadOrigin/treeOrigin -- centered
    // on backdropOrigin (X, Y, Z), wide enough to stay visible behind BOTH
    // windows (left trees, right road) at once regardless of where those
    // are individually dialed in, or how the debug camera pans/zooms.
    static const float HALF_WIDTH = 30.0f;
    static const float HALF_HEIGHT = 8.0f;
    Color grassColor = {60, 120, 55, 255};

    float z = backdropOrigin.z;
    Vector3 a = {backdropOrigin.x - HALF_WIDTH, backdropOrigin.y - HALF_HEIGHT, z};
    Vector3 b = {backdropOrigin.x + HALF_WIDTH, backdropOrigin.y - HALF_HEIGHT, z};
    Vector3 c = {backdropOrigin.x + HALF_WIDTH, backdropOrigin.y + HALF_HEIGHT, z};
    Vector3 d = {backdropOrigin.x - HALF_WIDTH, backdropOrigin.y + HALF_HEIGHT, z};
    DrawTriangle3D(a, b, c, grassColor);
    DrawTriangle3D(a, c, d, grassColor);
}

void TherapistWindowEffect::drawVehicle(const Vehicle& v) const {
    Vector3 pos = roadPointAt(v.t);
    // Facing direction follows actual travel direction -- outbound looks
    // toward increasing t (up the hill), oncoming toward decreasing t (down
    // the hill), so oncoming traffic visibly faces the opposite way.
    float aheadT = v.oncoming ? fmaxf(v.t - 0.02f, 0.0f) : fminf(v.t + 0.02f, 1.0f);
    Vector3 aheadPos = roadPointAt(aheadT);
    Vector3 dir = Vector3Normalize(Vector3Subtract(aheadPos, pos));

    // Offset into the right-hand lane (outbound) or left-hand lane
    // (oncoming) instead of driving down the road's centerline -- same
    // roadWidth-at-this-t formula drawRoad() uses, so the lane offset
    // narrows with distance the same way the road itself does.
    float roadWidth = 1.4f - v.t * 0.9f;
    Vector3 right = {1.0f, 0.0f, 0.0f};
    float laneSign = v.oncoming ? -1.0f : 1.0f;
    pos = Vector3Add(pos, Vector3Scale(right, roadWidth * 0.25f * laneSign));
    // DrawModelEx's Y-axis rotation maps local +X to world (cos, 0, -sin)
    // for angle theta -- so matching a world direction (dx, dz) needs
    // theta = atan2(-dz, dx). The vehicle meshes' own "nose" (hood/cab end)
    // sits at local -X, not +X (opposite of what was assumed when this was
    // first wired up -- that's what had every car/truck driving up the hill
    // backwards), so add 180 degrees to point the actual nose forward.
    float yawDeg = atan2f(-dir.z, dir.x) * RAD2DEG + 180.0f;

    // BuildVehicleMesh's own unit size already reads as car/truck-sized with
    // wheels resting on local Y=0 -- baseScale picks the actual on-road size
    // (trucks a bit bigger), narrowing further with distance the same way
    // the old stretched-cube version did.
    float baseScale = v.isTruck ? 0.55f : 0.42f;
    float scale = baseScale * (1.0f - v.t * 0.7f);
    DrawModelEx(v.model, pos, {0, 1, 0}, yawDeg, {scale, scale, scale}, WHITE);
}

void TherapistWindowEffect::drawBackground() {
    Vector3 camPos = {0.0f, 0.0f, debugCamDist};
    Vector3 forward = {0.0f, 0.0f, -debugCamDist};
    Vector3 right = {1.0f, 0.0f, 0.0f};
    forward = Vector3RotateByAxisAngle(forward, right, debugPitchDeg * DEG2RAD);

    Camera3D cam3d = {};
    cam3d.position   = camPos;
    cam3d.target     = Vector3Add(camPos, forward);
    cam3d.up         = {0.0f, 1.0f, 0.0f};
    cam3d.fovy       = debugFovyDeg;
    cam3d.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(cam3d);
        for (const auto& t : trees) drawTree(t);
        drawBackdrop();
        drawRoad();
        for (const auto& v : vehicles) drawVehicle(v);
    EndMode3D();
}

void TherapistWindowEffect::cleanup() {
    UnloadModel(trunkModel);
    UnloadModel(foliageModel);
    for (auto& v : vehicles) UnloadModel(v.model);
    vehicles.clear();
    UnloadTexture(whiteTex);
    UnloadShader(shader);
}
