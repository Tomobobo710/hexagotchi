#include "VehicleMesh.hpp"
#include "MeshBuilders.hpp"
#include <cstring>
#include <vector>

// Wheel: axle along Z (the vehicle's width axis) so the flat circular faces
// point out the sides and the tire rolls in the X-Y plane (forward/back) --
// this is the axis a real wheel actually needs; confirmed correct via the
// yaw-direction fix in TherapistWindowEffect.cpp (a tube built with its axis
// along X instead spins sideways).
static void PushWheel(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                       std::vector<unsigned char>& colors, Vector3 center, float radius, float width, Color color) {
    static const int SIDES = 12;
    std::vector<Vector3> ringOut(SIDES), ringIn(SIDES);
    float halfW = width * 0.5f;
    float outZ = center.z > 0 ? center.z + halfW : center.z - halfW;  // outer face (toward vehicle exterior)
    float inZ  = center.z > 0 ? center.z - halfW : center.z + halfW;  // inner face (toward vehicle centerline)
    for (int i = 0; i < SIDES; i++) {
        float angle = (float)i / SIDES * 2.0f * PI;
        float px = center.x + cosf(angle) * radius;
        float py = center.y + sinf(angle) * radius;
        ringOut[i] = {px, py, outZ};
        ringIn[i]  = {px, py, inZ};
    }
    bool outerIsPositive = outZ > inZ;
    // Tread: quad strip between the two rings. Numerically verified (not
    // just reasoned about): with ring order i->j going counter-clockwise
    // (increasing angle), a-b-c-d = out[j], out[i], in[i], in[j] gives a
    // radially OUTWARD normal when outerIsPositive -- the previous ordering
    // here (out[i], out[j], in[j], in[i]) was checked by hand-computing
    // cross(b-a,c-a) for i=0 and came out pointing INWARD instead, which is
    // exactly the "wheels are inverted" bug this replaces.
    for (int i = 0; i < SIDES; i++) {
        int j = (i + 1) % SIDES;
        if (outerIsPositive) {
            PushQuad(verts, normals, uvs, colors, ringOut[j], ringOut[i], ringIn[i], ringIn[j], color);
        } else {
            PushQuad(verts, normals, uvs, colors, ringIn[j], ringIn[i], ringOut[i], ringOut[j], color);
        }
    }
    // Hubcap faces: fan from the wheel's own center on each flat face, wound
    // to match the corrected tread winding above (outward-facing).
    Vector3 centerOut = {center.x, center.y, outZ};
    Vector3 centerIn  = {center.x, center.y, inZ};
    for (int i = 0; i < SIDES; i++) {
        int j = (i + 1) % SIDES;
        if (outerIsPositive) {
            PushTri(verts, normals, uvs, colors, centerOut, ringOut[i], ringOut[j], color);
            PushTri(verts, normals, uvs, colors, centerIn, ringIn[j], ringIn[i], color);
        } else {
            PushTri(verts, normals, uvs, colors, centerOut, ringOut[j], ringOut[i], color);
            PushTri(verts, normals, uvs, colors, centerIn, ringIn[i], ringIn[j], color);
        }
    }
}

// Sedan body -- hand-reshaped in Blender from the original procedural hull
// (round-tripped via tools/export_glb.cpp/build/export/humancar.glb, see
// blender-mesh-pipeline memory), then read back here as literal PushTri()
// calls with the exact reshaped vertex positions, same convention as
// PortalRingMesh.cpp/PortalBaseMesh.cpp -- not a baked data array. Wheels
// were untouched in the reshape (same PushWheel() below, same position/
// radius as the original export), only this body geometry changed.
static void PushCarBody(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                         std::vector<unsigned char>& colors, Color bodyColor) {
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, 0.4496f}, {-0.4706f, 0.9198f, 0.2994f}, {-0.9668f, 0.6238f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, 0.4496f}, {0.4840f, 0.9198f, 0.2994f}, {-0.4706f, 0.9198f, 0.2994f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.1249f, 0.5636f, 0.3793f}, {1.3453f, 0.3783f, -0.3489f}, {1.1249f, 0.5636f, -0.3793f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.1249f, 0.5636f, 0.3793f}, {1.3453f, 0.3783f, 0.3489f}, {1.3453f, 0.3783f, -0.3489f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.1249f, 0.5636f, -0.3793f}, {1.3453f, 0.1092f, -0.3489f}, {1.0513f, 0.1210f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.1249f, 0.5636f, -0.3793f}, {1.3453f, 0.3783f, -0.3489f}, {1.3453f, 0.1092f, -0.3489f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.0513f, 0.1210f, -0.4496f}, {1.3453f, 0.1092f, 0.3489f}, {1.0513f, 0.1210f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.0513f, 0.1210f, -0.4496f}, {1.3453f, 0.1092f, -0.3489f}, {1.3453f, 0.1092f, 0.3489f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.3453f, 0.1092f, 0.3489f}, {1.3453f, 0.3783f, -0.3489f}, {1.3453f, 0.3783f, 0.3489f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.3453f, 0.1092f, 0.3489f}, {1.3453f, 0.1092f, -0.3489f}, {1.3453f, 0.3783f, -0.3489f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, -0.4496f}, {-0.7957f, 0.1210f, 0.4496f}, {-0.7957f, 0.1210f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, -0.4496f}, {0.7957f, 0.1210f, 0.4496f}, {-0.7957f, 0.1210f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, -0.4496f}, {-0.9668f, 0.6238f, -0.4496f}, {0.9225f, 0.6238f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, -0.4496f}, {-0.7957f, 0.1210f, -0.4496f}, {-0.9668f, 0.6238f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, -0.4496f}, {1.0513f, 0.1210f, 0.4496f}, {0.7957f, 0.1210f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, -0.4496f}, {1.0513f, 0.1210f, -0.4496f}, {1.0513f, 0.1210f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, -0.4496f}, {1.0513f, 0.1210f, -0.4496f}, {0.7957f, 0.1210f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, -0.4496f}, {1.1249f, 0.5636f, -0.3793f}, {1.0513f, 0.1210f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, 0.4496f}, {1.1249f, 0.5636f, -0.3793f}, {0.9225f, 0.6238f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, 0.4496f}, {1.1249f, 0.5636f, 0.3793f}, {1.1249f, 0.5636f, -0.3793f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, 0.4496f}, {1.1249f, 0.5636f, 0.3793f}, {0.9225f, 0.6238f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.7957f, 0.1210f, 0.4496f}, {1.0513f, 0.1210f, 0.4496f}, {1.1249f, 0.5636f, 0.3793f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.0513f, 0.1210f, 0.4496f}, {1.3453f, 0.3783f, 0.3489f}, {1.1249f, 0.5636f, 0.3793f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {1.0513f, 0.1210f, 0.4496f}, {1.3453f, 0.1092f, 0.3489f}, {1.3453f, 0.3783f, 0.3489f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, -0.4496f}, {0.4840f, 0.9198f, -0.2994f}, {0.9225f, 0.6238f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, -0.4496f}, {-0.4706f, 0.9198f, -0.2994f}, {0.4840f, 0.9198f, -0.2994f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, 0.4496f}, {-0.4706f, 0.9198f, -0.2994f}, {-0.9668f, 0.6238f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, 0.4496f}, {-0.4706f, 0.9198f, 0.2994f}, {-0.4706f, 0.9198f, -0.2994f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.7957f, 0.1210f, 0.4496f}, {0.9225f, 0.6238f, 0.4496f}, {-0.9668f, 0.6238f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.7957f, 0.1210f, 0.4496f}, {0.7957f, 0.1210f, 0.4496f}, {0.9225f, 0.6238f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.7957f, 0.1210f, 0.4496f}, {-1.2609f, 0.1210f, -0.3856f}, {-0.7957f, 0.1210f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.7957f, 0.1210f, 0.4496f}, {-1.2609f, 0.1210f, 0.3856f}, {-1.2609f, 0.1210f, -0.3856f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, -0.4496f}, {0.4840f, 0.9198f, 0.2994f}, {0.9225f, 0.6238f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {0.9225f, 0.6238f, -0.4496f}, {0.4840f, 0.9198f, -0.2994f}, {0.4840f, 0.9198f, 0.2994f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.4706f, 0.9198f, -0.2994f}, {0.4840f, 0.9198f, 0.2994f}, {0.4840f, 0.9198f, -0.2994f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.4706f, 0.9198f, -0.2994f}, {-0.4706f, 0.9198f, 0.2994f}, {0.4840f, 0.9198f, 0.2994f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-1.2609f, 0.1210f, -0.3856f}, {-1.2609f, 0.5426f, 0.3856f}, {-1.2609f, 0.5426f, -0.3856f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-1.2609f, 0.1210f, -0.3856f}, {-1.2609f, 0.1210f, 0.3856f}, {-1.2609f, 0.5426f, 0.3856f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, 0.4496f}, {-1.2609f, 0.1210f, 0.3856f}, {-0.7957f, 0.1210f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, 0.4496f}, {-1.2609f, 0.5426f, 0.3856f}, {-1.2609f, 0.1210f, 0.3856f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.7957f, 0.1210f, -0.4496f}, {-1.2609f, 0.5426f, -0.3856f}, {-0.9668f, 0.6238f, -0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.7957f, 0.1210f, -0.4496f}, {-1.2609f, 0.1210f, -0.3856f}, {-1.2609f, 0.5426f, -0.3856f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, -0.4496f}, {-1.2609f, 0.5426f, 0.3856f}, {-0.9668f, 0.6238f, 0.4496f}, bodyColor);
    PushTri(verts, normals, uvs, colors, {-0.9668f, 0.6238f, -0.4496f}, {-1.2609f, 0.5426f, -0.3856f}, {-1.2609f, 0.5426f, 0.3856f}, bodyColor);
}

// Truck body -- hand-reshaped in Blender (build/export/humantruck.glb, same
// pipeline as the car above), then read back as literal PushTri() calls.
// Two colors instead of one: the flatbed section (bedColor, dark brown) is
// geometrically distinct from the front cab section (cabColor, grey) --
// classified here by X position (bed centroid X < -0.1, cab centroid X
// >= -0.1) since Blender's reshape didn't preserve a color split on its own
// (both came back flat white). Wheels untouched by the reshape, same
// PushWheel() as the car.
static void PushTruckBody(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                           std::vector<unsigned char>& colors, Color bedColor, Color cabColor) {
    // NOTE: unlike PushCarBody(), these are the RAW export vertex order (not
    // b/c-swapped) -- Tom confirmed the truck reshape came out with the
    // opposite winding convention from the car reshape (a separate Blender
    // edit, not guaranteed to match), so flipping it the same way the car
    // needed actually inverted it here. Trust the observed result over
    // "should be consistent with the car."
    PushTri(verts, normals, uvs, colors, {-0.3668f, 0.2606f, 0.4877f}, {-0.3668f, 1.0140f, 0.4877f}, {-0.9392f, 1.0140f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.3668f, 0.2606f, 0.4877f}, {-0.9392f, 1.0140f, 0.4877f}, {-0.9392f, 0.2606f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.3668f, 0.2606f, -0.4877f}, {-0.3668f, 1.0140f, -0.4877f}, {-0.3668f, 1.0140f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.3668f, 0.2606f, -0.4877f}, {-0.3668f, 1.0140f, 0.4877f}, {-0.3668f, 0.2606f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, -0.4877f}, {-0.9392f, 1.0140f, -0.4877f}, {-0.3668f, 1.0140f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, -0.4877f}, {-0.3668f, 1.0140f, -0.4877f}, {-0.3668f, 0.2606f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 1.0140f, 0.4877f}, {-0.9392f, 1.0140f, -0.4877f}, {-1.1968f, 0.6368f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 1.0140f, 0.4877f}, {-1.1968f, 0.6368f, -0.4877f}, {-1.1968f, 0.6368f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 1.0140f, -0.4877f}, {-0.9392f, 0.2606f, -0.4877f}, {-1.1968f, 0.2656f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 1.0140f, -0.4877f}, {-1.1968f, 0.2656f, -0.4877f}, {-1.1968f, 0.6368f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, 0.4877f}, {-0.9392f, 1.0140f, 0.4877f}, {-1.1968f, 0.6368f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, 0.4877f}, {-1.1968f, 0.6368f, 0.4877f}, {-1.1968f, 0.2656f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, -0.4877f}, {-0.9392f, 0.2606f, 0.4877f}, {-1.1968f, 0.2656f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, -0.4877f}, {-1.1968f, 0.2656f, 0.4877f}, {-1.1968f, 0.2656f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-1.1968f, 0.2656f, 0.4877f}, {-1.1968f, 0.6368f, 0.4877f}, {-1.1968f, 0.6368f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-1.1968f, 0.2656f, 0.4877f}, {-1.1968f, 0.6368f, -0.4877f}, {-1.1968f, 0.2656f, -0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.3668f, 1.0140f, -0.4877f}, {-0.9392f, 1.0140f, -0.4877f}, {-0.9392f, 1.0140f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.3668f, 1.0140f, -0.4877f}, {-0.9392f, 1.0140f, 0.4877f}, {-0.3668f, 1.0140f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, -0.4877f}, {-0.3668f, 0.2606f, -0.4877f}, {-0.3668f, 0.2606f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.9392f, 0.2606f, -0.4877f}, {-0.3668f, 0.2606f, 0.4877f}, {-0.9392f, 0.2606f, 0.4877f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.4423f, 0.2630f, 0.6616f}, {-0.4423f, 1.3803f, 0.6616f}, {-0.4423f, 1.3803f, -0.6616f}, bedColor);
    PushTri(verts, normals, uvs, colors, {-0.4423f, 0.2630f, 0.6616f}, {-0.4423f, 1.3803f, -0.6616f}, {-0.4423f, 0.2630f, -0.6616f}, bedColor);

    PushTri(verts, normals, uvs, colors, {-0.4423f, 0.2630f, -0.6616f}, {-0.4423f, 1.3803f, -0.6616f}, {1.7036f, 1.3803f, -0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {-0.4423f, 0.2630f, -0.6616f}, {1.7036f, 1.3803f, -0.6616f}, {1.7036f, 0.2630f, -0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {1.7036f, 0.2630f, -0.6616f}, {1.7036f, 1.3803f, -0.6616f}, {1.7036f, 1.3803f, 0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {1.7036f, 0.2630f, -0.6616f}, {1.7036f, 1.3803f, 0.6616f}, {1.7036f, 0.2630f, 0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {1.7036f, 0.2630f, 0.6616f}, {1.7036f, 1.3803f, 0.6616f}, {-0.4423f, 1.3803f, 0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {1.7036f, 0.2630f, 0.6616f}, {-0.4423f, 1.3803f, 0.6616f}, {-0.4423f, 0.2630f, 0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {-0.4423f, 0.2630f, -0.6616f}, {1.7036f, 0.2630f, -0.6616f}, {1.7036f, 0.2630f, 0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {-0.4423f, 0.2630f, -0.6616f}, {1.7036f, 0.2630f, 0.6616f}, {-0.4423f, 0.2630f, 0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {1.7036f, 1.3803f, -0.6616f}, {-0.4423f, 1.3803f, -0.6616f}, {-0.4423f, 1.3803f, 0.6616f}, cabColor);
    PushTri(verts, normals, uvs, colors, {1.7036f, 1.3803f, -0.6616f}, {-0.4423f, 1.3803f, 0.6616f}, {1.7036f, 1.3803f, 0.6616f}, cabColor);
}

Mesh BuildVehicleMesh(bool isTruck, Color bodyColor, Color cabColor) {
    std::vector<float> verts, normals, uvs;
    std::vector<unsigned char> colors;

    static const Color WHEEL_COLOR = {25, 25, 28, 255};

    float wheelRadius = 0.17f;
    float wheelY = wheelRadius;

    if (!isTruck) {
        // Sedan body is hand-modeled (see PushCarBody() above) -- wheel
        // positions/radius match exactly what the reshaped export used (see
        // humancar.glb's wheel vertex bounding box: X=+-0.79, Z=+-0.48,
        // radius ~0.17).
        PushCarBody(verts, normals, uvs, colors, bodyColor);
        float wheelX = 0.79f, wheelZ = 0.48f;
        PushWheel(verts, normals, uvs, colors, {-wheelX, wheelY,  wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
        PushWheel(verts, normals, uvs, colors, {-wheelX, wheelY, -wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
        PushWheel(verts, normals, uvs, colors, { wheelX, wheelY,  wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
        PushWheel(verts, normals, uvs, colors, { wheelX, wheelY, -wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
    } else {
        // Truck body is hand-modeled (see PushTruckBody() above) -- wheel
        // positions match humantruck.glb's wheel vertex bounding box
        // (X: -0.85 to 1.35, Z: +-0.54, radius ~0.17). Asymmetric front/rear
        // X (unlike the sedan) since the truck's bed and cab aren't the
        // same length ahead/behind the body's own origin.
        PushTruckBody(verts, normals, uvs, colors, bodyColor, cabColor);
        float rearX = 0.85f, frontX = 1.35f, wheelZ = 0.54f;
        PushWheel(verts, normals, uvs, colors, {-rearX,  wheelY,  wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
        PushWheel(verts, normals, uvs, colors, {-rearX,  wheelY, -wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
        PushWheel(verts, normals, uvs, colors, { frontX, wheelY,  wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
        PushWheel(verts, normals, uvs, colors, { frontX, wheelY, -wheelZ}, wheelRadius, 0.16f, WHEEL_COLOR);
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
