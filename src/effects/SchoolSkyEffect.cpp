// RLIGHTS_IMPLEMENTATION is already provided once by MoonEffect.cpp -- this
// file only needs rlights.h's declarations (Light/CreateLight/UpdateLightValues).
#include "SchoolSkyEffect.hpp"
#include "JetMesh.hpp"
#include "LitShader.hpp"

#include "raymath.h"
#include <cmath>

static Model MakeLitModel(Mesh mesh, Shader shader, Color tint) {
    Model m = LoadModelFromMesh(mesh);
    m.materials[0].shader = shader;
    m.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = tint;
    return m;
}

void SchoolSkyEffect::init() {
    shader = LoadLitShader();
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int ambLoc = GetShaderLocation(shader, "ambient");
    float amb[4] = {0.5f, 0.5f, 0.55f, 1.0f};
    SetShaderValue(shader, ambLoc, amb, SHADER_UNIFORM_VEC4);

    // Shared solid-white texture so every material's texelColor is opaque
    // white and the tint color alone determines what's drawn -- same trick
    // MoonEffect uses.
    Image white = GenImageColor(1, 1, WHITE);
    whiteTex = LoadTextureFromImage(white);
    UnloadImage(white);

    // Full detailed jet model shared with Model3DTestScene (JetMesh.hpp) --
    // replaces the old 4-primitive stand-in built directly in this file.
    Mesh jetMesh = BuildJetMesh();
    jetModel = LoadModelFromMesh(jetMesh);
    jetModel.materials[0].shader = shader;
    jetModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    jetModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    cloudPuff = MakeLitModel(GenMeshSphere(0.3f, 8, 8), shader, Color{255, 255, 255, 255});
    cloudPuff.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

    // Directional light: from the camera's side, up and behind, angled down
    // and away at 45 degrees -- position is up+toward-camera, target is
    // down+away, so the light vector (position->target) points forward-down.
    light = CreateLight0(LIGHT_DIRECTIONAL, {0.0f, 4.0f, 6.0f}, {0.0f, -4.0f, -6.0f}, WHITE, shader);

    // A pool of clouds, each a small cluster of overlapping puffs so they
    // read as a cloud silhouette rather than a single sphere. Scattered
    // across the visible width/depth at start (not all off one edge), then
    // drift left and recycle off the right edge -- see RespawnCloud().
    clouds.resize(10);
    for (auto& cloud : clouds) {
        RespawnCloud(cloud, false);
    }
}

void SchoolSkyEffect::update(float deltaTime) {
    jetT += deltaTime / jetLoopDuration;
    if (jetT > 1.0f) {
        jetT -= 1.0f;
        contrailPoints.clear();
    }
    rockTimer += deltaTime;

    Vector3 jetPos = Vector3Lerp(jetStart, jetEnd, jetT);

    contrailTimer += deltaTime;
    if (contrailTimer > 0.05f) {
        contrailTimer = 0.0f;
        contrailPoints.push_back(jetPos);
        if (contrailPoints.size() > 90) contrailPoints.erase(contrailPoints.begin());
    }

    // Clouds drift gently left; once one has gone far enough past the left
    // edge that it can't possibly still be visible, recycle it back in off
    // the right edge with a fresh randomized position/puff layout.
    static const float CLOUD_DESPAWN_X = -6.0f;
    for (auto& cloud : clouds) {
        cloud.basePos.x -= cloud.driftSpeed * deltaTime;
        if (cloud.basePos.x < CLOUD_DESPAWN_X) {
            RespawnCloud(cloud, true);
        }
    }

    float camPos[3] = {0.0f, 0.0f, 10.0f};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
    UpdateLightValues(shader, light);
}

void SchoolSkyEffect::RespawnCloud(CloudInstance& cloud, bool offscreenRight) {
    // X: either scattered across the visible width (initial placement) or
    // placed just past the right edge (recycling one that drifted off-left).
    // Recycle range is as wide as the initial scatter (6.5 to 12.5, matching
    // the 6-unit spread of -5.5 to 5.5) -- a narrow fixed recycle band made
    // every cloud funnel through the same ~1.5-unit slot on respawn, so after
    // the first cycle they'd drift in near-lockstep and clump/gap instead of
    // staying naturally staggered like the initial scatter.
    float x = offscreenRight ? (6.5f + (float)(GetRandomValue(0, 600)) / 100.0f)
                              : (float)(GetRandomValue(-550, 550)) / 100.0f;
    // The 3D camera (fovy=28 at z=10) always renders into the FULL 1280x720
    // canvas regardless of where the 2D SceneCamera is panned/zoomed to at
    // that moment -- the 2D camera crops a window out of this canvas after
    // the fact, it doesn't affect the 3D sky render itself. So this range
    // only needs to stay within the actual sky band of that fixed canvas:
    // above schoolbg.png's roofline (~3D-Y -0.6) and below the top edge
    // (~3D-Y +2.6). 0.0 to 1.6 keeps clouds comfortably inside that band
    // instead of clustering near the top edge (previous 0.5-2.5 put too much
    // of the range right at the frame's very top, above where you'd ever
    // actually see it once the 2D camera crops down toward gameplay zoom).
    float y = 0.0f + (float)(GetRandomValue(0, 160)) / 100.0f;
    float z = -0.8f - (float)(GetRandomValue(0, 180)) / 100.0f; // -0.8 to -2.6, some forward/back depth variance
    cloud.basePos = {x, y, z};
    cloud.driftSpeed = 0.05f + (float)(GetRandomValue(0, 60)) / 1000.0f;  // 0.05-0.11 u/s, gentle and varied

    int puffCount = GetRandomValue(3, 4);
    cloud.puffOffsets.clear();
    cloud.puffScales.clear();
    cloud.puffOffsets.push_back({0, 0, 0});
    cloud.puffScales.push_back(1.0f);
    for (int i = 1; i < puffCount; i++) {
        float ox = (float)(GetRandomValue(-30, 30)) / 100.0f;
        float oy = (float)(GetRandomValue(-2, 18)) / 100.0f;
        float oz = (float)(GetRandomValue(-6, 6)) / 100.0f;
        cloud.puffOffsets.push_back({ox, oy, oz});
        cloud.puffScales.push_back(0.55f + (float)(GetRandomValue(0, 25)) / 100.0f);
    }
}

void SchoolSkyEffect::drawCloud(const CloudInstance& cloud) const {
    for (size_t i = 0; i < cloud.puffOffsets.size(); i++) {
        Vector3 pos = Vector3Add(cloud.basePos, cloud.puffOffsets[i]);
        float s = cloud.puffScales[i];
        // Flattened on Y so puffs read as cloud-shaped rather than round balls.
        DrawModelEx(cloudPuff, pos, {0, 1, 0}, 0.0f, {s, s * 0.6f, s}, WHITE);
    }
}

void SchoolSkyEffect::drawBackground() {
    Camera3D cam3d = {};
    cam3d.position   = {0.0f, 0.0f, 10.0f};
    cam3d.target     = {0.0f, 0.0f, 0.0f};
    cam3d.up         = {0.0f, 1.0f, 0.0f};
    cam3d.fovy       = 28.0f;
    cam3d.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(cam3d);
        Vector3 jetPos = Vector3Lerp(jetStart, jetEnd, jetT);
        // Gentle rock/bank around the roll axis (X, the direction of travel) --
        // a slow sine wave, not real flight dynamics, just enough motion to
        // keep it from looking like a static cardboard cutout sliding past.
        // Two layered sine waves (a slow deep bank + a faster smaller
        // wobble) read as more alive than one pure sine, per feedback that
        // this needed to bank harder and more often.
        float rockAngle = sinf(rockTimer * 1.1f) * 22.0f + sinf(rockTimer * 2.3f) * 6.0f;
        DrawModelEx(jetModel, jetPos, {1, 0, 0}, rockAngle, {1, 1, 1}, WHITE);

        // Fading contrail: oldest points are fainter. DrawLine3D has no width
        // control, so apparent thickness comes from drawing several parallel
        // offset strands (vertical + horizontal jitter) instead of one hairline --
        // reads as a real contrail bloom rather than a thin wire.
        static const Vector3 STRAND_OFFSETS[] = {
            {0.0f, 0.0f, 0.0f}, {0.0f, 0.05f, 0.0f}, {0.0f, -0.05f, 0.0f},
            {0.0f, 0.0f, 0.05f}, {0.0f, 0.0f, -0.05f},
        };
        for (size_t i = 1; i < contrailPoints.size(); i++) {
            float age = (float)i / (float)contrailPoints.size();
            unsigned char alpha = (unsigned char)(220.0f * age);
            Color trailColor = {255, 255, 255, alpha};
            for (const Vector3& off : STRAND_OFFSETS) {
                DrawLine3D(Vector3Add(contrailPoints[i - 1], off), Vector3Add(contrailPoints[i], off), trailColor);
            }
        }

        for (const auto& cloud : clouds) drawCloud(cloud);
    EndMode3D();
}

void SchoolSkyEffect::cleanup() {
    UnloadModel(jetModel);
    UnloadTexture(whiteTex);
    UnloadModel(cloudPuff);
    UnloadShader(shader);
}
