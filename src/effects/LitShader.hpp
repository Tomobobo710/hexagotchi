#ifndef LIT_SHADER_HPP
#define LIT_SHADER_HPP

#include "raylib.h"

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

#endif // LIT_SHADER_HPP
