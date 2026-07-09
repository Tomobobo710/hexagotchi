#ifndef THERAPIST_WINDOW_EFFECT_HPP
#define THERAPIST_WINDOW_EFFECT_HPP

#include "SceneEffect.hpp"
#include "raylib.h"
#include "rlights.h"
#include <vector>

// One-off 3D background effect for TherapistOfficeScene: therapistbg.png has
// two windows baked into the art (left and right) with transparent cutouts,
// same convention as ApartmentScene/apartmentbg.png -- this draws full-canvas
// behind the 2D background art (registered via addEffect(), same as
// CityWindowEffect), and the opaque art masks everything except what shows
// through the two window cutouts.
//
// One fixed 3D camera looking straight down -Z covers both windows at once:
// trees sit off to -X (behind the left window), the hill road sits off to
// +X (behind the right window) -- same shared-space trick CityWindowEffect
// uses for its multiple building rows.
//
// Left window: a small stand of trees, static but swaying gently.
// Right window: a road climbing a hill in the distance, with cars/trucks
// periodically driving up it and over the crest.
//
// Same "one-off per scene" convention as SchoolSkyEffect/CityWindowEffect --
// not meant to be reused elsewhere, just lives in its own file/class.
class TherapistWindowEffect : public SceneEffect {
public:
    void init() override;
    void update(float deltaTime) override;
    void drawBackground() override;
    void cleanup() override;

    // SceneEffect's debug-camera interface (see SceneDebugCamera.hpp) --
    // same live-tuning pattern as CityWindowEffect/PortalEffect.
    bool hasDebugCamera() const override { return true; }
    float getDebugCamDist() const override { return debugCamDist; }
    void setDebugCamDist(float dist) override { debugCamDist = dist; }
    float getDebugPitch() const override { return debugPitchDeg; }
    void setDebugPitch(float deg) override { debugPitchDeg = deg; }
    float getDebugFovy() const override { return debugFovyDeg; }
    void setDebugFovy(float deg) override { debugFovyDeg = deg; }

    // Live-tunable root offset for the whole driving-scene assembly (road,
    // vehicles, backdrop quad) -- see roadOrigin below. Exposed so a scene
    // can wire up its own dial-in controls (e.g. TherapistOfficeScene's
    // I/K, J/L, U/O for Y/X/Z) while iterating on placement.
    Vector3 getRoadOrigin() const { return roadOrigin; }
    void setRoadOrigin(Vector3 origin) { roadOrigin = origin; }

    // Same idea as roadOrigin, but for the tree stand -- one offset applied
    // to every tree's basePos so the whole stand can be dialed into the
    // left window as a single unit.
    Vector3 getTreeOrigin() const { return treeOrigin; }
    void setTreeOrigin(Vector3 origin) { treeOrigin = origin; }

    // Center point of the big green backdrop quad behind both windows.
    // Independent of roadOrigin/treeOrigin -- the backdrop needs to stay
    // visible behind BOTH windows at once regardless of where the road/
    // trees themselves sit, so it gets its own dial-in origin rather than
    // following either of them.
    Vector3 getBackdropOrigin() const { return backdropOrigin; }
    void setBackdropOrigin(Vector3 origin) { backdropOrigin = origin; }

private:
    struct Tree {
        Vector3 basePos;
        float trunkHeight;
        float foliageRadius;
        float swayPhase;
        float swaySpeed;
    };
    std::vector<Tree> trees;

    struct Vehicle {
        float t;            // 0..1 progress up the hill road
        float speed;        // t/sec, always positive -- direction of travel
                             // comes from oncoming, not the sign of speed
        Color bodyColor;
        bool isTruck;       // trucks are bigger/slower
        // Oncoming traffic drives in the left lane, down the hill (t counts
        // 1->0 instead of 0->1) facing the opposite way -- see update()/
        // drawVehicle().
        bool oncoming;
        // Each vehicle bakes its own body color directly into its mesh (see
        // VehicleMesh.hpp) rather than sharing one model tinted via
        // DrawModelEx, since a tint multiply would also wash out the fixed
        // glass/wheel colors baked into the same mesh -- owned per-instance,
        // unloaded when the vehicle despawns (see update()).
        Model model;
    };
    std::vector<Vehicle> vehicles;
    float spawnTimer = 0.0f;
    float spawnInterval = 4.0f;

    Shader shader;
    Light light;
    Model trunkModel;      // unit cylinder, scaled per-tree
    Model foliageModel;    // unit sphere, scaled per-tree
    Texture2D whiteTex = {0};

    void buildTrees();
    void drawTree(const Tree& tree) const;
    // Road path: world-space position at t=0..1 up the hill (0 = base/near
    // camera, 1 = over the crest, off into the distance). Includes
    // roadOrigin, so moving that one field repositions the road, every
    // vehicle on it, AND the backdrop quad together as a single unit.
    Vector3 roadPointAt(float t) const;
    void drawRoad() const;
    void drawVehicle(const Vehicle& v) const;
    void drawBackdrop() const;

    // Single offset the whole driving-scene assembly (road, vehicles,
    // backdrop quad) is built relative to -- move this one value to
    // reposition everything together into the window frame, rather than
    // needing to separately re-tune the road path, backdrop, and vehicle
    // placement math.
    Vector3 roadOrigin = {8.55f, -2.07f, -11.89f};
    // Same purpose as roadOrigin, but for the tree stand -- see
    // getTreeOrigin()/setTreeOrigin().
    Vector3 treeOrigin = {3.61f, 0.20f, 11.76f};
    // Center of the backdrop quad -- see getBackdropOrigin()/
    // setBackdropOrigin(). Starts centered/at-origin; dial in via
    // TherapistOfficeScene's controls same as road/tree origins were.
    Vector3 backdropOrigin = {0.00f, -7.08f, -29.15f};

    float time = 0.0f;

    // Camera framing, tuned live via the shared debug-camera controls
    // (SceneDebugCamera.hpp): Numpad 8/2 dist, 9/3 pitch, 7/1 fovy.
    float debugCamDist = 25.0f;
    float debugPitchDeg = 0.0f;
    float debugFovyDeg = 32.0f;
};

#endif // THERAPIST_WINDOW_EFFECT_HPP
