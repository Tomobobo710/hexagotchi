#ifndef JET_MESH_HPP
#define JET_MESH_HPP

#include "raylib.h"

// Hand-built low-poly airliner mesh (tapered fuselage, swept wings with real
// airfoil thickness, underwing engine pods, tailplane, vertical stabilizer,
// red nose/stripe/tail paint), shared between Model3DTestScene (the orbiting
// inspector used to design it) and SchoolSkyEffect (the actual background
// jet that flies past in SchoolScene) -- unlike MoonEffect's one-off-per-
// scene placement code, the mesh geometry itself is identical in both places,
// so it's a real shared asset rather than duplicated boilerplate.
// Caller owns the returned Mesh and must UnloadMesh()/fold it into a Model
// (LoadModelFromMesh) as usual.
Mesh BuildJetMesh();

#endif // JET_MESH_HPP
