#ifndef VEHICLE_MESH_HPP
#define VEHICLE_MESH_HPP

#include "raylib.h"

// Hand-built low-poly car/truck silhouette for TherapistWindowEffect's hill
// road -- replaces the original stretched GenMeshCube placeholder, which
// read as a bare box with no wheels or cab. Unit-scale model, nose along
// +X, up is +Y, wheels resting on Y=0 (the effect scales/positions/yaws
// this per-vehicle the same way it did the old cube).
//
// isTruck picks between a car silhouette (lower body + a raised
// cab/greenhouse toward the front) and a truck silhouette (a flat cargo
// bed running the back two-thirds, cab only at the front) -- same
// PushQuad/PushTri techniques as JetMesh.cpp, not GenMesh* primitives glued
// together, so the cab step and wheel wells actually read as a vehicle.
//
// bodyColor is baked directly into the body/cab vertices (not left WHITE
// for a caller to tint via DrawModelEx) because glass and wheels need their
// own fixed colors that a tint multiply would otherwise wash out along with
// the body -- so each vehicle instance builds/owns its own small mesh
// rather than sharing one model across different-colored vehicles.
//
// For a truck, bodyColor is the flatbed/rear-box color and cabColor is the
// front cab color (two distinct sections of the reshaped truck body -- see
// PushTruckBody() in the .cpp); ignored for a car, which uses bodyColor for
// its whole (single-piece) body.
Mesh BuildVehicleMesh(bool isTruck, Color bodyColor, Color cabColor = WHITE);

#endif // VEHICLE_MESH_HPP
