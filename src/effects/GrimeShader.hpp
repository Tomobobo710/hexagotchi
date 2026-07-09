#ifndef GRIME_SHADER_HPP
#define GRIME_SHADER_HPP

#include "raylib.h"

// GrimeShader - applies a procedural grime effect to the Gotchi character
// in story scenes. The grimeAmount uniform (0.0 = pristine, 1.0 = fully neglected)
// desaturates, darkens, and adds procedural noise to the character's appearance.
//
// This is a single shader instance that wraps the Gotchi draw call. It works
// for texture-based draws (Gotchi with animation frames) by sampling texture0
// and applying the grime effect to the texel color.
//
// Usage:
//     GrimeShader& shader = GrimeShader::instance();
//     shader.begin(state.grime);
//     // draw Gotchi here (SceneActor::draw())
//     shader.end();
//
// The shader is lazily initialized on first use. It is NOT a SceneEffect -
// it does not render full-screen; it wraps individual character draws.

class GrimeShader {
public:
    static GrimeShader& instance();   // lazy GL-init on first begin()

    void begin(float grimeAmount);    // BeginShaderMode + set uniform
    void end();                       // EndShaderMode

    bool loaded() const { return loaded_; }

private:
    GrimeShader() = default;
    ~GrimeShader() = default;

    Shader shader_ = {0};
    bool loaded_ = false;
    int grimeLoc_ = -1;
};

#endif // GRIME_SHADER_HPP
