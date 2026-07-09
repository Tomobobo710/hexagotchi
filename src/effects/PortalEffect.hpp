#ifndef PORTAL_EFFECT_HPP
#define PORTAL_EFFECT_HPP

#include "SceneEffect.hpp"
#include "raylib.h"
#include "rlights.h"

// The membrane is still procedural (a plain quad, no reason to hand-edit it
// in Blender); exposed so tools/export_glb.cpp can dump it for reference.
// The ring frame lives in PortalRingMesh.hpp/.cpp (BuildPortalRingMesh());
// the stationary pedestal/staircase/pillars live separately in
// PortalBaseMesh.hpp/.cpp (BuildPortalBaseMesh()) so the ring can spin
// independently of the base -- see those files' header comments for the
// Blender export/reshape/bake-back pipeline.
Mesh BuildMembraneMesh(float innerRadius);
extern const float RING_RADIUS;
extern const float RING_TUBE_THICKNESS;

// The office's teleporter/"merge machine" -- lore-wise, the device that
// moves Tom between his world and the physical Tomagotchi. A lit ring frame
// (spins continuously) on a stationary pedestal base (same hand-built-mesh/
// LitShader convention as JetMesh/SchoolSkyEffect), with an unlit animated
// membrane inside the ring (PortalShader) standing in for the portal
// surface itself.
//
// Draws in FRONT of the scene's 2D background but BEHIND its actors -- see
// OfficeScene::draw(), which splits its 2D passes around this effect's
// drawBackground() call instead of using the normal background/foreground
// SceneEffect split (background effects render behind the WHOLE 2D layer,
// which would put this behind Tom too).
class PortalEffect : public SceneEffect {
public:
    void init() override;
    void update(float deltaTime) override;
    void drawBackground() override;
    void cleanup() override;

    // SceneEffect's debug-camera interface (see SceneDebugCamera.hpp) --
    // same live-tuning pattern as CityWindowEffect.
    bool hasDebugCamera() const override { return true; }
    float getDebugCamDist() const override { return debugCamDist; }
    void setDebugCamDist(float dist) override { debugCamDist = dist; }
    float getDebugPitch() const override { return debugPitchDeg; }
    void setDebugPitch(float deg) override { debugPitchDeg = deg; }
    float getDebugFovy() const override { return debugFovyDeg; }
    void setDebugFovy(float deg) override { debugFovyDeg = deg; }

    // 0 = idle (gentle ambient shimmer), 1 = fully charged (bright, fast) --
    // OfficeScene can ramp this up around an actual teleport beat later.
    void setIntensity(float i) { intensity = i; }

    // Uniform scale and yaw (rotation around Y, degrees) for the whole
    // assembled device (ring + pedestal + membrane) -- lets the device be
    // resized/turned to fit a scene without touching mesh-build constants.
    // Live-tunable via the same debug-camera controls (SceneDebugCamera.hpp
    // extends into these too): Numpad +/- for scale, Numpad 4/6 for yaw.
    float getObjectScale() const { return objectScale; }
    void setObjectScale(float s) { objectScale = s; }
    float getObjectYaw() const { return objectYawDeg; }
    void setObjectYaw(float deg) { objectYawDeg = deg; }

    // World-space position of the whole assembled device (ring + pedestal +
    // membrane), independent of the debug camera -- lets a scene place the
    // portal off to one side of its 3D space (e.g. OfficeScene tucking it in
    // the back-left corner) instead of it always sitting dead-center in
    // front of the camera. Defaults to the origin, i.e. the old behavior.
    Vector3 getObjectPosition() const { return objectPosition; }
    void setObjectPosition(Vector3 pos) { objectPosition = pos; }

private:
    Shader litShader;
    Light light;
    Model ringModel;      // the spinning ring frame only
    Model baseModel;      // stationary pedestal/staircase/pillars
    Texture2D whiteTex = {0};

    Shader portalShader;
    Mesh membraneMesh = {};
    Model membraneModel;
    int portalTimeLoc = -1;
    int portalIntensityLoc = -1;

    float time = 0.0f;
    float intensity = 0.6f;

    float debugCamDist = 6.0f;
    float debugPitchDeg = 0.0f;
    float debugFovyDeg = 45.0f;

    float objectScale = 1.0f;
    float objectYawDeg = 0.0f;
    Vector3 objectPosition = {0.0f, 0.0f, 0.0f};

    // Continuous flywheel-style spin around the ring's own facing axis (Z),
    // driven by `time` -- separate from the manual debug yaw above (which
    // turns the whole device to face a different direction, not spin it in
    // its own plane).
    float ringSpinDegPerSec = 12.0f;
};

#endif // PORTAL_EFFECT_HPP
