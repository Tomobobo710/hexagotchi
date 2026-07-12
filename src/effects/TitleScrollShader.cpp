#include "TitleScrollShader.hpp"
#include "CharacterRegistry.hpp"
#include "GameConstants.hpp"
#include <string>
#include <cmath>

// TitleScrollShader - a slowly ROTATING, layered honeycomb of the 9 cast
// colors. Per-pixel fragment work that shapes can't do: parallax depth (two
// hex layers), per-hex radial glow that breathes, cast-color bleed toward
// neighbors, a highlight that flows along the seams, and a slow chromatic wave.
// Tuned for a premium ambient background -- alive and eye-catching, not busy.
//
// The two GLSL dialects (ES 100 for web, 330 for desktop) differ ONLY in the
// header boilerplate, so the actual logic lives in ONE shared body string
// (SHARED_FS_BODY) that both wrap via an OUTCOLOR macro. No two-copy drift.

// --- Vertex shaders (trivial passthrough) ------------------------------------
static const char* TITLE_SCROLL_VS_100 = R"(#version 100
attribute vec3 vertexPosition;
attribute vec2 vertexTexCoord;
uniform mat4 mvp;
varying vec2 fragTexCoord;
void main() {
    fragTexCoord = vertexTexCoord;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

static const char* TITLE_SCROLL_VS_330 = R"(#version 330
in vec3 vertexPosition;
in vec2 vertexTexCoord;
uniform mat4 mvp;
out vec2 fragTexCoord;
void main() {
    fragTexCoord = vertexTexCoord;
    gl_Position = mvp*vec4(vertexPosition, 1.0);
}
)";

// --- Fragment headers per dialect. OUTCOLOR macro lets the shared body write
//     to gl_FragColor (ES100) or a declared out (330). ------------------------
static const char* FS_HEAD_100 = R"(#version 100
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define OUTCOLOR gl_FragColor
varying vec2 fragTexCoord;
uniform float time;
uniform vec3 palette[9];
uniform float aspect;
)";

static const char* FS_HEAD_330 = R"(#version 330
#define OUTCOLOR finalColor
in vec2 fragTexCoord;
uniform float time;
uniform vec3 palette[9];
uniform float aspect;
out vec4 finalColor;
)";

// ============================================================================
// Two fragment bodies live here. ACTIVE_FS_BODY (bottom of this section) picks
// which one the title uses:
//   GRID_FS_BODY     -- the clean rotating hex-grid honeycomb (currently active)
//   HEXFIELD_FS_BODY -- a field of 24 individual tumbling 3D hexes (kept around)
// Both are dialect-agnostic; the FS_HEAD_* wrappers supply the version header.
// ============================================================================

// --- HEXFIELD: a field of INDIVIDUAL hexagons (not a tiling): 24 separate
// hexes, each with its own drifting position, size, spin, and a 3D tumble so it
// foreshortens edge-on like a coin flipping and catches a light. Kept for later
// (not currently active -- see ACTIVE_FS_BODY).
[[maybe_unused]] static const char* HEXFIELD_FS_BODY = R"(
#define NHEX 24

float hash11(float n) { return fract(sin(n * 43758.5453123) * 12345.6789); }

// One of the 9 cast colors by index 0..8.
vec3 castColorI(int idx) {
    vec3 c = palette[0];
    for (int i = 1; i < 9; i++) {
        if (i == idx) c = palette[i];
    }
    return c;
}

// Signed distance to a flat-top regular hexagon of "radius" r, centered at 0.
float sdHex(vec2 p, float r) {
    const vec3 k = vec3(-0.866025404, 0.5, 0.577350269);
    p = abs(p);
    p -= 2.0 * min(dot(k.xy, p), 0.0) * k.xy;
    p -= vec2(clamp(p.x, -k.z * r, k.z * r), r);
    return length(p) * sign(p.y);
}

// Accumulate one hex instance `i` into (rgb, coverage) with front-to-back
// alpha. Each hex has its own orbit, size, spin, 3D tumble and color.
void addHex(int i, vec2 uv, float t, inout vec4 acc) {
    float fi = float(i);
    float r1 = hash11(fi * 1.3 + 0.1);
    float r2 = hash11(fi * 2.7 + 3.4);
    float r3 = hash11(fi * 5.1 + 7.2);

    // Depth: a per-hex z in [0,1]; nearer hexes are bigger and brighter and
    // drawn later (on top). We sort implicitly by looping i in depth order via
    // the hash -- good enough visually.
    float depth = r3;                       // 0 far .. 1 near
    float scale = mix(0.05, 0.18, depth);   // near hexes larger

    // Slow drifting orbit -- each hex loops on its own lissajous path.
    float sp = mix(0.05, 0.16, r1);         // orbit speed
    vec2 center = vec2(
        sin(t * sp + fi * 2.1) * mix(0.30, 0.75, r2),
        cos(t * sp * 0.9 + fi * 1.3) * mix(0.30, 0.62, r1)
    );

    vec2 q = uv - center;

    // Spin: rotate the hex in-plane.
    float spin = t * mix(0.2, 0.9, r2) * (r1 > 0.5 ? 1.0 : -1.0);
    float cs = cos(spin), sn = sin(spin);
    q = vec2(q.x * cs - q.y * sn, q.x * sn + q.y * cs);

    // 3D TUMBLE: fake a rotation about a tilted axis by squashing one axis with
    // cos(angle) -- the hex foreshortens to a thin sliver edge-on, then opens
    // back up. This is what reads as "3D flipping".
    float tumble = t * mix(0.4, 1.3, r3) + fi;
    float fore = abs(cos(tumble));          // 1 flat-on .. 0 edge-on
    fore = 0.15 + 0.85 * fore;              // never fully vanish
    // squash along a per-hex axis
    float ax = fi * 1.7;
    float ca = cos(ax), sa = sin(ax);
    vec2 qa = vec2(q.x * ca - q.y * sa, q.x * sa + q.y * ca);
    qa.x /= fore;                           // foreshorten
    q = vec2(qa.x * ca + qa.y * sa, -qa.x * sa + qa.y * ca);

    // Hex SDF -> soft mask.
    float dsdf = sdHex(q, scale);
    float aa = 0.006;
    float mask = smoothstep(aa, -aa, dsdf); // 1 inside, soft edge
    if (mask <= 0.001) return;

    // Color by CYCLING the 9 palette indices (i mod 9), not random -- this
    // guarantees every cast color appears and they're evenly spread, instead of
    // random draws doubling some (two greens) and skipping others.
    int cidx = i - (i / 9) * 9;   // i mod 9 (integer, GLSL ES 100 safe)
    vec3 base = castColorI(cidx);

    // Lighting: a directional light + the SDF gradient gives the flat face a
    // gentle sheen; the foreshorten factor darkens edge-on faces (like a real
    // tumbling tile turning away from the light).
    float shade = 0.55 + 0.45 * fore;                    // edge-on = darker
    float rim = smoothstep(-0.02, 0.0, dsdf);            // bright rim at the edge
    vec3 col = base * shade + base * rim * 0.6;
    // Center sheen sweep.
    float sheen = 0.5 + 0.5 * sin(q.x * 24.0 + t * 1.5 + fi);
    col += base * 0.15 * sheen * mask;

    // Depth fog: far hexes slightly dimmer + toward background.
    col *= mix(0.55, 1.0, depth);

    // Front-to-back composite (premultiplied). acc.a is accumulated coverage.
    float a = mask * (1.0 - acc.a);
    acc.rgb += col * a;
    acc.a += a;
}

void main() {
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= aspect;
    float t = time;

    // Deep background: a very dark cast-tinted gradient so the hexes float on
    // something richer than flat black, with a soft central glow.
    float bgw = 0.5 + 0.5 * sin(uv.x * 1.1 + uv.y * 0.7 - t * 0.25);
    vec3 bg = mix(palette[6] * 0.05, palette[3] * 0.09, bgw);   // grey<->blue, very dark
    bg += palette[0] * 0.04 * (1.0 - dot(uv, uv));             // faint green center glow

    // Composite the hex field front-to-back. Loop is depth-agnostic; the per-hex
    // depth dim + coverage sorting keeps it readable.
    vec4 acc = vec4(0.0);
    for (int i = 0; i < NHEX; i++) {
        addHex(i, uv, t, acc);
    }

    vec3 col = bg * (1.0 - acc.a) + acc.rgb;

    // Gentle vignette to seat the centered UI.
    float vig = 1.0 - 0.35 * dot(uv, uv);
    col *= clamp(vig, 0.65, 1.0);

    OUTCOLOR = vec4(col, 1.0);
}
)";

// --- GRID: the clean rotating hex-grid honeycomb. Each hex is one cast color,
// with a breathing radial glow, edge color-bleed toward a neighbor, and a
// flowing seam highlight; a back layer of larger hexes gives parallax depth,
// plus a slow chromatic wave and vignette. Precision-safe hash (mod-wrapped) so
// there's no flicker banding on web. Currently the active title background.
static const char* GRID_FS_BODY = R"(
float hash(vec2 p) {
    p = mod(p, 256.0);
    return fract(sin(dot(p, vec2(41.3, 289.1))) * 43758.5453);
}

// Pointy-top axial hex grid. Returns (cell center .xy, edge-distance .z 0..1,
// stable per-cell hash .w) and writes the cell's INTEGER coords to qr.
//
// qr comes from the exact same floor() values that select the cell -- never
// recovered from the center afterward. (The old recovery version rounded
// differently on the GPU than the selection did, so whole rows of cells got
// the neighboring row's index -> same-color neighbor pairs.)
// A-lattice cells land on (odd, odd) qr, B-lattice on (even, even); qr.x is
// in half-column units, qr.y in half-row (sqrt(3)/2) units.
vec4 hexInfo(vec2 p, out vec2 qr) {
    vec2 s = vec2(1.0, 1.7320508);
    vec2 fa = floor(p / s);
    vec2 fb = floor(p / s - 0.5);
    vec2 ca = (fa + 0.5) * s;   // A candidate center
    vec2 cb = (fb + 1.0) * s;   // B candidate center
    vec2 hexA = p - ca;
    vec2 hexB = p - cb;
    // Tiny bias so the A/B tie line doesn't flicker per-pixel (it produced a
    // dithered noise streak where the two sub-lattices are exactly equidistant).
    bool aWins = dot(hexA, hexA) < dot(hexB, hexB) + 1.0e-4;
    vec2 hex  = aWins ? hexA : hexB;
    vec2 grid = aWins ? ca : cb;
    qr = aWins ? fa * 2.0 + 1.0 : fb * 2.0 + 2.0;
    vec3 hc = vec3(hex.x, hex.y*0.5 + hex.x*0.28867513, hex.y*0.5 - hex.x*0.28867513);
    float d = max(max(abs(hc.x), abs(hc.y)), abs(hc.z)) / 0.8660254;
    return vec4(grid, d, hash(qr));
}

// One of the 9 cast colors by integer index 0..8 (dynamic-index-free).
vec3 castColorI(int idx) {
    vec3 c = palette[0];
    for (int i = 1; i < 9; i++) {
        if (i == idx) c = palette[i];
    }
    return c;
}
// Backward-compat float-id picker (used for the neighbor-bleed sample).
vec3 castColor(float id) {
    return castColorI(int(floor(id * 9.0)));
}

// Pick a cast color for a hex from its integer qr coords (from hexInfo).
//
// Any pure formula (k linear in qr, mod 9) puts each color on a perfect
// diagonal -- reads as stripes/lines. Instead: a proper 3-COLORING of the hex
// lattice (class = qr.x mod 3; neighbor deltas in qr.x are +-1/+-2, so
// touching cells ALWAYS land in different classes), where each class owns a
// private bucket of 3 palette colors and the cell picks RANDOMLY within its
// bucket. Random-looking, yet two touching hexes can never share a color --
// and look-alike colors are grouped into the same bucket (see the palette
// cast order) so near-identical hues can't touch either.
vec3 castColorForCell(vec2 qr) {
    float cls = mod(qr.x, 3.0);
    if (cls < 0.0) cls += 3.0;
    float sub = min(floor(hash(qr) * 3.0), 2.0);
    return castColorI(int(cls * 3.0 + sub));
}

// One rotated, drifting hex layer, shaded with glow + neighbor bleed + flowing
// seam. `depth` dims the back layer so it reads further away.
// Returns color in .rgb and edge-distance in .a; writes the cell center (in
// layer grid space) to cellCenter so main() can run cell-quantized opacity
// waves over the front layer.
vec4 sampleLayer(vec2 uv, float sc, float rot, vec2 drift, float depth, out vec2 cellCenter, out vec2 layerP) {
    float cr = cos(rot), sr = sin(rot);
    vec2 rv = vec2(uv.x*cr - uv.y*sr, uv.x*sr + uv.y*cr);
    vec2 p = rv * sc + drift;
    layerP = p;

    vec2 qr;
    vec4 info = hexInfo(p, qr);
    float d = info.z;
    float id = info.w;
    vec3 base = castColorForCell(qr);   // even, non-clumping cast coloring

    // Brightness kept nearly FLAT: heavy per-cell dimming collapsed distinct
    // palette colors into each other (white dimmed 40% == grey, yellow ==
    // orange), which read as same-color neighbor pairs. Subtle glow only.
    float pulse = sin(time * 0.9 + id * 6.2831853) * 0.5 + 0.5;
    float glow = (1.0 - smoothstep(0.0, 1.0, d)) * (0.35 + 0.35 * pulse);
    vec3 col = base * (0.92 + 0.16 * glow);

    vec3 nb = castColor(fract(id + 0.37));
    col = mix(col, mix(col, nb, 0.15), smoothstep(0.55, 1.0, d));

    float edge = smoothstep(0.86, 0.98, d);
    float flow = sin(id * 20.0 + time * 1.6) * 0.5 + 0.5;
    vec3 seam = base * (0.30 + 0.9 * flow);
    col = mix(col, seam, edge * 0.8);

    col = mix(col, col * 0.60, depth);
    cellCenter = info.xy;
    return vec4(col, d);
}

void main() {
    vec2 uv = fragTexCoord - 0.5;
    uv.x *= aspect;

    float t = time;
    // ONE grid: small crisp hexes, cast colors, provably no same-color
    // neighbors (see castColorForCell).
    vec2 fc, fp;
    vec4 front = sampleLayer(uv, 9.0, t * 0.04, vec2(t*0.12, t*-0.08), 0.0, fc, fp);

    // Gentle brightness RIPPLES sweeping across the field. Spatially smooth
    // (built from the continuous layer position fp, never per-cell values --
    // cell-quantized modulation dithered into noise bands at cell boundaries).
    float w1 = sin(dot(fp, vec2(0.80, 0.45)) * 1.5 - t * 2.6);
    float w2 = sin(dot(fp, vec2(-0.35, 0.90)) * 1.9 + t * 1.9);
    float ripple = w1 * 0.5 + w2 * 0.5;                    // -1..1
    vec3 col = front.rgb * (1.0 + 0.07 * ripple);

    // Wave + vignette kept SHALLOW for the same reason as the flat cell glow:
    // deep luminance swings make distinct palette colors read as each other.
    float wave = sin(uv.x * 1.2 + uv.y * 0.8 - t * 0.4) * 0.5 + 0.5;
    col *= 0.96 + 0.06 * wave;

    float vig = 1.0 - 0.18 * dot(uv, uv);
    col *= clamp(vig, 0.8, 1.0);

    OUTCOLOR = vec4(col, 1.0);
}
)";

// Active title body: GRID (clean rotating honeycomb). Swap to HEXFIELD_FS_BODY
// to use the tumbling-hexes version instead.
#define ACTIVE_FS_BODY GRID_FS_BODY

// Stitch header + shared body per dialect (built once at first use).
static const std::string TITLE_SCROLL_FS_100_STR = std::string(FS_HEAD_100) + ACTIVE_FS_BODY;
static const std::string TITLE_SCROLL_FS_330_STR = std::string(FS_HEAD_330) + ACTIVE_FS_BODY;

#if defined(PLATFORM_WEB)
    #define TITLE_SCROLL_VS TITLE_SCROLL_VS_100
    #define TITLE_SCROLL_FS TITLE_SCROLL_FS_100_STR.c_str()
#else
    #define TITLE_SCROLL_VS TITLE_SCROLL_VS_330
    #define TITLE_SCROLL_FS TITLE_SCROLL_FS_330_STR.c_str()
#endif

// Global shader loader for direct use if needed
Shader LoadTitleScrollShader() {
    return LoadShaderFromMemory(TITLE_SCROLL_VS, TITLE_SCROLL_FS);
}

// TitleScrollShader class implementation

TitleScrollShader::TitleScrollShader()
    : time_(0.0f)
{
    // The palette IS the cast: each hex gets one of the 9 characters' identity
    // (name) colors, pulled straight from CharacterRegistry so this stays in
    // sync if a character's color ever changes.
    //
    // ORDER MATTERS: the shader's 3-coloring gives each lattice class a
    // private bucket of 3 consecutive palette slots (0-2, 3-5, 6-8), and
    // touching hexes are always in different classes/buckets. So look-alike
    // colors are grouped INTO THE SAME BUCKET to guarantee they never touch:
    // white/grey (+olive), the warm yellow/orange/red family, and the
    // pink/purple/blue family.
    const CharacterId cast[PALETTE_SIZE] = {
        CharacterId::Judy,  CharacterId::Larry,   CharacterId::Tom,     // white | grey | olive
        CharacterId::Karen, CharacterId::Loraine, CharacterId::Ronzer,  // yellow | orange | red
        CharacterId::Mark,  CharacterId::Bimmy,   CharacterId::Jimmy,   // pink | purple | blue
    };
    for (int i = 0; i < PALETTE_SIZE; i++) {
        palette_[i] = CharacterRegistry::get(cast[i]).nameColor;
    }
}

void TitleScrollShader::init() {
    shader_ = LoadShaderFromMemory(TITLE_SCROLL_VS, TITLE_SCROLL_FS);
    if (shader_.id != 0) {
        timeLoc_ = GetShaderLocation(shader_, "time");
        aspectLoc_ = GetShaderLocation(shader_, "aspect");
        paletteLoc_ = GetShaderLocation(shader_, "palette");

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

        // Aspect is FIXED at the game's render-target ratio (720x720 = 1.0), NOT
        // the live window size. The shader draws into the letterboxed 720x720
        // target, so using GetScreenWidth()/Height() here made the hexes scale
        // and stretch with the OS window.
        float aspect = (float)GAME_W / (float)GAME_H;
        SetShaderValue(shader_, aspectLoc_, &aspect, SHADER_UNIFORM_FLOAT);

        // Upload the 9-color cast palette as a flat vec3 array.
        float pal[PALETTE_SIZE * 3];
        for (int i = 0; i < PALETTE_SIZE; i++) {
            pal[i * 3 + 0] = palette_[i].r / 255.0f;
            pal[i * 3 + 1] = palette_[i].g / 255.0f;
            pal[i * 3 + 2] = palette_[i].b / 255.0f;
        }
        SetShaderValueV(shader_, paletteLoc_, pal, SHADER_UNIFORM_VEC3, PALETTE_SIZE);
    }
}

void TitleScrollShader::drawBackground() {
    if (!loaded_) return;

    BeginShaderMode(shader_);

    // Cover the fixed 720x720 render target (NOT the OS window). This effect is
    // drawn inside the game's letterboxed target, which main.cpp then scales to
    // the window -- so the quad and the shader's UVs must be target-sized, or the
    // hexes stretch/scale as the window resizes.
    Rectangle screenRect = {0, 0, (float)GAME_W, (float)GAME_H};
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
