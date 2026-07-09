#ifndef PORTAL_BASE_MESH_HPP
#define PORTAL_BASE_MESH_HPP

#include "raylib.h"

// The office teleporter/merge-machine's stationary base (pedestal,
// staircase, and the two support pillars) -- split out from
// PortalRingMesh.hpp/.cpp so the ring can spin independently of this while
// the base stays put. See PortalRingMesh.hpp and PortalBaseMesh.cpp for the
// Blender round-trip pipeline this was built through.
// Caller owns the returned Mesh and must UnloadMesh()/fold it into a Model
// (LoadModelFromMesh) as usual.
Mesh BuildPortalBaseMesh();

#endif // PORTAL_BASE_MESH_HPP
