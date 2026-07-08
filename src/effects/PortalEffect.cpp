#include "PortalEffect.hpp"
#include "LitShader.hpp"
#include "PortalShader.hpp"
#include "MeshBuilders.hpp"
#include "raymath.h"
#include <cstring>
#include <cmath>
#include <vector>

// Ring frame mesh: a hoop built from a loop of rectangular-cross-section
// segments (each segment a small box between two points on the circle),
// plus a pedestal block anchoring it to the floor. Hand-built rather than
// GenMeshTorus so the cross-section can flare per-side later if wanted, and
// to stay consistent with the JetMesh hand-mesh convention project-wide.
Mesh BuildRingFrameMesh(float radius, float tubeThickness, int segments) {
    std::vector<float> verts, normals, uvs;
    std::vector<unsigned char> colors;

    static const Color FRAME_METAL = {70, 74, 82, 255};
    static const Color FRAME_ACCENT = {110, 200, 220, 255};

    // Ring: a closed loop of short boxes, each a small quad-prism between
    // consecutive points around the circle. Every 4th segment gets the
    // accent color as a simple "panel line" detail.
    float half = tubeThickness * 0.5f;
    for (int i = 0; i < segments; i++) {
        float a0 = (float)i / segments * 2.0f * PI;
        float a1 = (float)(i + 1) / segments * 2.0f * PI;
        Vector3 p0 = {cosf(a0) * radius, sinf(a0) * radius, 0.0f};
        Vector3 p1 = {cosf(a1) * radius, sinf(a1) * radius, 0.0f};

        // Local outward/tangent basis for this segment so the box has real
        // width (radial) and depth (through the ring, +Z) instead of being
        // a flat ribbon.
        Vector3 outward0 = Vector3Normalize(p0);
        Vector3 outward1 = Vector3Normalize(p1);

        Color segColor = (i % 4 == 0) ? FRAME_ACCENT : FRAME_METAL;

        Vector3 p0Out = Vector3Add(p0, Vector3Scale(outward0, half));
        Vector3 p0In  = Vector3Subtract(p0, Vector3Scale(outward0, half));
        Vector3 p1Out = Vector3Add(p1, Vector3Scale(outward1, half));
        Vector3 p1In  = Vector3Subtract(p1, Vector3Scale(outward1, half));

        Vector3 front = {0, 0, half};
        Vector3 back  = {0, 0, -half};

        // Front face (+Z)
        PushQuad(verts, normals, uvs, colors,
            Vector3Add(p0Out, front), Vector3Add(p1Out, front),
            Vector3Add(p1In, front), Vector3Add(p0In, front), segColor);
        // Back face (-Z)
        PushQuad(verts, normals, uvs, colors,
            Vector3Add(p0In, back), Vector3Add(p1In, back),
            Vector3Add(p1Out, back), Vector3Add(p0Out, back), segColor);
        // Outer rim face
        PushQuad(verts, normals, uvs, colors,
            Vector3Add(p0Out, back), Vector3Add(p1Out, back),
            Vector3Add(p1Out, front), Vector3Add(p0Out, front), segColor);
        // Inner rim face (faces the membrane)
        PushQuad(verts, normals, uvs, colors,
            Vector3Add(p0In, front), Vector3Add(p1In, front),
            Vector3Add(p1In, back), Vector3Add(p0In, back), segColor);
    }

    // Pedestal: a simple tapered block under the ring, wide at the floor,
    // narrowing up to where it meets the ring's bottom -- anchors the
    // device to the ground instead of it looking like it's floating.
    float pedTop = -radius - half;
    float pedBottom = pedTop - 1.1f;
    float pedHalfWideBottom = radius * 0.55f;
    float pedHalfWideTop = radius * 0.22f;
    float pedHalfDepth = tubeThickness * 1.4f;

    Vector3 b0 = {-pedHalfWideBottom, pedBottom, -pedHalfDepth};
    Vector3 b1 = { pedHalfWideBottom, pedBottom, -pedHalfDepth};
    Vector3 b2 = { pedHalfWideBottom, pedBottom,  pedHalfDepth};
    Vector3 b3 = {-pedHalfWideBottom, pedBottom,  pedHalfDepth};
    Vector3 t0 = {-pedHalfWideTop, pedTop, -pedHalfDepth};
    Vector3 t1 = { pedHalfWideTop, pedTop, -pedHalfDepth};
    Vector3 t2 = { pedHalfWideTop, pedTop,  pedHalfDepth};
    Vector3 t3 = {-pedHalfWideTop, pedTop,  pedHalfDepth};

    static const Color PEDESTAL_COLOR = {50, 52, 58, 255};
    // Winding reversed from the first pass -- every face's normal was
    // pointing inward (verified by computing each quad's (b-a)x(c-a)), so
    // the whole pedestal was lit as if seen from inside itself.
    PushQuad(verts, normals, uvs, colors, t0, t1, b1, b0, PEDESTAL_COLOR); // front
    PushQuad(verts, normals, uvs, colors, t2, t3, b3, b2, PEDESTAL_COLOR); // back
    PushQuad(verts, normals, uvs, colors, t1, t2, b2, b1, PEDESTAL_COLOR); // right
    PushQuad(verts, normals, uvs, colors, t3, t0, b0, b3, PEDESTAL_COLOR); // left
    PushQuad(verts, normals, uvs, colors, t3, t2, t1, t0, PEDESTAL_COLOR); // top

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

    Mesh ringMesh = BuildRingFrameMesh(RING_RADIUS, RING_TUBE_THICKNESS, RING_SEGMENTS);
    ringModel = LoadModelFromMesh(ringMesh);
    ringModel.materials[0].shader = litShader;
    ringModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;

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
    Vector3 scaleVec = {objectScale, objectScale, objectScale};

    // Membrane sits slightly forward of the ring's own plane (local +Z) so
    // it never z-fights the inner rim face -- that offset has to be rotated
    // by the same yaw as the ring, or the membrane would drift sideways out
    // of the ring's opening as the device turns instead of staying centered
    // in it.
    Vector3 membraneLocalOffset = {0.0f, 0.0f, RING_TUBE_THICKNESS * 0.3f};
    Vector3 membraneOffset = Vector3RotateByAxisAngle(membraneLocalOffset, yawAxis, objectYawDeg * DEG2RAD);
    Vector3 membranePos = Vector3Scale(membraneOffset, objectScale);

    BeginMode3D(cam3d);
        DrawModelEx(ringModel, {0.0f, 0.0f, 0.0f}, yawAxis, objectYawDeg, scaleVec, WHITE);
        DrawModelEx(membraneModel, membranePos, yawAxis, objectYawDeg, scaleVec, WHITE);
    EndMode3D();
}

void PortalEffect::cleanup() {
    UnloadModel(ringModel);
    UnloadModel(membraneModel);
    UnloadTexture(whiteTex);
    UnloadShader(litShader);
    UnloadShader(portalShader);
}
