#ifndef TOMAGOTCHITOY_MESH_HPP
#define TOMAGOTCHITOY_MESH_HPP

#include "raylib.h"

// Hand-built mesh baked from a Blender round-trip (see blender-mesh-pipeline
// memory / export_glb.cpp + tools/dump_glb.py) -- one PushTri() call per
// triangle, generated mechanically from the exported .glb's already-resolved
// per-triangle colors (vertex-paint COLOR_0 or per-material baseColorFactor,
// whichever the source file used), not hand-classified into named parts.
Mesh BuildTomagotchiToyMesh();

#endif // TOMAGOTCHITOY_MESH_HPP
