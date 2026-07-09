#ifndef MODEL_3D_TEST_SCENE_HPP
#define MODEL_3D_TEST_SCENE_HPP

#include "Scene.hpp"
#include "raylib.h"
#include "rlights.h"   // declarations only; RLIGHTS_IMPLEMENTATION already lives in MoonEffect.cpp

// Sandbox scene for iterating on hand-built 3D models (e.g. the school-sky
// jet) without needing to fly the actual scene's effect to see changes.
// Free-look: drag to orbit, wheel to zoom, so you can inspect the model from
// any angle while tuning geometry. Registered on the scene-select hub as
// "3D MODEL TEST", not tied to any narrative scene.
class Model3DTestScene : public Scene {
public:
    Model3DTestScene();

    void init() override;
    void update(float deltaTime) override;
    void draw() override;
    void cleanup() override;

private:
    // TAB cycles through every hand-built model this scene can inspect.
    // COMBINED shows the whole assembled portal (ring + base + membrane)
    // exactly as PortalEffect draws it -- spinning ring, shimmering
    // membrane shader and all -- so a full combined check doesn't require
    // flying the actual OfficeScene.
    enum class ModelKind { JET, PORTAL_RING, PORTAL_BASE, PORTAL_COMBINED, CAR, TRUCK };
    ModelKind modelKind = ModelKind::JET;

    Shader shader;
    Light light;
    Model jetModel;
    Model portalRingModel;
    Model portalBaseModel;
    Model carModel;
    Model truckModel;
    Texture2D whiteTex = {0};

    // Membrane needs its own shader (unlit, animated) -- see PortalEffect,
    // this mirrors that setup for the COMBINED preview.
    Shader portalShader;
    Model portalMembraneModel;
    int portalTimeLoc = -1;
    int portalIntensityLoc = -1;
    float portalTime = 0.0f;
    float ringSpinDegPerSec = 12.0f;

    float orbitYaw = 0.4f;
    float orbitPitch = 0.35f;
    float orbitDistance = 4.0f;

    void drawActiveModel();
};

#endif // MODEL_3D_TEST_SCENE_HPP
