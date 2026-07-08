#ifndef SCENE_EFFECT_HPP
#define SCENE_EFFECT_HPP

class SceneEffect {
public:
    virtual ~SceneEffect() = default;
    virtual void init() {}
    virtual void update(float deltaTime) {}
    virtual void drawBackground() {}
    virtual void drawForeground() {}
    virtual void cleanup() {}

    // Optional debug-camera controls for a 3D effect's own internal camera
    // (distance/pitch/fovy) -- lets any scene expose the same live-tuning
    // numpad controls (see SceneDebugCamera.hpp) against whichever effect it
    // holds, without the caller needing to know the effect's concrete type.
    // Effects that don't have a tunable 3D camera just leave the default
    // hasDebugCamera() == false and the rest are never called.
    virtual bool hasDebugCamera() const { return false; }
    virtual float getDebugCamDist() const { return 0.0f; }
    virtual void setDebugCamDist(float dist) {}
    virtual float getDebugPitch() const { return 0.0f; }
    virtual void setDebugPitch(float deg) {}
    virtual float getDebugFovy() const { return 0.0f; }
    virtual void setDebugFovy(float deg) {}
};

#endif
