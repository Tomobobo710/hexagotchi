// RLIGHTS_IMPLEMENTATION is already provided once by MoonEffect.cpp -- this
// file only needs rlights.h's declarations (Light/CreateLight/UpdateLightValues).
#include "SchoolSkyEffect.hpp"
#include "JetMesh.hpp"
#include "LitShader.hpp"

#include "raymath.h"
#include <cmath>

// Cloud loop range: despawn once a cloud drifts past CLOUD_DESPAWN_X off the
// left edge; recycled/initial clouds spawn somewhere in
// [CLOUD_DESPAWN_X, CLOUD_SPAWN_X_MAX] off the right edge. Shared by init()'s
// even initial spacing and RespawnCloud()'s recycle placement so both agree
// on the same loop span.
//
// Sized against the CURRENT camera framing (debugCamDist 4, pitch -5deg,
// fovy 28), whose visible frustum at the clouds' depth is only ~2.4 units
// wide -- this span is wider than that so clouds still drift in/out past the
// edges instead of popping, but nowhere near the old 8.5-wide span that was
// tuned for a since-changed dist-10 camera and left most of the loop
// permanently off-frame.
static const float CLOUD_DESPAWN_X = -2.5f;
static const float CLOUD_SPAWN_X_MAX = 2.5f;

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
    // read as a cloud silhouette rather than a single sphere. Evenly spaced
    // across the full loop span at start (not randomly scattered -- random
    // scatter let RNG clump several on one side and leave the rest empty),
    // then drift left at a shared fixed speed and recycle off the right edge
    // -- see RespawnCloud(). Even spacing + a shared speed means they never
    // bunch up or gap out over time, unlike the old per-cloud random speed.
    clouds.resize(10);
    float slotWidth = (CLOUD_SPAWN_X_MAX - CLOUD_DESPAWN_X) / (float)clouds.size();
    for (size_t i = 0; i < clouds.size(); i++) {
        RespawnCloud(clouds[i]);
        clouds[i].basePos.x = CLOUD_DESPAWN_X + slotWidth * ((float)i + 0.5f);
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
        contrailPoints.push_back(Vector3Add(jetPos, jetOrigin));
        if (contrailPoints.size() > 90) contrailPoints.erase(contrailPoints.begin());
    }

    // Clouds drift gently left; once one has gone far enough past the left
    // edge that it can't possibly still be visible, recycle it back in off
    // the right edge, placed one slot-width beyond whichever cloud is
    // currently furthest right -- keeps the even spacing from init()
    // permanently instead of just for the first lap (a fixed/random
    // off-screen respawn band would let spacing drift once clouds are no
    // longer all recycling in the same order they started in).
    float slotWidth = (CLOUD_SPAWN_X_MAX - CLOUD_DESPAWN_X) / (float)clouds.size();
    for (auto& cloud : clouds) {
        cloud.basePos.x -= cloud.driftSpeed * deltaTime;
        if (cloud.basePos.x < CLOUD_DESPAWN_X) {
            float maxX = CLOUD_SPAWN_X_MAX - slotWidth;
            for (const auto& other : clouds) {
                if (other.basePos.x > maxX) maxX = other.basePos.x;
            }
            RespawnCloud(cloud);
            cloud.basePos.x = maxX + slotWidth;
        }
    }

    float camPos[3] = {0.0f, 0.0f, 10.0f};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
    UpdateLightValues(shader, light);
}

void SchoolSkyEffect::RespawnCloud(CloudInstance& cloud) {
    // X is NOT set here -- both call sites (init()'s even initial spacing and
    // update()'s "one slot-width past the current rightmost cloud" recycle
    // placement) set basePos.x themselves right after calling this, so every
    // cloud stays evenly spaced across the loop permanently instead of only
    // for the first lap. This only randomizes what varies cloud-to-cloud
    // within the existing band: depth/height/puff shape/fixed drift speed.
    //
    // The 3D camera (fovy=28 at z=10) always renders into the FULL 1280x720
    // canvas regardless of where the 2D SceneCamera is panned/zoomed to at
    // that moment -- the 2D camera crops a window out of this canvas after
    // the fact, it doesn't affect the 3D sky render itself. So this range
    // only needs to stay within the actual sky band of that fixed canvas:
    // Camera is now much closer/tighter than when this band was tuned
    // (debugCamDist 4, pitch -5deg, fovy 28 -- was dist 10, pitch 0). At that
    // framing the visible frustum at the clouds' depth is only ~2.4 units
    // wide and ~1.4 tall, so the old X/Y/Z band (sized for a dist-10 camera)
    // put almost the entire cloud path outside the frame -- only whichever
    // cloud happened to be crossing dead-center was ever visible. Rescaled
    // to actually sit inside the current frustum, with the vertical center
    // nudged down slightly to account for the downward pitch.
    float y = -0.3f + (float)(GetRandomValue(0, 140)) / 100.0f;  // -0.3 to 1.1
    float z = -0.8f - (float)(GetRandomValue(0, 180)) / 100.0f; // -0.8 to -2.6, some forward/back depth variance
    cloud.basePos.y = y;
    cloud.basePos.z = z;
    // Fixed shared speed (was randomized 0.05-0.11 per cloud) -- combined
    // with even spacing that never gets reshuffled, this is what keeps the
    // sky from clumping/gapping over time; a shared speed can't let one
    // cloud catch up to or fall behind another.
    cloud.driftSpeed = 0.08f;

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
        Vector3 pos = Vector3Add(Vector3Add(cloud.basePos, cloudOrigin), cloud.puffOffsets[i]);
        float s = cloud.puffScales[i];
        // Flattened on Y so puffs read as cloud-shaped rather than round balls.
        DrawModelEx(cloudPuff, pos, {0, 1, 0}, 0.0f, {s, s * 0.6f, s}, WHITE);
    }
}

void SchoolSkyEffect::drawBackground() {
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
        Vector3 jetPos = Vector3Add(Vector3Lerp(jetStart, jetEnd, jetT), jetOrigin);
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

