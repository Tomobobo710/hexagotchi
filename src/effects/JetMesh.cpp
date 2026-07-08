#include "JetMesh.hpp"
#include "MeshBuilders.hpp"
#include "raymath.h"
#include <cstring>
#include <cmath>
#include <vector>

// Hand-built low-poly airliner silhouette: a tapered fuselage, swept wings
// with real airfoil thickness, underwing engine pods, a tailplane, and a
// proper double-sided vertical stabilizer. Deliberately low-poly/toy-like --
// a background silhouette seen from a distance, not a hero asset -- but with
// enough detail to read as a real jet up close when inspected directly.
// All coordinates in local model space, nose pointing +X, up is +Y.
Mesh BuildJetMesh() {
    std::vector<float> verts, normals, uvs;
    std::vector<unsigned char> colors;

    static const int FUSELAGE_SIDES = 10;
    static const Color BODY_WHITE = {225, 228, 232, 255};
    static const Color ACCENT_RED = {190, 30, 35, 255};
    static const Color ENGINE_GREY = {55, 58, 62, 255};

    // Main fuselage tube: more rings than before for a smoother taper.
    // Ring 0-1 (the nose cap/cockpit section) is painted red; a full-length
    // stripe runs down both the +Z and -Z sides, symmetric left/right;
    // everything else stays body white.
    std::vector<TubeRing> fuselage = {
        {  1.00f, 0.000f, 0.000f },  // nose tip
        {  0.86f, 0.075f, 0.065f },
        {  0.68f, 0.14f,  0.12f  },
        {  0.50f, 0.165f, 0.145f },
        {  0.10f, 0.17f,  0.15f  },
        { -0.30f, 0.17f,  0.15f  },  // constant body through the wing box
        { -0.55f, 0.15f,  0.13f  },
        { -0.75f, 0.11f,  0.095f },
        { -0.92f, 0.05f,  0.045f },
        { -1.00f, 0.000f, 0.000f },  // tail tip
    };
    PushTube(verts, normals, uvs, colors, fuselage, FUSELAGE_SIDES, {0, 0, 0},
        [](int ringIdx, int side) -> Color {
            if (ringIdx <= 1) return ACCENT_RED;      // nose/cockpit
            if (side == 0 || side == FUSELAGE_SIDES / 2) return ACCENT_RED;
            return BODY_WHITE;
        });

    // Wings: swept-back, tapering to a point at the tip, mounted mid-fuselage,
    // now with real airfoil-style thickness via PushTaperedPanel (closed
    // solid, not a one-sided sheet).
    Vector3 wingRootR = {0.02f, -0.02f, 0.15f};
    Vector3 wingTipR  = {-0.70f, -0.04f, 1.15f};
    PushTaperedPanel(verts, normals, uvs, colors, wingRootR, 0.10f, 0.32f, wingTipR, 0.025f, false, BODY_WHITE);

    Vector3 wingRootL = {0.02f, -0.02f, -0.15f};
    Vector3 wingTipL  = {-0.70f, -0.04f, -1.15f};
    PushTaperedPanel(verts, normals, uvs, colors, wingRootL, 0.10f, 0.32f, wingTipL, 0.025f, true, BODY_WHITE);

    // Underwing engine pods: two per side, small tapered tubes slung below
    // the wing's actual surface at that span position (interpolated along
    // the wing's own root-to-tip line, so they hang attached under the wing
    // instead of floating at a fixed offset that ignores the sweep/dihedral).
    // Four engines total, the other unmistakably-747 detail (not two).
    auto wingPointAt = [](Vector3 root, Vector3 tip, float t) -> Vector3 {
        return Vector3Lerp(root, tip, t);
    };
    auto addEngine = [&](Vector3 root, Vector3 tip, float t) {
        Vector3 mount = wingPointAt(root, tip, t);
        std::vector<TubeRing> engine = {
            { 0.16f, 0.000f, 0.000f },
            { 0.12f, 0.045f, 0.045f },
            { -0.05f, 0.05f, 0.05f },
            { -0.16f, 0.03f, 0.03f },
            { -0.20f, 0.000f, 0.000f },
        };
        // Hang below and slightly ahead of the wing's local surface point.
        PushTube(verts, normals, uvs, colors, engine, 8, {mount.x + 0.05f, mount.y, mount.z},
            [](int, int) { return ENGINE_GREY; });
    };
    addEngine(wingRootR, wingTipR, 0.30f);
    addEngine(wingRootR, wingTipR, 0.68f);
    addEngine(wingRootL, wingTipL, 0.30f);
    addEngine(wingRootL, wingTipL, 0.68f);

    // Tailplane: same tapered-panel technique, much smaller, near the tail.
    Vector3 tailRootR = {-0.80f, 0.05f, 0.05f};
    Vector3 tailTipR  = {-0.90f, 0.05f, 0.22f};
    PushTaperedPanel(verts, normals, uvs, colors, tailRootR, 0.05f, 0.10f, tailTipR, 0.012f, false, BODY_WHITE);

    Vector3 tailRootL = {-0.80f, 0.05f, -0.05f};
    Vector3 tailTipL  = {-0.90f, 0.05f, -0.22f};
    PushTaperedPanel(verts, normals, uvs, colors, tailRootL, 0.05f, 0.10f, tailTipL, 0.012f, true, BODY_WHITE);

    // Vertical stabilizer: an upright tapered panel. Painted red per feedback.
    Vector3 finRoot = {-0.80f, -0.001f, 0.0f};
    Vector3 finTip  = {-0.88f, 0.40f, 0.0f};
    PushUprightTaperedPanel(verts, normals, uvs, colors, finRoot, 0.10f, 0.18f, finTip, 0.014f, true, ACCENT_RED);

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
