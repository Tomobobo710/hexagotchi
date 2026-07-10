#ifndef UNLIT_SHADER_HPP
#define UNLIT_SHADER_HPP

#include "raylib.h"

// Flat, full-bright shader: outputs vertex color * texture with no lighting
// math at all (no ambient, no directional/specular terms). Use this for
// models where each triangle's baked color IS the final on-screen color --
// e.g. a Blender-materials-painted toy prop -- since LitShader's ambient term
// washes out dark/black vertex colors into gray (it adds ambient*texelColor
// unconditionally, not scaled by the vertex color -- see LitShader.cpp).
//
// Caller still owns the returned Shader and must UnloadShader() it, same as
// LoadShaderFromMemory() always requires.
Shader LoadUnlitShader();

#endif // UNLIT_SHADER_HPP
