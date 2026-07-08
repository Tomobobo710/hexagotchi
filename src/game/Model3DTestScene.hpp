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
    Shader shader;
    Light light;
    Model jetModel;
    Texture2D whiteTex = {0};

    float orbitYaw = 0.4f;
    float orbitPitch = 0.35f;
    float orbitDistance = 4.0f;
};

#endif // MODEL_3D_TEST_SCENE_HPP
