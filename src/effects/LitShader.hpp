#ifndef LIT_SHADER_HPP
#define LIT_SHADER_HPP

#include "raylib.h"
#include "rlights.h"

// Shared directional-lit shader used by every 3D SceneEffect (MoonEffect,
// SchoolSkyEffect, and any future one) -- compiles the same GLSL that used
// to be duplicated verbatim in each effect's .cpp (web #version 100 vs
// desktop #version 330, picked via PLATFORM_WEB). The *shader* is genuinely
// identical across effects and worth sharing; each effect's *placement*
// (camera, light angle/color, ambient level, model position) stays local to
// that effect -- see the "one-off per scene" note in JetMesh.hpp/memory.
//
// Caller still owns the returned Shader and must UnloadShader() it, same as
// LoadShaderFromMemory() always required.
Shader LoadLitShader();

// rlights.h's CreateLight() uses a process-global lightsCount that never
// resets, incrementing every time any effect creates a light -- fine for a
// single static scene, but every effect here loads its OWN shader instance
// and always wants light slot 0 of it. Repeatedly constructing/destroying
// effects (e.g. cycling through ScenePreviewScene) exhausts the shared
// MAX_LIGHTS=4 budget after 4 total CreateLight() calls across the whole
// program, after which CreateLight() silently returns a disabled, unbound
// Light{0} forever -- this bypasses that counter and always binds slot 0.
Light CreateLight0(int type, Vector3 position, Vector3 target, Color color, Shader shader);

#endif // LIT_SHADER_HPP
