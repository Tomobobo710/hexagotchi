#include "UnlitShader.hpp"

// Same #version-100-vs-330 split as LitShader.cpp, for the same reason (web
// GLSL ES vs desktop core profile aren't source-compatible).

static const char* UNLIT_VS_100 = R"(#version 100

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec4 vertexColor;

uniform mat4 mvp;

varying vec2 fragTexCoord;
varying vec4 fragColor;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

static const char* UNLIT_FS_100 = R"(#version 100

precision mediump float;

varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

void main()
{
    vec4 texelColor = texture2D(texture0, fragTexCoord);
    gl_FragColor = texelColor*colDiffuse*fragColor;
}
)";

static const char* UNLIT_VS_330 = R"(#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec4 vertexColor;

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

static const char* UNLIT_FS_330 = R"(#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = texelColor*colDiffuse*fragColor;
}
)";

#if defined(PLATFORM_WEB)
    #define UNLIT_VS UNLIT_VS_100
    #define UNLIT_FS UNLIT_FS_100
#else
    #define UNLIT_VS UNLIT_VS_330
    #define UNLIT_FS UNLIT_FS_330
#endif

Shader LoadUnlitShader() {
    return LoadShaderFromMemory(UNLIT_VS, UNLIT_FS);
}
