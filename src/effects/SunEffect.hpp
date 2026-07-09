#ifndef SUN_EFFECT_HPP
#define SUN_EFFECT_HPP

#include "SceneEffect.hpp"
#include "raylib.h"
#include "rlights.h"   // declarations only; RLIGHTS_IMPLEMENTATION lives in MoonEffect.cpp

// One-off 3D background effect for PizzaParlorScene: a glowing sunset sun,
// glimpsed far off through the transparent glass cutout in the parlor's
// front door (parlorbg.png, door glass pane roughly x:107-253 y:100-362),
// with a low-poly pine tree silhouette standing in front of it. Same
// "one-off per scene" convention as MoonEffect/SchoolSkyEffect/
// CityWindowEffect/TherapistWindowEffect -- registered via addEffect() so it
// draws behind the 2D background art, showing through only where the art's
// alpha is cut out.
//
// Same low-poly lit-sphere/shader rig as MoonEffect (LitShader, one
// directional light, continuous spin), just tinted warm orange/yellow and
// lit to match TherapistWindowEffect's light angle so the window effects
// read consistently across scenes. The pine tree shares that same shader/
// light rig.
class SunEffect : public SceneEffect {
public:
    void init() override;
    void update(float deltaTime) override;
    void drawBackground() override;
    void cleanup() override;

    // SceneEffect's debug-camera interface (see SceneDebugCamera.hpp) --
    // same live-tuning pattern as CityWindowEffect/TherapistWindowEffect.
    bool hasDebugCamera() const override { return true; }
    float getDebugCamDist() const override { return debugCamDist; }
    void setDebugCamDist(float dist) override { debugCamDist = dist; }
    float getDebugPitch() const override { return debugPitchDeg; }
    void setDebugPitch(float deg) override { debugPitchDeg = deg; }
    float getDebugFovy() const override { return debugFovyDeg; }
    void setDebugFovy(float deg) override { debugFovyDeg = deg; }

    // Live-tunable world position of the sun itself -- exposed so a scene can
    // wire up its own dial-in controls (I/K, J/L, U/O for Y/X/Z, same pattern
    // TherapistWindowEffect's roadOrigin/treeOrigin/backdropOrigin used)
    // while iterating on where it should sit behind the door glass.
    Vector3 getSunOrigin() const { return sunOrigin; }
    void setSunOrigin(Vector3 origin) { sunOrigin = origin; }

    // Same dial-in pattern, for a low-poly pine tree silhouette drawn in
    // front of the sun (closer to the camera than sunOrigin).
    Vector3 getTreeOrigin() const { return treeOrigin; }
    void setTreeOrigin(Vector3 origin) { treeOrigin = origin; }

private:
    Shader shader;
    Model sunModel;
    Light light;
    Texture2D whiteTex = {0};
    float rotation = 0.0f;

    // Pine tree: a trunk cylinder (GenMeshCylinder) plus several stacked,
    // shrinking cone tiers (GenMeshCone) -- drawn lit, same shader/light as
    // the sun. Both are unit meshes, scaled per-tier at draw time.
    Model trunkModel;
    Model tierModel;

    Vector3 sunOrigin = {-26.08f, 15.70f, -22.85f};
    Vector3 treeOrigin = {-14.36f, 3.02f, -7.87f};

    float debugCamDist = 25.0f;
    float debugPitchDeg = 0.0f;
    float debugFovyDeg = 32.0f;
};

#endif // SUN_EFFECT_HPP
