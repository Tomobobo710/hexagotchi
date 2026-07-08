#ifndef MESH_BUILDERS_HPP
#define MESH_BUILDERS_HPP

#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <functional>
#include <cmath>

// Shared low-level helpers for hand-building triangle meshes (as opposed to
// GenMeshCube/GenMeshCylinder/etc. primitives glued together, which reads as
// crude for organic/tapered shapes -- see JetMesh.cpp for the first real use).
// Header-only since these are small and only ever called from one mesh-
// building .cpp per model; move a given model's specific mesh (e.g.
// BuildJetMesh) into its own file, not this one -- this file is only the
// reusable primitives.

// Appends one triangle (3 verts, CCW winding when viewed from the side the
// normal points to) to the flat vertex/normal arrays being built up. Color is
// per-vertex (multiplied with the material tint in the shader, see fragColor
// in LitShader) so different parts of the one merged mesh/model can be
// tinted differently without needing separate materials/draw calls.
inline void PushTri(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                     std::vector<unsigned char>& colors,
                     Vector3 a, Vector3 b, Vector3 c, Color color = WHITE) {
    Vector3 n = Vector3Normalize(Vector3CrossProduct(Vector3Subtract(b, a), Vector3Subtract(c, a)));
    Vector3 pts[3] = {a, b, c};
    for (int i = 0; i < 3; i++) {
        verts.push_back(pts[i].x); verts.push_back(pts[i].y); verts.push_back(pts[i].z);
        normals.push_back(n.x); normals.push_back(n.y); normals.push_back(n.z);
        uvs.push_back(0.0f); uvs.push_back(0.0f);
        colors.push_back(color.r); colors.push_back(color.g); colors.push_back(color.b); colors.push_back(color.a);
    }
}

// Appends two triangles forming a quad a-b-c-d (in order around the perimeter).
inline void PushQuad(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                      std::vector<unsigned char>& colors,
                      Vector3 a, Vector3 b, Vector3 c, Vector3 d, Color color = WHITE) {
    PushTri(verts, normals, uvs, colors, a, b, c, color);
    PushTri(verts, normals, uvs, colors, a, c, d, color);
}

// Builds one solid, fully-closed tapered panel (a wing/tailplane/fin blade):
// a flat root cross-section (leading edge + trailing edge, given thickness)
// sweeping out to a single point at the tip. Closed with top and bottom faces
// so it reads as a real solid surface from every angle, not a one-sided
// paper triangle. `rootPos`/`tipPos` are the panel's centerline points
// (leading/trailing edges are offset from rootPos along X by the given
// lengths); `thickness` is the half-thickness added/subtracted along +Y.
//
// `mirrored`: a panel built with `tipPos` on the -Z side (e.g. the left half
// of a symmetric pair) is a mirror reflection of the +Z (right) case, which
// flips triangle handedness -- the winding that faces outward on the right
// ends up facing inward on the left. Pass mirrored=true for the -Z copy to
// swap each pair's vertex order and compensate, so both sides shade
// correctly instead of one being lit backwards. This bit us twice in
// practice (a wing, then a vertical fin) -- if a mirrored/symmetric part
// looks lit-backwards, check this flag before suspecting the light angle.
inline void PushTaperedPanel(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                              std::vector<unsigned char>& colors,
                              Vector3 rootPos, float rootLeadLen, float rootTrailLen,
                              Vector3 tipPos, float thickness, bool mirrored = false, Color color = WHITE) {
    Vector3 rootLead  = {rootPos.x + rootLeadLen, rootPos.y, rootPos.z};
    Vector3 rootTrail = {rootPos.x - rootTrailLen, rootPos.y, rootPos.z};
    Vector3 rootTop    = {rootPos.x, rootPos.y + thickness, rootPos.z};
    Vector3 rootBot    = {rootPos.x, rootPos.y - thickness, rootPos.z};
    Vector3 tip = tipPos;

    if (!mirrored) {
        // Top surface (faces +Y): two triangles fanning from the tip.
        PushTri(verts, normals, uvs, colors, rootLead, rootTop, tip, color);
        PushTri(verts, normals, uvs, colors, rootTop, rootTrail, tip, color);
        // Bottom surface (faces -Y): same fan, winding reversed.
        PushTri(verts, normals, uvs, colors, rootBot, rootLead, tip, color);
        PushTri(verts, normals, uvs, colors, rootBot, tip, rootTrail, color);
    } else {
        PushTri(verts, normals, uvs, colors, rootTop, rootLead, tip, color);
        PushTri(verts, normals, uvs, colors, rootTrail, rootTop, tip, color);
        PushTri(verts, normals, uvs, colors, rootLead, rootBot, tip, color);
        PushTri(verts, normals, uvs, colors, tip, rootBot, rootTrail, color);
    }
}

// Same shape as PushTaperedPanel but upright: thickness spreads along Z
// (left/right faces) instead of Y, and the tip rises along Y instead of
// spreading along Z. Use for any vertical fin/stabilizer-style panel.
inline void PushUprightTaperedPanel(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                                     std::vector<unsigned char>& colors,
                                     Vector3 rootPos, float rootLeadLen, float rootTrailLen,
                                     Vector3 tipPos, float thickness, bool mirrored = false, Color color = WHITE) {
    Vector3 rootLead  = {rootPos.x + rootLeadLen, rootPos.y, rootPos.z};
    Vector3 rootTrail = {rootPos.x - rootTrailLen, rootPos.y, rootPos.z};
    Vector3 rootRight = {rootPos.x, rootPos.y, rootPos.z + thickness};
    Vector3 rootLeftS = {rootPos.x, rootPos.y, rootPos.z - thickness};
    Vector3 tip = tipPos;

    if (!mirrored) {
        // Right face (+Z): fan from the tip, matching PushTaperedPanel's
        // proven top-surface winding with Y and Z swapped.
        PushTri(verts, normals, uvs, colors, rootLead, rootRight, tip, color);
        PushTri(verts, normals, uvs, colors, rootRight, rootTrail, tip, color);
        // Left face (-Z): same fan, winding reversed.
        PushTri(verts, normals, uvs, colors, rootLeftS, rootLead, tip, color);
        PushTri(verts, normals, uvs, colors, rootLeftS, tip, rootTrail, color);
    } else {
        PushTri(verts, normals, uvs, colors, rootRight, rootLead, tip, color);
        PushTri(verts, normals, uvs, colors, rootTrail, rootRight, tip, color);
        PushTri(verts, normals, uvs, colors, rootLead, rootLeftS, tip, color);
        PushTri(verts, normals, uvs, colors, tip, rootLeftS, rootTrail, color);
    }
}

// Builds one ring-based tapered tube (fuselage segment, engine pod, or any
// other lofted round shape): a list of {x, halfW, halfH} rings stitched into
// quads, tips (halfW==0) closed as a single-triangle fan. `colorAt(ringIdx,
// side)` picks the color for the panel between ring ringIdx/ringIdx+1 at
// side index `side` (0..sides-1) -- lets a single tube carry a paint scheme
// (e.g. one red side-stripe) instead of every tube being one flat color.
struct TubeRing { float x, halfW, halfH; };

inline void PushTube(std::vector<float>& verts, std::vector<float>& normals, std::vector<float>& uvs,
                     std::vector<unsigned char>& colors,
                     const std::vector<TubeRing>& rings, int sides, Vector3 offset,
                     std::function<Color(int, int)> colorAt = nullptr) {
    std::vector<float> angles(sides);
    for (int i = 0; i < sides; i++) angles[i] = (float)i / sides * 2.0f * PI;

    auto ringPoint = [&](const TubeRing& r, float angle) -> Vector3 {
        return {r.x + offset.x, offset.y + sinf(angle) * r.halfH, offset.z + cosf(angle) * r.halfW};
    };

    for (size_t ringIdx = 0; ringIdx + 1 < rings.size(); ringIdx++) {
        const TubeRing& r0 = rings[ringIdx];
        const TubeRing& r1 = rings[ringIdx + 1];
        for (int i = 0; i < sides; i++) {
            int j = (i + 1) % sides;
            Vector3 a = ringPoint(r0, angles[i]);
            Vector3 b = ringPoint(r0, angles[j]);
            Vector3 c = ringPoint(r1, angles[j]);
            Vector3 d = ringPoint(r1, angles[i]);
            Color color = colorAt ? colorAt((int)ringIdx, i) : WHITE;
            if (r0.halfW == 0.0f) {
                PushTri(verts, normals, uvs, colors, a, c, d, color);
            } else if (r1.halfW == 0.0f) {
                PushTri(verts, normals, uvs, colors, a, b, c, color);
            } else {
                PushQuad(verts, normals, uvs, colors, a, b, c, d, color);
            }
        }
    }
}

#endif // MESH_BUILDERS_HPP
