#include "SunEffect.hpp"
#include "LitShader.hpp"
#include "raymath.h"
#include <cmath>

void SunEffect::init() {
    shader = LoadLitShader();
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int ambLoc = GetShaderLocation(shader, "ambient");
    float amb[4] = {0.4f, 0.4f, 0.5f, 1.0f};
    SetShaderValue(shader, ambLoc, amb, SHADER_UNIFORM_VEC4);

    // Same low-poly sphere/lit-shader rig as MoonEffect, but MoonEffect's
    // radius (0.2) was tuned for its own tight/close camera ({0,0,10},
    // fovy 10) -- this effect's camera sits much farther back with a much
    // wider fovy (debugCamDist/debugFovyDeg default 25/32, matching
    // TherapistWindowEffect's window-framing camera), so the same radius
    // would be a barely-visible speck. Scaled up to actually read at this
    // camera's distance/fovy.
    sunModel = LoadModelFromMesh(GenMeshSphere(2.5f, 6, 6));
    sunModel.materials[0].shader = shader;
    Image white = GenImageColor(1, 1, WHITE);
    whiteTex = LoadTextureFromImage(white);
    UnloadImage(white);
    sunModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    sunModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {255, 200, 90, 255};

    // Same light rig as TherapistWindowEffect but with X negated on both
    // position and target -- flips the yaw so it comes from top-right and
    // points toward bottom-left instead of top-left/bottom-right.
    light = CreateLight0(LIGHT_DIRECTIONAL, {2.0f, 4.0f, 5.0f}, {-1.5f, -3.0f, -2.0f}, WHITE, shader);

    // Pine tree: unit trunk cylinder + unit cone (tiers scaled per-tier at
    // draw time), same shader/light rig as the sun.
    trunkModel = LoadModelFromMesh(GenMeshCylinder(0.5f, 1.0f, 8));
    trunkModel.materials[0].shader = shader;
    trunkModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

    tierModel = LoadModelFromMesh(GenMeshCone(1.0f, 1.0f, 8));
    tierModel.materials[0].shader = shader;
    tierModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
}

void SunEffect::update(float deltaTime) {
    rotation += 18.0f * deltaTime;
    UpdateLightValues(shader, light);
    float camPos[3] = {0.0f, 0.0f, debugCamDist};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
}

void SunEffect::drawBackground() {
    // Lens-shift the camera target toward the door glass cutout in
    // parlorbg.png (bbox roughly x:107-253, y:100-362, center (180,231) vs
    // the 1280x720 canvas center (640,360)) -- same technique
    // CityWindowEffect uses for apartmentbg.png's window: a shift of the
    // look direction (not a world offset), so it stays aligned at every
    // depth the sun gets dialed to.
    float offsetXFrac = (180.0f - 640.0f) / 1280.0f;
    float offsetYFrac = (231.0f - 360.0f) / 720.0f;
    float camFovy = debugFovyDeg;
    float camDist = debugCamDist;
    float halfH = tanf(camFovy * 0.5f * DEG2RAD) * camDist;
    float halfW = halfH * (1280.0f / 720.0f);
    float targetX = offsetXFrac * 2.0f * halfW;
    float targetY = -offsetYFrac * 2.0f * halfH; // screen Y-down -> world Y-up

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
        DrawModelEx(sunModel, sunOrigin, {0.0f, 1.0f, 0.3f}, rotation,
            {1.0f, 1.0f, 1.0f}, {255, 200, 90, 255});

        // Trunk: base at treeOrigin (GenMeshCylinder's own origin sits at its
        // base, not centered -- see TherapistWindowEffect's tree trunks for
        // the same convention).
        static const Color TRUNK_COLOR = {60, 40, 25, 255};
        static const float TRUNK_HEIGHT = 1.2f;
        DrawModelEx(trunkModel, treeOrigin, {0, 1, 0}, 0.0f,
            {0.5f, TRUNK_HEIGHT, 0.5f}, TRUNK_COLOR);

        // Stacked, shrinking cone tiers on top of the trunk, each overlapping
        // the one below so no gap shows -- same silhouette as a classic
        // low-poly pine (widest tier at the bottom, narrowing toward the tip).
        static const Color FOLIAGE_COLOR = {45, 90, 40, 255};
        static const int TIER_COUNT = 6;
        static const float TIER_RADIUS_BASE = 2.0f;
        static const float TIER_HEIGHT = 1.6f;
        static const float TIER_OVERLAP = 0.55f;   // fraction of a tier's height the next one sinks into it
        float tierY = treeOrigin.y + TRUNK_HEIGHT;
        for (int i = 0; i < TIER_COUNT; i++) {
            float shrink = 1.0f - (float)i / TIER_COUNT * 0.7f;
            float radius = TIER_RADIUS_BASE * shrink;
            Vector3 tierPos = {treeOrigin.x, tierY, treeOrigin.z};
            DrawModelEx(tierModel, tierPos, {0, 1, 0}, 0.0f,
                {radius, TIER_HEIGHT, radius}, FOLIAGE_COLOR);
            tierY += TIER_HEIGHT * (1.0f - TIER_OVERLAP);
        }
    EndMode3D();
}

void SunEffect::cleanup() {
    UnloadTexture(whiteTex);
    UnloadShader(shader);
    UnloadModel(sunModel);
    UnloadModel(trunkModel);
    UnloadModel(tierModel);
}
