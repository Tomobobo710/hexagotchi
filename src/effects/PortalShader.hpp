#ifndef PORTAL_SHADER_HPP
#define PORTAL_SHADER_HPP

#include "raylib.h"

// Shader for the office teleporter/merge-machine's inner membrane (see
// PortalEffect) -- an unlit fullscreen-in-quad effect, not a LitShader user:
// concentric warped rings scrolling inward, a bright fresnel-style edge glow,
// and a slow cyan/magenta color cycle. Same LoadShaderFromMemory /
// web-vs-desktop-GLSL-version split as LitShader.hpp.
//
// Uniforms the caller must set every frame:
//   "time"      float  -- seconds, drives the scroll/cycle animation
//   "intensity" float  -- 0..~1.5, overall brightness/energy (e.g. pulses
//                          up when the portal actually activates)
Shader LoadPortalShader();

#endif // PORTAL_SHADER_HPP
