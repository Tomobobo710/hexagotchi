#define RLIGHTS_IMPLEMENTATION
#include "LitShader.hpp"

// The lighting shaders are embedded as strings and compiled via
// LoadShaderFromMemory, rather than loaded from files. This keeps the build
// free of any runtime asset files: nothing to ship next to the desktop exe,
// nothing to --preload-file on web, and no dependence on the working directory.
//
// Two versions are required because the GL backends are not source-compatible:
//   - Web (WebGL / GLSL ES) needs #version 100  (attribute/varying/gl_FragColor)
//   - Desktop (OpenGL 3.3 core) needs #version 330 (in/out/finalColor)

static const char* LIT_VS_100 = R"(#version 100

attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
attribute vec3 vertexNormal;
attribute vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

// GLSL ES 100 has no built-in inverse()/transpose(), so provide them.
// https://github.com/glslify/glsl-inverse
mat3 inverse(mat3 m)
{
  float a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
  float a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
  float a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];

  float b01 = a22*a11 - a12*a21;
  float b11 = -a22*a10 + a12*a20;
  float b21 = a21*a10 - a11*a20;

  float det = a00*b01 + a01*b11 + a02*b21;

  return mat3(b01, (-a22*a01 + a02*a21), (a12*a01 - a02*a11),
              b11, (a22*a00 - a02*a20), (-a12*a00 + a02*a10),
              b21, (-a21*a00 + a01*a20), (a11*a00 - a01*a10))/det;
}

mat3 transpose(mat3 m)
{
  return mat3(m[0][0], m[1][0], m[2][0],
              m[0][1], m[1][1], m[2][1],
              m[0][2], m[1][2], m[2][2]);
}

void main()
{
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

    mat3 normalMatrix = transpose(inverse(mat3(matModel)));
    fragNormal = normalize(normalMatrix*vertexNormal);

    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

static const char* LIT_FS_100 = R"(#version 100

precision mediump float;

varying vec3 fragPosition;
varying vec2 fragTexCoord;
varying vec4 fragColor;
varying vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

#define     MAX_LIGHTS              4
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

uniform Light lights[MAX_LIGHTS];
uniform vec4 ambient;
uniform vec3 viewPos;

void main()
{
    vec4 texelColor = texture2D(texture0, fragTexCoord);
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 specular = vec3(0.0);

    vec4 tint = colDiffuse*fragColor;

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 1)
        {
            vec3 light = vec3(0.0);

            if (lights[i].type == LIGHT_DIRECTIONAL)
            {
                light = -normalize(lights[i].target - lights[i].position);
            }

            if (lights[i].type == LIGHT_POINT)
            {
                light = normalize(lights[i].position - fragPosition);
            }

            float NdotL = max(dot(normal, light), 0.0);
            lightDot += lights[i].color.rgb*NdotL;

            float specCo = 0.0;
            if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), 16.0); // 16 refers to shine
            specular += specCo;
        }
    }

    vec3 lit = (texelColor.rgb*((tint.rgb + specular)*lightDot));
    lit += texelColor.rgb*(ambient.rgb/10.0);
    lit = pow(lit, vec3(1.0/2.2));

    // Alpha is the surface opacity, not a product of the lighting math.
    gl_FragColor = vec4(lit, texelColor.a*tint.a);
}
)";

static const char* LIT_VS_330 = R"(#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

out vec3 fragPosition;
out vec2 fragTexCoord;
out vec4 fragColor;
out vec3 fragNormal;

void main()
{
    fragPosition = vec3(matModel*vec4(vertexPosition, 1.0));
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

    mat3 normalMatrix = transpose(inverse(mat3(matModel)));
    fragNormal = normalize(normalMatrix*vertexNormal);

    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

static const char* LIT_FS_330 = R"(#version 330

in vec3 fragPosition;
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

#define     MAX_LIGHTS              4
#define     LIGHT_DIRECTIONAL       0
#define     LIGHT_POINT             1

struct Light {
    int enabled;
    int type;
    vec3 position;
    vec3 target;
    vec4 color;
};

uniform Light lights[MAX_LIGHTS];
uniform vec4 ambient;
uniform vec3 viewPos;

void main()
{
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec3 lightDot = vec3(0.0);
    vec3 normal = normalize(fragNormal);
    vec3 viewD = normalize(viewPos - fragPosition);
    vec3 specular = vec3(0.0);

    vec4 tint = colDiffuse*fragColor;

    for (int i = 0; i < MAX_LIGHTS; i++)
    {
        if (lights[i].enabled == 1)
        {
            vec3 light = vec3(0.0);

            if (lights[i].type == LIGHT_DIRECTIONAL)
            {
                light = -normalize(lights[i].target - lights[i].position);
            }

            if (lights[i].type == LIGHT_POINT)
            {
                light = normalize(lights[i].position - fragPosition);
            }

            float NdotL = max(dot(normal, light), 0.0);
            lightDot += lights[i].color.rgb*NdotL;

            float specCo = 0.0;
            if (NdotL > 0.0) specCo = pow(max(0.0, dot(viewD, reflect(-(light), normal))), 16.0); // 16 refers to shine
            specular += specCo;
        }
    }

    vec3 lit = (texelColor.rgb*((tint.rgb + specular)*lightDot));
    lit += texelColor.rgb*(ambient.rgb/10.0);
    lit = pow(lit, vec3(1.0/2.2));

    // Alpha is the surface opacity, not a product of the lighting math.
    finalColor = vec4(lit, texelColor.a*tint.a);
}
)";

#if defined(PLATFORM_WEB)
    #define LIT_VS LIT_VS_100
    #define LIT_FS LIT_FS_100
#else
    #define LIT_VS LIT_VS_330
    #define LIT_FS LIT_FS_330
#endif

Shader LoadLitShader() {
    return LoadShaderFromMemory(LIT_VS, LIT_FS);
}

Light CreateLight0(int type, Vector3 position, Vector3 target, Color color, Shader shader) {
    Light light = { 0 };
    light.enabled = true;
    light.type = type;
    light.position = position;
    light.target = target;
    light.color = color;

    light.enabledLoc = GetShaderLocation(shader, "lights[0].enabled");
    light.typeLoc = GetShaderLocation(shader, "lights[0].type");
    light.positionLoc = GetShaderLocation(shader, "lights[0].position");
    light.targetLoc = GetShaderLocation(shader, "lights[0].target");
    light.colorLoc = GetShaderLocation(shader, "lights[0].color");

    UpdateLightValues(shader, light);
    return light;
}
