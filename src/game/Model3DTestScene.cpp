#include "Model3DTestScene.hpp"
#include "GameConstants.hpp"
#include "JetMesh.hpp"
#include "LitShader.hpp"
#include "raymath.h"
#include <cmath>

Model3DTestScene::Model3DTestScene() : Scene((float)GAME_W, (float)GAME_H, Color{60, 70, 90, 255}) {
}

void Model3DTestScene::init() {
    shader = LoadLitShader();
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    int ambLoc = GetShaderLocation(shader, "ambient");
    float amb[4] = {0.45f, 0.45f, 0.5f, 1.0f};
    SetShaderValue(shader, ambLoc, amb, SHADER_UNIFORM_VEC4);

    Image white = GenImageColor(1, 1, WHITE);
    whiteTex = LoadTextureFromImage(white);
    UnloadImage(white);

    Mesh jetMesh = BuildJetMesh();
    jetModel = LoadModelFromMesh(jetMesh);
    jetModel.materials[0].shader = shader;
    jetModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    jetModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    // Same "from behind-above, down-and-away at 45" light direction used in
    // SchoolSkyEffect, so this preview matches how it'll actually look there.
    light = CreateLight0(LIGHT_DIRECTIONAL, {0.0f, 4.0f, 6.0f}, {0.0f, -4.0f, -6.0f}, WHITE, shader);
}

void Model3DTestScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        Vector2 delta = GetMouseDelta();
        orbitYaw += delta.x * 0.01f;
        orbitPitch += delta.y * 0.01f;
        if (orbitPitch > 1.5f) orbitPitch = 1.5f;
        if (orbitPitch < -1.5f) orbitPitch = -1.5f;
    }
    orbitDistance -= GetMouseWheelMove() * 0.4f;
    if (orbitDistance < 1.2f) orbitDistance = 1.2f;
    if (orbitDistance > 12.0f) orbitDistance = 12.0f;

    float camX = cosf(orbitPitch) * sinf(orbitYaw) * orbitDistance;
    float camY = sinf(orbitPitch) * orbitDistance;
    float camZ = cosf(orbitPitch) * cosf(orbitYaw) * orbitDistance;
    float camPos[3] = {camX, camY, camZ};
    SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], camPos, SHADER_UNIFORM_VEC3);
    UpdateLightValues(shader, light);
}

void Model3DTestScene::draw() {
    Scene::draw();

    float camX = cosf(orbitPitch) * sinf(orbitYaw) * orbitDistance;
    float camY = sinf(orbitPitch) * orbitDistance;
    float camZ = cosf(orbitPitch) * cosf(orbitYaw) * orbitDistance;

    Camera3D cam3d = {};
    cam3d.position = {camX, camY, camZ};
    cam3d.target = {0.0f, 0.0f, 0.0f};
    cam3d.up = {0.0f, 1.0f, 0.0f};
    cam3d.fovy = 45.0f;
    cam3d.projection = CAMERA_PERSPECTIVE;

    BeginMode3D(cam3d);
        DrawModelEx(jetModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
        DrawGrid(10, 0.3f);
    EndMode3D();

    DrawText("3D MODEL TEST", 20, 20, 28, RAYWHITE);
    DrawText("Drag: orbit   Wheel: zoom", 20, 54, 18, Color{200, 200, 210, 255});
    DrawText("7: Scene Select", 20, GAME_H - 30, 18, Color{200, 200, 210, 255});
}

void Model3DTestScene::cleanup() {
    Scene::cleanup();
    UnloadModel(jetModel);
    UnloadTexture(whiteTex);
    UnloadShader(shader);
}
