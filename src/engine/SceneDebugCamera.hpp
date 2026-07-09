#ifndef SCENE_DEBUG_CAMERA_HPP
#define SCENE_DEBUG_CAMERA_HPP

#include "SceneEffect.hpp"
#include "SceneCamera.hpp"
#include "raylib.h"

// Shared live-tuning controls for any SceneEffect that implements
// SceneEffect's debug-camera interface (hasDebugCamera() == true) -- e.g.
// CityWindowEffect. Numpad 8/2 adjusts camera distance, 9/3 pitch, 7/1 fovy.
// Any scene can call updateSceneDebugCamera() from its own update() and
// drawSceneDebugCameraReadout() from its own draw() against whichever effect
// it holds, without needing to know the effect's concrete type or duplicate
// this control logic per scene (previously only ScenePreviewScene had this,
// wired directly to CityWindowEffect via a dynamic_cast).
inline void updateSceneDebugCamera(SceneEffect* effect, SceneCamera* camera, float deltaTime) {
    if (!effect || !effect->hasDebugCamera()) return;

    float dist = effect->getDebugCamDist();
    if (IsKeyDown(KEY_KP_8)) dist -= 20.0f * deltaTime;
    if (IsKeyDown(KEY_KP_2)) dist += 20.0f * deltaTime;
    if (dist < 0.1f) dist = 0.1f;
    effect->setDebugCamDist(dist);

    float pitch = effect->getDebugPitch();
    if (IsKeyDown(KEY_KP_9)) pitch += 20.0f * deltaTime;
    if (IsKeyDown(KEY_KP_3)) pitch -= 20.0f * deltaTime;
    effect->setDebugPitch(pitch);

    float fovy = effect->getDebugFovy();
    if (IsKeyDown(KEY_KP_7)) fovy -= 20.0f * deltaTime;
    if (IsKeyDown(KEY_KP_1)) fovy += 20.0f * deltaTime;
    if (fovy < 1.0f) fovy = 1.0f;
    if (fovy > 170.0f) fovy = 170.0f;
    effect->setDebugFovy(fovy);
}

inline void drawSceneDebugCameraReadout(SceneEffect* effect, int x, int y) {
    if (!effect || !effect->hasDebugCamera()) return;

    const char* txt1 = TextFormat("cam dist: %.2f   (Numpad 8/2: closer/farther)", effect->getDebugCamDist());
    DrawText(txt1, x, y, 18, Color{255, 220, 140, 255});
    const char* txt2 = TextFormat("pitch: %.1f deg (Numpad 9/3)   fov: %.1f deg (Numpad 7/1)",
        effect->getDebugPitch(), effect->getDebugFovy());
    DrawText(txt2, x, y + 22, 18, Color{255, 220, 140, 255});
}

#endif // SCENE_DEBUG_CAMERA_HPP
