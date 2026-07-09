#include "PortalShader.hpp"

// Unlit -- the membrane emits its own light, it doesn't reflect the scene's
// directional light -- so this is a much smaller shader than LitShader:
// just position/UV in, a self-contained animated pattern out.

static const char* PORTAL_VS_100 = R"(#version 100

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

static const char* PORTAL_FS_100 = R"(#version 100

precision mediump float;

varying vec2 fragTexCoord;

uniform float time;
uniform float intensity;

void main()
{
    // UV origin at the membrane's center, radius 0..~0.7 at the corners.
    vec2 uv = fragTexCoord - vec2(0.5);
    float r = length(uv);
    float ang = atan(uv.y, uv.x);

    // Concentric rings scrolling inward: a sine of radius minus time, warped
    // by a second, faster/finer sine of angle so the rings aren't perfectly
    // circular -- reads as an unstable/energetic membrane rather than a
    // static bullseye.
    float warp = sin(ang * 7.0 + time * 1.6) * 0.035;
    float ringPhase = (r + warp) * 22.0 - time * 3.2;
    float rings = 0.5 + 0.5 * sin(ringPhase);
    rings = pow(rings, 3.0); // sharpen into thinner bright bands

    // Fresnel-style edge glow: brighter toward the rim (r near ~0.5), darker
    // in the middle, so the membrane reads as a lit boundary/window rather
    // than a flat disc.
    float edge = smoothstep(0.15, 0.55, r);
    float core = 1.0 - smoothstep(0.0, 0.25, r);

    // Slow cyan/magenta color cycle, independent of the ring pattern, so the
    // whole membrane's hue drifts over time instead of being static.
    vec3 colA = vec3(0.15, 0.95, 1.0);   // cyan
    vec3 colB = vec3(0.95, 0.15, 1.0);   // magenta
    float mixT = 0.5 + 0.5 * sin(time * 0.6);
    vec3 baseColor = mix(colA, colB, mixT);

    vec3 color = baseColor * (0.55 + rings * 1.1);
    color += baseColor * edge * 0.5;   // rim brightening
    color += vec3(1.0) * core * 0.35;  // near-white hot core

    // The membrane mesh is a square (see PortalEffect::BuildMembraneMesh)
    // but only the disc inside the ring should ever be visible -- fade to
    // fully transparent past r=0.5 so the corners outside the ring don't
    // show as a visible square behind the frame.
    float circleMask = 1.0 - smoothstep(0.46, 0.5, r);

    float alpha = clamp(0.55 + rings * 0.35 + edge * 0.3, 0.0, 1.0) * circleMask;

    gl_FragColor = vec4(color * intensity, alpha * clamp(intensity, 0.0, 1.0));
}
)";

static const char* PORTAL_VS_330 = R"(#version 330

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

static const char* PORTAL_FS_330 = R"(#version 330

in vec2 fragTexCoord;

uniform float time;
uniform float intensity;

out vec4 finalColor;

void main()
{
    vec2 uv = fragTexCoord - vec2(0.5);
    float r = length(uv);
    float ang = atan(uv.y, uv.x);

    float warp = sin(ang * 7.0 + time * 1.6) * 0.035;
    float ringPhase = (r + warp) * 22.0 - time * 3.2;
    float rings = 0.5 + 0.5 * sin(ringPhase);
    rings = pow(rings, 3.0);

    float edge = smoothstep(0.15, 0.55, r);
    float core = 1.0 - smoothstep(0.0, 0.25, r);

    vec3 colA = vec3(0.15, 0.95, 1.0);
    vec3 colB = vec3(0.95, 0.15, 1.0);
    float mixT = 0.5 + 0.5 * sin(time * 0.6);
    vec3 baseColor = mix(colA, colB, mixT);

    vec3 color = baseColor * (0.55 + rings * 1.1);
    color += baseColor * edge * 0.5;
    color += vec3(1.0) * core * 0.35;

    float circleMask = 1.0 - smoothstep(0.46, 0.5, r);

    float alpha = clamp(0.55 + rings * 0.35 + edge * 0.3, 0.0, 1.0) * circleMask;

    finalColor = vec4(color * intensity, alpha * clamp(intensity, 0.0, 1.0));
}
)";

#if defined(PLATFORM_WEB)
    #define PORTAL_VS PORTAL_VS_100
    #define PORTAL_FS PORTAL_FS_100
#else
    #define PORTAL_VS PORTAL_VS_330
    #define PORTAL_FS PORTAL_FS_330
#endif

Shader LoadPortalShader() {
    return LoadShaderFromMemory(PORTAL_VS, PORTAL_FS);
}
