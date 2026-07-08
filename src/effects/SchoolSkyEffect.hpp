#ifndef SCHOOL_SKY_EFFECT_HPP
#define SCHOOL_SKY_EFFECT_HPP

#include "SceneEffect.hpp"
#include "raylib.h"
#include "rlights.h"   // declarations only; RLIGHTS_IMPLEMENTATION already lives in MoonEffect.cpp
#include <vector>

// One-off 3D background effect for SchoolScene: a small jet drifts left to
// right behind two lit 3D clouds, looping. Deliberately NOT a shared/reusable
// class with MoonEffect -- see the "one-off per scene" discussion this was
// born from. Only the shader-compile-and-light-rig plumbing is copied from
// MoonEffect's pattern (cheap to duplicate); the placement/content is unique
// to this scene, so it lives in its own file instead of growing MoonEffect
// into a shared base class it was never designed to be.
class SchoolSkyEffect : public SceneEffect {
public:
    void init() override;
    void update(float deltaTime) override;
    void drawBackground() override;
    void cleanup() override;

private:
    Shader shader;
    Light light;

    // The full hand-built jet model shared with Model3DTestScene (see
    // JetMesh.hpp) -- previously this effect built its own crude 4-primitive
    // stand-in (cylinder fuselage, cone nose, box wings/tail); now it flies
    // the same detailed model that gets designed/inspected there.
    Model jetModel;
    Texture2D whiteTex = {0};

    // Clouds: each is a small cluster of overlapping flattened spheres so
    // they read as a cloud rather than a single ball.
    Model cloudPuff;

    struct CloudInstance {
        Vector3 basePos;
        std::vector<Vector3> puffOffsets;
        std::vector<float> puffScales;
        float driftSpeed;  // world units/sec, always leftward (-X)
    };
    std::vector<CloudInstance> clouds;

    // Respawns one cloud at a randomized position off the right edge (with
    // some depth/altitude variance) and gives it a fresh randomized puff
    // layout -- used both for initial placement and recycling one that's
    // drifted off the left edge.
    void RespawnCloud(CloudInstance& cloud, bool offscreenRight);

    float jetT = 0.0f;          // 0..1 progress across the flight path this loop
    float jetLoopDuration = 45.0f;  // slow, distant crawl rather than a fast buzz
    // Pushed way past both clouds' Z (-1.2 / -1.4) so it genuinely reads as a
    // far-off airliner and passes behind them, not just a scaled-down model.
    // Y raised well above the clouds' own altitude (1.1-1.7) per feedback --
    // reads as cruising altitude above the cloud layer, not through it.
    Vector3 jetStart = {-40.0f, 10.0f, -80.0f};
    Vector3 jetEnd   = { 40.0f, 10.5f, -80.0f};

    // Gentle rocking (roll oscillation) for subtle life, not a real flight
    // dynamic -- a slow sine wave applied as bank angle in drawBackground().
    float rockTimer = 0.0f;

    // Contrail: a short history of recent jet positions, drawn as a fading
    // line trail behind it.
    std::vector<Vector3> contrailPoints;
    float contrailTimer = 0.0f;

    void drawCloud(const CloudInstance& cloud) const;
};

#endif // SCHOOL_SKY_EFFECT_HPP
