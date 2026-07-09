#ifndef PORTAL_RING_MESH_HPP
#define PORTAL_RING_MESH_HPP

#include "raylib.h"

// The office teleporter/merge-machine's ring frame, pedestal, staircase, and
// support pillars, as one merged mesh -- originally hand-built procedurally,
// then reshaped by Tom in Blender (round-tripped via tools/export_glb.cpp:
// export the working mesh to .glb, reshape in Blender, export back, then the
// vertex data gets read back in and re-expressed here as PushTri() calls,
// one per triangle, same as every other hand-built mesh in this codebase
// (see JetMesh.hpp/.cpp) rather than an opaque baked vertex blob.
// Caller owns the returned Mesh and must UnloadMesh()/fold it into a Model
// (LoadModelFromMesh) as usual.
Mesh BuildPortalRingMesh();

#endif // PORTAL_RING_MESH_HPP
