#include "Model3DTestScene.hpp"
#include "GameConstants.hpp"
#include "JetMesh.hpp"
#include "PortalRingMesh.hpp"
#include "PortalBaseMesh.hpp"
#include "PortalEffect.hpp"
#include "PortalShader.hpp"
#include "VehicleMesh.hpp"
#include "TomagotchiToyMesh.hpp"
#include "LitShader.hpp"
#include "UnlitShader.hpp"
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

    Mesh portalRingMesh = BuildPortalRingMesh();
    portalRingModel = LoadModelFromMesh(portalRingMesh);
    portalRingModel.materials[0].shader = shader;
    portalRingModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    portalRingModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    Mesh portalBaseMesh = BuildPortalBaseMesh();
    portalBaseModel = LoadModelFromMesh(portalBaseMesh);
    portalBaseModel.materials[0].shader = shader;
    portalBaseModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    portalBaseModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    Mesh carMesh = BuildVehicleMesh(false, Color{180, 40, 40, 255});
    carModel = LoadModelFromMesh(carMesh);
    carModel.materials[0].shader = shader;
    carModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    carModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    Mesh truckMesh = BuildVehicleMesh(true, Color{60, 42, 30, 255}, Color{110, 112, 116, 255});
    truckModel = LoadModelFromMesh(truckMesh);
    truckModel.materials[0].shader = shader;
    truckModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    truckModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    unlitShader = LoadUnlitShader();

    Mesh tomagotchiToyMesh = BuildTomagotchiToyMesh();
    tomagotchiToyModel = LoadModelFromMesh(tomagotchiToyMesh);
    tomagotchiToyModel.materials[0].shader = unlitShader;
    tomagotchiToyModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = whiteTex;
    tomagotchiToyModel.materials[0].maps[MATERIAL_MAP_DIFFUSE].color = WHITE;

    // Same "from behind-above, down-and-away at 45" light direction used in
    // SchoolSkyEffect, so this preview matches how it'll actually look there.
    light = CreateLight0(LIGHT_DIRECTIONAL, {0.0f, 4.0f, 6.0f}, {0.0f, -4.0f, -6.0f}, WHITE, shader);

    // Membrane: own unlit animated shader, same as PortalEffect -- lets the
    // COMBINED preview actually show the shimmering portal surface rather
    // than just the two lit meshes.
    portalShader = LoadPortalShader();
    portalTimeLoc = GetShaderLocation(portalShader, "time");
    portalIntensityLoc = GetShaderLocation(portalShader, "intensity");

    Mesh membraneMesh = BuildMembraneMesh(RING_RADIUS - RING_TUBE_THICKNESS * 0.5f);
    portalMembraneModel = LoadModelFromMesh(membraneMesh);
    portalMembraneModel.materials[0].shader = portalShader;
}

void Model3DTestScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (IsKeyPressed(KEY_TAB)) {
        int next = (int)modelKind + 1;
        if (next > (int)ModelKind::TOMAGOTCHI_TOY) next = (int)ModelKind::JET;
        modelKind = (ModelKind)next;
    }

    if (modelKind == ModelKind::TOMAGOTCHI_TOY && IsKeyPressed(KEY_Z)) {
        tomagotchiToyUseUnlit = !tomagotchiToyUseUnlit;
        tomagotchiToyModel.materials[0].shader = tomagotchiToyUseUnlit ? unlitShader : shader;
    }

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

    if (modelKind == ModelKind::PORTAL_COMBINED) {
        portalTime += deltaTime;
        float intensity = 0.6f;
        SetShaderValue(portalShader, portalTimeLoc, &portalTime, SHADER_UNIFORM_FLOAT);
        SetShaderValue(portalShader, portalIntensityLoc, &intensity, SHADER_UNIFORM_FLOAT);
    }
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
        drawActiveModel();
        DrawGrid(10, 0.3f);
    EndMode3D();

    const char* label = "3D MODEL TEST: JET";
    switch (modelKind) {
        case ModelKind::PORTAL_RING:     label = "3D MODEL TEST: PORTAL RING"; break;
        case ModelKind::PORTAL_BASE:     label = "3D MODEL TEST: PORTAL BASE"; break;
        case ModelKind::PORTAL_COMBINED: label = "3D MODEL TEST: PORTAL (COMBINED, SPINNING)"; break;
        case ModelKind::CAR:             label = "3D MODEL TEST: CAR"; break;
        case ModelKind::TRUCK:           label = "3D MODEL TEST: TRUCK"; break;
        case ModelKind::TOMAGOTCHI_TOY:  label = "3D MODEL TEST: TOMAGOTCHI TOY"; break;
        default: break;
    }
    DrawText(label, 20, 20, 28, RAYWHITE);
    DrawText("Drag: orbit   Wheel: zoom   TAB: switch model", 20, 54, 18, Color{200, 200, 210, 255});
    if (modelKind == ModelKind::TOMAGOTCHI_TOY) {
        const char* shaderLabel = tomagotchiToyUseUnlit ? "Z: shader (UNLIT)" : "Z: shader (LIT)";
        DrawText(shaderLabel, 20, 78, 18, Color{200, 200, 210, 255});
    }
    DrawText("7: Scene Select", 20, GAME_H - 30, 18, Color{200, 200, 210, 255});
}

void Model3DTestScene::drawActiveModel() {
    if (modelKind == ModelKind::JET) {
        DrawModelEx(jetModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
        return;
    }
    if (modelKind == ModelKind::PORTAL_RING) {
        DrawModelEx(portalRingModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
        return;
    }
    if (modelKind == ModelKind::PORTAL_BASE) {
        DrawModelEx(portalBaseModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
        return;
    }
    if (modelKind == ModelKind::CAR) {
        DrawModelEx(carModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
        return;
    }
    if (modelKind == ModelKind::TRUCK) {
        DrawModelEx(truckModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
        return;
    }
    if (modelKind == ModelKind::TOMAGOTCHI_TOY) {
        DrawModelEx(tomagotchiToyModel, {0, 0, 0}, {0, 1, 0}, 0.0f, {1, 1, 1}, WHITE);
        return;
    }

    // PORTAL_COMBINED: base stays put, ring spins continuously around its
    // own Z axis, membrane follows the ring -- same transform composition
    // as PortalEffect::drawBackground(), just without the object-scale/
    // debug-yaw knobs (this preview is fixed at identity for those).
    float spinDeg = portalTime * ringSpinDegPerSec;
    Matrix ringSpin = MatrixRotateZ(spinDeg * DEG2RAD);

    portalBaseModel.transform = MatrixIdentity();
    DrawModel(portalBaseModel, {0, 0, 0}, 1.0f, WHITE);

    portalRingModel.transform = ringSpin;
    DrawModel(portalRingModel, {0, 0, 0}, 1.0f, WHITE);

    Vector3 membraneLocalOffset = {0.0f, 0.0f, RING_TUBE_THICKNESS * 0.3f};
    Vector3 membranePos = Vector3Transform(membraneLocalOffset, ringSpin);
    portalMembraneModel.transform = ringSpin;
    DrawModel(portalMembraneModel, membranePos, 1.0f, WHITE);
}

void Model3DTestScene::cleanup() {
    Scene::cleanup();
    UnloadModel(jetModel);
    UnloadModel(portalRingModel);
    UnloadModel(portalBaseModel);
    UnloadModel(portalMembraneModel);
    UnloadModel(carModel);
    UnloadModel(truckModel);
    UnloadModel(tomagotchiToyModel);
    UnloadTexture(whiteTex);
    UnloadShader(shader);
    UnloadShader(portalShader);
    UnloadShader(unlitShader);
}
