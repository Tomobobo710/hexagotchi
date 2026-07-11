#include "TitleScrollShader.hpp"
#include <cmath>

// TitleScrollShader - an infinitely scrolling background shader inspired by
// Toe Tam & Earl's funky liquid/gas background effects. Uses perlin-style
// noise with multiple layers moving at different speeds and directions to
// create a rich, organic scroll effect.

// Vertex shader - passes through position and UV
static const char* TITLE_SCROLL_VS_100 = R"(#version 100

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;

uniform mat4 mvp;

varying vec2 fragTexCoord;

void main()
{
    fragTexCoord = vertexTexCoord;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

static const char* TITLE_SCROLL_FS_100 = R"(#version 100

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

varying vec2 fragTexCoord;

uniform float time;
uniform vec3 color1;
uniform vec3 color2;
uniform vec3 color3;
uniform float aspect;

// Simple hash function for noise
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

// 2D noise function using hash
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

// Fractal noise - multiple octaves for richer detail
float fbm(vec2 p) {
    float total = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    // 4 octaves of noise
    for (int i = 0; i < 4; i++) {
        total += noise(p * frequency) * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return total;
}

void main()
{
    vec2 uv = fragTexCoord;
    uv.x *= aspect;

    // Base scales chosen so several noise cells span the screen. TUNABLE.
    vec2 layer1 = uv * 3.0 + vec2(time * 0.3, time * 0.2);
    vec2 layer2 = uv * 5.0 + vec2(time * -0.4, time * 0.5);
    vec2 layer3 = uv * 8.0 + vec2(time * 0.2, time * -0.3);

    // Sample each layer with FBM
    float val1 = fbm(layer1);
    float val2 = fbm(layer2);
    float val3 = fbm(layer3);

    // Combine layers
    float combined = (val1 + val2 + val3) / 3.0;

    // Soft clamp and sharpen
    combined = smoothstep(0.2, 0.8, combined);

    // Mix colors based on the noise values
    vec3 result = mix(color1, color2, combined);
    result = mix(result, color3, combined * 0.5);

    // Add some subtle variation for more organic feel
    float detail = fbm(uv * 12.0 + vec2(time * 0.1));
    result += (color1 - color2) * detail * 0.1;

    gl_FragColor = vec4(result, 1.0);
}
)";

static const char* TITLE_SCROLL_VS_330 = R"(#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;

uniform mat4 mvp;

out vec2 fragTexCoord;

void main()
{
    fragTexCoord = vertexTexCoord;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

static const char* TITLE_SCROLL_FS_330 = R"(#version 330

in vec2 fragTexCoord;

uniform float time;
uniform vec3 color1;
uniform vec3 color2;
uniform vec3 color3;
uniform float aspect;

out vec4 finalColor;

// Simple hash function for noise
float hash(float n) {
    return fract(sin(n) * 43758.5453);
}

// 2D noise function using hash
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

// Fractal noise - multiple octaves for richer detail
float fbm(vec2 p) {
    float total = 0.0;
    float amplitude = 0.5;
    float frequency = 1.0;

    // 4 octaves of noise
    for (int i = 0; i < 4; i++) {
        total += noise(p * frequency) * amplitude;
        amplitude *= 0.5;
        frequency *= 2.0;
    }
    return total;
}

void main()
{
    vec2 uv = fragTexCoord;
    uv.x *= aspect;

    // Base scales chosen so several noise cells span the screen. TUNABLE.
    vec2 layer1 = uv * 3.0 + vec2(time * 0.3, time * 0.2);
    vec2 layer2 = uv * 5.0 + vec2(time * -0.4, time * 0.5);
    vec2 layer3 = uv * 8.0 + vec2(time * 0.2, time * -0.3);

    // Sample each layer with FBM
    float val1 = fbm(layer1);
    float val2 = fbm(layer2);
    float val3 = fbm(layer3);

    // Combine layers
    float combined = (val1 + val2 + val3) / 3.0;

    // Soft clamp and sharpen
    combined = smoothstep(0.2, 0.8, combined);

    // Mix colors based on the noise values
    vec3 result = mix(color1, color2, combined);
    result = mix(result, color3, combined * 0.5);

    // Add some subtle variation for more organic feel
    float detail = fbm(uv * 12.0 + vec2(time * 0.1));
    result += (color1 - color2) * detail * 0.1;

    finalColor = vec4(result, 1.0);
}
)";

#if defined(PLATFORM_WEB)
    #define TITLE_SCROLL_VS TITLE_SCROLL_VS_100
    #define TITLE_SCROLL_FS TITLE_SCROLL_FS_100
#else
    #define TITLE_SCROLL_VS TITLE_SCROLL_VS_330
    #define TITLE_SCROLL_FS TITLE_SCROLL_FS_330
#endif

// Global shader loader for direct use if needed
Shader LoadTitleScrollShader() {
    return LoadShaderFromMemory(TITLE_SCROLL_VS, TITLE_SCROLL_FS);
}

// TitleScrollShader class implementation

TitleScrollShader::TitleScrollShader()
    : time_(0.0f)
    , color1_({64, 128, 192, 255})
    , color2_({128, 64, 160, 255})
    , color3_({192, 128, 64, 255})
{
}

void TitleScrollShader::init() {
    shader_ = LoadShaderFromMemory(TITLE_SCROLL_VS, TITLE_SCROLL_FS);
    if (shader_.id != 0) {
        timeLoc_ = GetShaderLocation(shader_, "time");
        aspectLoc_ = GetShaderLocation(shader_, "aspect");
        color1Loc_ = GetShaderLocation(shader_, "color1");
        color2Loc_ = GetShaderLocation(shader_, "color2");
        color3Loc_ = GetShaderLocation(shader_, "color3");

        // Create a 1x1 white texture for drawing the fullscreen quad
        Image white = GenImageColor(1, 1, WHITE);
        texture_ = LoadTextureFromImage(white);
        UnloadImage(white);

        loaded_ = true;
    }
}

void TitleScrollShader::update(float deltaTime) {
    if (loaded_) {
        time_ += deltaTime;

        // Update time uniform
        SetShaderValue(shader_, timeLoc_, &time_, SHADER_UNIFORM_FLOAT);

        // Update aspect ratio uniform (handles window resize)
        float aspect = (float)GetScreenWidth() / (float)GetScreenHeight();
        SetShaderValue(shader_, aspectLoc_, &aspect, SHADER_UNIFORM_FLOAT);

        // Update color uniforms each frame
        float col1[3] = {color1_.r / 255.0f, color1_.g / 255.0f, color1_.b / 255.0f};
        float col2[3] = {color2_.r / 255.0f, color2_.g / 255.0f, color2_.b / 255.0f};
        float col3[3] = {color3_.r / 255.0f, color3_.g / 255.0f, color3_.b / 255.0f};
        SetShaderValue(shader_, color1Loc_, col1, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader_, color2Loc_, col2, SHADER_UNIFORM_VEC3);
        SetShaderValue(shader_, color3Loc_, col3, SHADER_UNIFORM_VEC3);
    }
}

void TitleScrollShader::drawBackground() {
    if (!loaded_) return;

    BeginShaderMode(shader_);

    // Draw a fullscreen quad using raylib's texture drawing
    // The shader will be applied to everything drawn between BeginShaderMode/EndShaderMode
    Rectangle screenRect = {0, 0, (float)GetScreenWidth(), (float)GetScreenHeight()};
    Rectangle texRect = {0, 0, 1.0f, 1.0f};  // Full texture

    DrawTexturePro(texture_, texRect, screenRect, Vector2{0, 0}, 0.0f, WHITE);

    EndShaderMode();
}

void TitleScrollShader::cleanup() {
    if (loaded_) {
        UnloadTexture(texture_);
        UnloadShader(shader_);
        loaded_ = false;
    }
}
