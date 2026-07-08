// This translation unit hosts the single rlights.h implementation for the whole
// program. RLIGHTS_IMPLEMENTATION must be defined in exactly one .cpp before the
// header is included; every other TU includes rlights.h (via MoonEffect.hpp) for
// declarations only.
#define RLIGHTS_IMPLEMENTATION
#include "MoonEffect.hpp"
#include "LitShader.hpp"

#include "raymath.h"
#include <cmath>

void MoonEffect::init() {
    shader = LoadLitShader();
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int ambLoc = GetShaderLocation(shader, "ambient");
    float amb[4] = {0.4f, 0.4f, 0.5f, 1.0f};
    SetShaderValue(shader, ambLoc, amb, SHADER_UNIFORM_VEC4);
    model = LoadModelFromMesh(GenMeshSphere(0.2f, 6, 6));
    model.materials[0].shader = shader;
    // Load a solid white texture so texelColor.a is always 1
    Image white = GenImageColor(1, 1, WHITE);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = LoadTextureFromImage(white);
    UnloadImage(white);
    model.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = {220, 220, 180, 255};
    light = CreateLight0(LIGHT_DIRECTIONAL, {6.0f, 8.0f, 4.0f}, {0,0,0}, WHITE, shader);
}

void MoonEffect::update(float deltaTime) {
    rotation += 18.0f * deltaTime;
    float t = (float)GetTime();
    light.position = {0.8f + 4.0f * sinf(t * 0.07f), 0.7f + 4.0f * cosf(t * 0.11f), 3.0f * sinf(t * 0.05f)};
    light.target   = {0.8f, 0.7f, 0.0f};
    UpdateLightValues(shader, light);
    float camPos[3] = {0.0f, 0.0f, 10.0f};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
}

void MoonEffect::drawBackground() {
    Camera3D cam3d = {};
    cam3d.position   = {0.0f, 0.0f, 10.0f};
    cam3d.target     = {0.0f, 0.0f, 0.0f};
    cam3d.up         = {0.0f, 1.0f, 0.0f};
    cam3d.fovy       = 10.0f;
    cam3d.projection = CAMERA_PERSPECTIVE;
    BeginMode3D(cam3d);
        DrawModelEx(model, {0.8f, 0.7f, 0.0f}, {0.0f, 1.0f, 0.3f}, rotation, {1.0f, 1.0f, 1.0f}, {220, 220, 180, 255});
    EndMode3D();
}

void MoonEffect::cleanup() {
    UnloadTexture(model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture);
    UnloadShader(shader);
    UnloadModel(model);
}
