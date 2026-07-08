#include "GrimeShader.hpp"
#include <cstddef>

// The grime shader is embedded as strings and compiled via LoadShaderFromMemory.
// Two versions are required because the GL backends are not source-compatible:
//   - Web (WebGL 2.0 / GLSL ES 300 es) needs #version 300 es  (in/out/finalColor)
//   - Desktop (OpenGL 3.3 core) needs #version 330 (in/out/finalColor)
//
// Raylib provides a default vertex shader when NULL is passed to LoadShaderFromMemory.
// The default shader uses the same attribute names in both desktop and web variants:
//   - vertexPosition (vec3), vertexTexCoord (vec2), vertexColor (vec4)
//   - Outputs vertexColor as 'color' to the fragment shader
//
// The shader operates on 'color' which is either:
//   - texture * tint for sprite draws (Gotchi with animation frames)
//   - the color passed to procedural shape functions (DrawRectangle, etc.)
//
// It:
//   1. Desaturates toward luminance as grime increases
//   2. Multiplies rgb by a darkening factor
//   3. Adds procedural noise keyed off gl_FragCoord for blotchy grime
//
// Identity at 0: grimeAmount == 0 outputs the base color unchanged.
// Preserves alpha: only rgb is modified, alpha passes through untouched.

// GLSL fragment shader for desktop (OpenGL 3.3 core)
static const char* FRAG_DESKTOP = R"(#version 330

// Tunable constants - adjust for desired visual effect
#define DESAT       0.85   // how much saturation is lost at grime = 1
#define DARK_FLOOR  0.55   // brightness multiplier at grime = 1
#define GRIME_NOISE 0.25   // strength of procedural grime blotches at grime = 1

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float grimeAmount;

// Simple hash function for noise - works in both 330 and 300 es
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

// 2D noise function using hash - works in both 330 and 300 es
float noise(vec2 p) {
    float i = floor(p.x);
    float f = fract(p.x);
    float g = floor(p.y);
    float h = fract(p.y);

    float v00 = hash(i + g * 57.0);
    float v10 = hash(i + 1.0 + g * 57.0);
    float v01 = hash(i + (g + 1.0) * 57.0);
    float v11 = hash(i + 1.0 + (g + 1.0) * 57.0);

    float u = f * f * (3.0 - 2.0 * f);
    float v = h * h * (3.0 - 2.0 * h);

    float x00 = mix(v00, v10, u);
    float x01 = mix(v01, v11, u);
    return mix(x00, x01, v);
}

vec3 desaturate(vec3 color, float amount) {
    // Compute luminance using Rec. 709 coefficients
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(color, vec3(luma), amount);
}

void main() {
    // Sample the texture and compute base color
    vec4 texelColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    // Identity at zero - output original texel color
    if (grimeAmount <= 0.0) {
        finalColor = texelColor;
        return;
    }

    // Clamp grime amount
    float g = clamp(grimeAmount, 0.0, 1.0);

    // 1. Desaturate toward luminance
    vec3 resultColor = desaturate(texelColor.rgb, g * DESAT);

    // 2. Darken: multiply by mix(1.0, DARK_FLOOR, grimeAmount)
    resultColor *= mix(1.0, DARK_FLOOR, g);

    // 3. Procedural grime noise keyed off fragCoord
    // Use a coarse grid so blotches are visible even on small sprites
    vec2 grid = floor(gl_FragCoord.xy / 8.0);
    float noiseVal = noise(grid);
    // Subtract a little darkness where noise is high - creates irregular blotches
    resultColor *= (1.0 - g * GRIME_NOISE * noiseVal);

    // 4. Preserve alpha from the texture, not from fragColor
    finalColor = vec4(resultColor, texelColor.a);
}
)";

// GLSL fragment shader for web (WebGL 2.0 / GLSL ES 300)
static const char* FRAG_WEB = R"(#version 300 es

precision mediump float;

// Tunable constants - adjust for desired visual effect
#define DESAT       0.85   // how much saturation is lost at grime = 1
#define DARK_FLOOR  0.55   // brightness multiplier at grime = 1
#define GRIME_NOISE 0.25   // strength of procedural grime blotches at grime = 1

in vec2 fragTexCoord;
in vec4 fragColor;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float grimeAmount;

// Simple hash function for noise - works in both 330 and 300 es
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

// 2D noise function using hash - works in both 330 and 300 es
float noise(vec2 p) {
    float i = floor(p.x);
    float f = fract(p.x);
    float g = floor(p.y);
    float h = fract(p.y);

    float v00 = hash(i + g * 57.0);
    float v10 = hash(i + 1.0 + g * 57.0);
    float v01 = hash(i + (g + 1.0) * 57.0);
    float v11 = hash(i + 1.0 + (g + 1.0) * 57.0);

    float u = f * f * (3.0 - 2.0 * f);
    float v = h * h * (3.0 - 2.0 * h);

    float x00 = mix(v00, v10, u);
    float x01 = mix(v01, v11, u);
    return mix(x00, x01, v);
}

vec3 desaturate(vec3 color, float amount) {
    // Compute luminance using Rec. 709 coefficients
    float luma = dot(color, vec3(0.2126, 0.7152, 0.0722));
    return mix(color, vec3(luma), amount);
}

void main() {
    // Sample the texture and compute base color
    vec4 texelColor = texture(texture0, fragTexCoord) * colDiffuse * fragColor;

    // Identity at zero - output original texel color
    if (grimeAmount <= 0.0) {
        finalColor = texelColor;
        return;
    }

    // Clamp grime amount
    float g = clamp(grimeAmount, 0.0, 1.0);

    // 1. Desaturate toward luminance
    vec3 resultColor = desaturate(texelColor.rgb, g * DESAT);

    // 2. Darken: multiply by mix(1.0, DARK_FLOOR, grimeAmount)
    resultColor *= mix(1.0, DARK_FLOOR, g);

    // 3. Procedural grime noise keyed off fragCoord
    // Use a coarse grid so blotches are visible even on small sprites
    vec2 grid = floor(gl_FragCoord.xy / 8.0);
    float noiseVal = noise(grid);
    // Subtract a little darkness where noise is high - creates irregular blotches
    resultColor *= (1.0 - g * GRIME_NOISE * noiseVal);

    // 4. Preserve alpha from the texture, not from fragColor
    finalColor = vec4(resultColor, texelColor.a);
}
)";

#if defined(PLATFORM_WEB)
    #define GRIME_FS FRAG_WEB
#else
    #define GRIME_FS FRAG_DESKTOP
#endif

// Lazy initialization on first use
GrimeShader& GrimeShader::instance() {
    static GrimeShader singleton;
    return singleton;
}

void GrimeShader::begin(float grimeAmount) {
    if (!loaded_) {
        // Load shader with NULL vertex shader - raylib provides default
        // The default shader outputs 'color' which our fragment shader reads
        shader_ = LoadShaderFromMemory(NULL, GRIME_FS);
        if (shader_.id != 0) {
            grimeLoc_ = GetShaderLocation(shader_, "grimeAmount");
            loaded_ = true;
        }
    }

    if (loaded_) {
        BeginShaderMode(shader_);
        float val = grimeAmount;
        SetShaderValue(shader_, grimeLoc_, &val, SHADER_UNIFORM_FLOAT);
    }
}

void GrimeShader::end() {
    if (loaded_) {
        EndShaderMode();
    }
}
