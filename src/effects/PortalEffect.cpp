#include "PortalEffect.hpp"
#include "LitShader.hpp"
#include "PortalShader.hpp"
#include "PortalRingMesh.hpp"
#include "PortalBaseMesh.hpp"
#include "MeshBuilders.hpp"
#include "raymath.h"
#include <cstring>
#include <cmath>
#include <vector>

// Membrane: a single quad filling the ring's inner radius, UV 0..1 across
// it so PortalShader can compute a centered radial pattern.
Mesh BuildMembraneMesh(float innerRadius) {
    std::vector<float> verts, normals, uvs;
    std::vector<unsigned char> colors;

    // Built by hand (not PushQuad) since PushQuad always emits UV (0,0) --
    // fine for the lit meshes, which don't sample UVs, but PortalShader
    // genuinely needs a real 0..1 UV per corner to compute a centered radial
    // coordinate.
    float r = innerRadius;
    Vector3 p0 = {-r, -r, 0.0f}, uv0 = {0, 0, 0};
    Vector3 p1 = { r, -r, 0.0f}, uv1 = {1, 0, 0};
    Vector3 p2 = { r,  r, 0.0f}, uv2 = {1, 1, 0};
    Vector3 p3 = {-r,  r, 0.0f}, uv3 = {0, 1, 0};
    Vector3 tri[6]   = {p0, p1, p2, p0, p2, p3};
    Vector3 triUV[6] = {uv0, uv1, uv2, uv0, uv2, uv3};
    Vector3 n = {0, 0, 1};
    for (int i = 0; i < 6; i++) {
        verts.push_back(tri[i].x); verts.push_back(tri[i].y); verts.push_back(tri[i].z);
        normals.push_back(n.x); normals.push_back(n.y); normals.push_back(n.z);
        uvs.push_back(triUV[i].x); uvs.push_back(triUV[i].y);
        colors.push_back(255); colors.push_back(255); colors.push_back(255); colors.push_back(255);
    }

    Mesh mesh = {};
    mesh.triangleCount = (int)(verts.size() / 9);
    mesh.vertexCount = mesh.triangleCount * 3;
    mesh.vertices = (float*)RL_MALLOC(verts.size() * sizeof(float));
    mesh.normals = (float*)RL_MALLOC(normals.size() * sizeof(float));
    mesh.texcoords = (float*)RL_MALLOC(uvs.size() * sizeof(float));
    mesh.colors = (unsigned char*)RL_MALLOC(colors.size() * sizeof(unsigned char));
    memcpy(mesh.vertices, verts.data(), verts.size() * sizeof(float));
    memcpy(mesh.normals, normals.data(), normals.size() * sizeof(float));
    memcpy(mesh.texcoords, uvs.data(), uvs.size() * sizeof(float));
    memcpy(mesh.colors, colors.data(), colors.size() * sizeof(unsigned char));
    UploadMesh(&mesh, false);
    return mesh;
}

const float RING_RADIUS = 1.3f;
const float RING_TUBE_THICKNESS = 0.16f;
const int RING_SEGMENTS = 28;

void PortalEffect::init() {
    litShader = LoadLitShader();
    litShader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(litShader, "viewPos");
    int ambLoc = GetShaderLocation(litShader, "ambient");
    float amb[4] = {0.3f, 0.32f, 0.38f, 1.0f};
    SetShaderValue(litShader, ambLoc, amb, SHADER_UNIFORM_VEC4);

    Image white = GenImageColor(1, 1, WHITE);
    whiteTex = LoadTextureFromImage(white);
    UnloadImage(white);

    Mesh ringMesh = BuildPortalRingMesh();
    ringModel = LoadModelFromMesh(ringMesh);
    ringModel.materials[0].shader = litShader;
    ringModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

    Mesh baseMesh = BuildPortalBaseMesh();
    baseModel = LoadModelFromMesh(baseMesh);
    baseModel.materials[0].shader = litShader;
    baseModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

    // Light from above-front, matching the office's implied ceiling lighting.
    light = CreateLight0(LIGHT_DIRECTIONAL, {1.0f, 3.0f, 4.0f}, {-0.5f, -2.0f, -1.0f}, WHITE, litShader);

    portalShader = LoadPortalShader();
    portalTimeLoc = GetShaderLocation(portalShader, "time");
    portalIntensityLoc = GetShaderLocation(portalShader, "intensity");

    membraneMesh = BuildMembraneMesh(RING_RADIUS - RING_TUBE_THICKNESS * 0.5f);
    membraneModel = LoadModelFromMesh(membraneMesh);
    membraneModel.materials[0].shader = portalShader;
}

void PortalEffect::update(float deltaTime) {
    time += deltaTime;

    float camPos[3] = {0.0f, 0.0f, debugCamDist};
    SetShaderValue(litShader, litShader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
    UpdateLightValues(litShader, light);

    SetShaderValue(portalShader, portalTimeLoc, &time, SHADER_UNIFORM_FLOAT);
    SetShaderValue(portalShader, portalIntensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
}

void PortalEffect::drawBackground() {
    float camFovy = debugFovyDeg;
    float camDist = debugCamDist;

    // Camera is fixed in world space (independent of objectPosition) --
    // moving the device around the room via setObjectPosition() should look
    // like the object stays planted while the camera's view frames it
    // differently, not like the camera is chasing the object around.
    Vector3 camPos = {0.0f, 0.0f, camDist};
    Vector3 forward = {0.0f, 0.0f, -camDist};
    Vector3 right = {1.0f, 0.0f, 0.0f};
    forward = Vector3RotateByAxisAngle(forward, right, debugPitchDeg * DEG2RAD);

    Camera3D cam3d = {};
    cam3d.position   = camPos;
    cam3d.target     = Vector3Add(camPos, forward);
    cam3d.up         = {0.0f, 1.0f, 0.0f};
    cam3d.fovy       = camFovy;
    cam3d.projection = CAMERA_PERSPECTIVE;

    Vector3 yawAxis = {0.0f, 1.0f, 0.0f};
    Vector3 zAxis = {0.0f, 0.0f, 1.0f};
    Vector3 scaleVec = {objectScale, objectScale, objectScale};

    // Continuous spin around Z (the ring's own facing axis), like a flywheel
    // -- separate from the manual debug yaw (Numpad 4/6, rotates the whole
    // device to face a different direction). DrawModelEx only takes one
    // axis+angle, so combining "spin on Z" with "yaw on Y" needs a real
    // composed rotation matrix on the model's own transform instead of two
    // DrawModelEx calls with different axes.
    float spinDeg = time * ringSpinDegPerSec;
    Matrix ringSpin = MatrixRotateZ(spinDeg * DEG2RAD);
    Matrix ringYaw = MatrixRotateY(objectYawDeg * DEG2RAD);
    Matrix ringScale = MatrixScale(objectScale, objectScale, objectScale);
    Matrix ringTransform = MatrixMultiply(MatrixMultiply(ringScale, ringSpin), ringYaw);

    // Base (pedestal/staircase/pillars) gets the same yaw+scale as the ring
    // -- so the whole device still turns/resizes together -- but NOT the
    // spin, since only the ring itself should spin in place; the base stays
    // fixed under it.
    Matrix baseTransform = MatrixMultiply(ringScale, ringYaw);

    // Membrane sits slightly forward of the ring's own plane (local +Z) so
    // it never z-fights the inner rim face -- rotated/scaled by the same
    // yaw+spin as the ring so it stays centered in the ring's opening as
    // the device turns or spins.
    Vector3 membraneLocalOffset = {0.0f, 0.0f, RING_TUBE_THICKNESS * 0.3f};
    Vector3 membraneOffset = Vector3Transform(membraneLocalOffset, MatrixMultiply(ringSpin, ringYaw));
    Vector3 membranePos = Vector3Add(objectPosition, Vector3Scale(membraneOffset, objectScale));
    Matrix membraneTransform = MatrixMultiply(MatrixMultiply(ringScale, ringSpin), ringYaw);

    BeginMode3D(cam3d);
        baseModel.transform = baseTransform;
        DrawModel(baseModel, objectPosition, 1.0f, WHITE);
        ringModel.transform = ringTransform;
        DrawModel(ringModel, objectPosition, 1.0f, WHITE);
        membraneModel.transform = membraneTransform;
        DrawModel(membraneModel, membranePos, 1.0f, WHITE);
    EndMode3D();
}

void PortalEffect::cleanup() {
    UnloadModel(ringModel);
    UnloadModel(baseModel);
    UnloadModel(membraneModel);
    UnloadTexture(whiteTex);
    UnloadShader(litShader);
    UnloadShader(portalShader);
}
