#include "GotchiScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "GotchiStats.hpp"
#include "GotchiMood.hpp"
#include "SceneManager.hpp"
#include "EventBus.h"
#include "MergeController.hpp"
#include "ExploreGate.h"
#include "EventType.h"
#include <iostream>
#include <sstream>
#include <algorithm>

// ============================================================================
// Action Overlay Fragment Shaders
// ============================================================================
// Two versions for desktop (OpenGL 3.3) and web (WebGL 2.0)
// ============================================================================

// ============================================================================
// Action Overlay Fragment Shaders - Revised for visible effects
// ============================================================================
// Two versions for desktop (OpenGL 3.3) and web (WebGL 2.0)
// Fixed: No texture0 sampling, mask-based compositing with real UVs
// ============================================================================

// ============================================================================
// Action codes for CareAction events
// ============================================================================
// These match the shader modes and button order in GotchiScene::addButtons()
// Order: Wash=0, Groom=1, Feed=2, Pet=3, Water=4, Merge=5
// Warmth actions (for affection): Pet only (play/ball not yet implemented as button)
// Hygiene actions (for mercy): Wash, Groom (not Water or Feed)
static bool isWarmthAction(int actionCode) {
    // Warmth = pet/play only
    // Current implementation: only Pet (code 3) is a warmth action
    return actionCode == 3;  // Pet
}

static bool isHygieneAction(int actionCode) {
    // Hygiene = wash/groom
    return actionCode == 0 || actionCode == 1;  // Wash or Groom
}

// Gotchi pixel scale - now in GameConstants.hpp for shared use across scenes

// ============================================================================
// Action Overlay Fragment Shaders  —  drop-in replacement
// ============================================================================
// Replaces ACTION_OVERLAY_FS_DESKTOP and ACTION_OVERLAY_FS_WEB in GotchiScene.cpp.
//
// Same uniform interface as before (uMode / uProgress / uTime / uResolution),
// so NO C++ changes are needed — this is a pure swap of the two string literals.
//
// NOTE ON VERSIONS: I kept your existing pairing here — desktop = #version 330,
// web = #version 100 (varying / gl_FragColor). This matches what's currently in
// your file. Your other projects use "#version 300 es" for web; if hexagotchi's
// raylib web target is actually GLES3, tell me and I'll re-emit the web copy as
// 300 es (in/out + `out vec4 finalColor`). Left as-is to avoid breaking the build.
//
// The shared body uses only GLSL-ES-1.00-safe constructs: constant-bound loops,
// no reversed-edge smoothstep, no dynamic array indexing, no dFdx/textureLod.
// ============================================================================

// Desktop shader - #version 330
static const char* ACTION_OVERLAY_FS_DESKTOP = R"(#version 330
    in vec2 fragTexCoord;
    in vec4 fragColor;
    out vec4 finalColor;

    uniform int   uMode;       // 0=Wash, 1=Groom, 2=Feed, 3=Pet, 4=Water
    uniform float uProgress;   // 0..1 over effect duration
    uniform float uTime;       // seconds
    uniform vec2  uResolution; // dest rect size (for aspect-correct UVs)

    float hash21(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
    float hash11(float n){ return fract(sin(n * 17.13) * 43758.5453); }

    // 1.0 at x==c, falls off within +/- w
    float band(float x, float c, float w){ return 1.0 - smoothstep(0.0, w, abs(x - c)); }

    // 1.0 inside radius r, soft edge of width `soft`
    float blob(vec2 p, float r, float soft){ return 1.0 - smoothstep(r - soft, r, length(p)); }

    // 4-point twinkle glint centered at origin, arm length ~ s
    float sparkle(vec2 p, float s){
        p /= s;
        float ax = abs(p.x), ay = abs(p.y);
        float h = (1.0 - smoothstep(0.0, 1.0, ax)) * (1.0 - smoothstep(0.0, 0.14, ay));
        float v = (1.0 - smoothstep(0.0, 1.0, ay)) * (1.0 - smoothstep(0.0, 0.14, ax));
        float core = 1.0 - smoothstep(0.0, 0.28, length(p));
        return clamp(h + v + core, 0.0, 1.0);
    }

    // Classic heart curve: (x^2 + y^2 - 1)^3 - x^2 y^3.  < 0 inside, point down.
    float heartField(vec2 p){
        float x = p.x, y = p.y;
        float a = x*x + y*y - 1.0;
        return a*a*a - x*x*y*y*y;
    }

    // One column-stream of falling food pellets (used twice for a layered look).
    float feedStream(vec2 sv, float t, float N, float R, float phase,
                    float spdScale, float rad, float soft, float thr){
        float colId = floor(sv.x * N);
        float lane  = (fract(sv.x * N) - 0.5) / N;          // suv-space x offset
        float spd   = (0.5 + hash11(colId) * 0.7) * spdScale;
        float yy    = fract(sv.y * R + t * spd + hash11(colId + phase));
        float dy    = (yy - 0.5) / R;
        float pellet = 1.0 - smoothstep(rad - soft, rad, sqrt(lane*lane + dy*dy));
        float on    = step(thr, hash11(colId + 2.0 + phase));
        return pellet * on;
    }

    void main() {
        vec2  uv = fragTexCoord; uv.y = 1.0 - uv.y;          // Y up
        float aspect = uResolution.x / max(uResolution.y, 1.0);
        vec2  suv = uv;    suv.x *= aspect;                  // aspect-corrected, for fields
        vec2  q   = uv - 0.5; q.x *= aspect;                 // centered & isotropic, for radial

        float p    = clamp(uProgress, 0.0, 1.0);
        float ease = sin(p * 3.14159265);                    // 0 -> 1 -> 0

        vec3  col  = vec3(0.0);
        float mask = 0.0;

        if (uMode == 0) {                    // WASH — drifting soap suds + sheen
            float m = 0.0;
            for (int i = 0; i < 3; i++){
                float fi  = float(i);
                float scl = 6.0 + fi * 3.0;
                vec2  gp  = suv * scl;
                gp.y     -= uTime * (0.6 + fi * 0.25);       // bubbles rise
                vec2  cell = floor(gp);
                vec2  f    = fract(gp) - 0.5;
                float rnd  = hash21(cell + fi * 31.0);
                float r    = 0.16 + 0.16 * rnd;
                float d    = length(f);
                float body = (1.0 - smoothstep(r - 0.05, r, d)) * 0.45;
                float rim  = band(d, r - 0.03, 0.03) * 0.6;
                m = max(m, body + rim);
            }
            float sweep = band(uv.x, p * 1.2 - 0.1, 0.10) * 0.5;
            m    = max(m, sweep);
            col  = mix(vec3(0.72, 0.88, 1.0), vec3(1.0), m);
            mask = clamp(m, 0.0, 1.0) * 0.8;

        } else if (uMode == 1) {             // GROOM — twinkling glints + shine wipe
            float m   = 0.0;
            vec2  gp  = suv * 5.0;
            vec2  cell = floor(gp);
            vec2  f    = fract(gp) - 0.5;
            float rnd  = hash21(cell);
            float on   = step(0.6, rnd);
            float ph   = hash21(cell + 7.0) * 6.2831;
            float tw   = sin(uTime * 3.0 + ph) * 0.5 + 0.5;
            float sz   = 0.22 + 0.14 * hash21(cell + 3.0);
            m += sparkle(f, sz) * tw * on;
            float diag  = (uv.x + uv.y) * 0.5;
            float shine = band(diag, p * 1.4 - 0.2, 0.06) * 0.6;
            m += shine;
            col  = vec3(1.0, 0.95, 0.62);
            mask = clamp(m, 0.0, 1.0) * 0.85;

        } else if (uMode == 2) {             // FEED — layered falling pellets + confined warm glow
            float m = 0.0;
            // two layered pellet streams: main + a smaller/faster one for depth
            m = max(m, feedStream(suv, uTime, 13.0, 4.5,  5.0, 1.0, 0.020, 0.012, 0.22));
            m = max(m, feedStream(suv, uTime, 16.0, 6.5, 19.0, 1.4, 0.015, 0.010, 0.37));
            float crumb = m;
            // confined warm glow at bottom-center — NOT a full-width fill
            float glow = (1.0 - smoothstep(0.0, 0.30, length(q - vec2(0.0, -0.34)))) * 0.18;
            float warm = max(crumb, glow * 0.6);   // glow leans warm/cream, not muddy brown
            m += glow;
            col  = mix(vec3(0.85, 0.55, 0.28), vec3(1.0, 0.85, 0.5), warm);
            mask = clamp(m, 0.0, 1.0) * 0.85;

        } else if (uMode == 3) {             // PET — rising hearts + warm pulse
            float m  = 0.0;
            float pr = fract(uTime * 0.4);
            m += band(length(q), pr * 0.55, 0.04) * (1.0 - pr) * 0.4;   // radial pulse
            for (int i = 0; i < 5; i++){
                float fi   = float(i);
                float seed = hash11(fi + 1.0);
                float t    = fract(uTime * 0.32 + seed);
                float hx   = (seed - 0.5) * 0.45 + sin((t + seed) * 6.2831) * 0.05;
                float hy   = -0.32 + t * 0.62;
                float scl  = 5.0 + seed * 2.5;
                vec2  hp   = (q - vec2(hx, hy)) * scl;
                float hf   = heartField(hp);
                float heart = 1.0 - smoothstep(-0.05, 0.06, hf);        // 1 inside
                float fade  = smoothstep(0.0, 0.15, t) * (1.0 - smoothstep(0.7, 1.0, t));
                m = max(m, heart * fade);
            }
            col  = vec3(1.0, 0.55, 0.62);
            mask = clamp(m, 0.0, 1.0) * 0.85;

        } else if (uMode == 4) {             // WATER — droplets w/ heads + base ripple
            float m     = 0.0;
            float N     = 8.0;
            float colId = floor(suv.x * N);
            float lane  = fract(suv.x * N) - 0.5;
            float spd   = 0.6 + hash11(colId) * 0.7;
            float yy    = fract(suv.y + uTime * spd * 0.55 + hash11(colId + 4.0));
            float on    = step(0.35, hash11(colId + 1.0));
            float head  = blob(vec2(lane, (yy - 0.5) * 1.5), 0.12, 0.10);
            float tail  = band(lane, 0.0, 0.05)
                        * (1.0 - smoothstep(0.5, 0.72, yy)) * smoothstep(0.5, 0.55, yy);
            m += (head + tail * 0.5) * on;
            float rp = fract(uTime * 0.7);
            m += band(length(q - vec2(0.0, -0.4)), rp * 0.4, 0.025) * (1.0 - rp)
            * (1.0 - smoothstep(0.0, 0.35, uv.y)) * 0.5;
            col  = vec3(0.45, 0.72, 1.0);
            mask = clamp(m, 0.0, 1.0) * 0.8;
        }

        finalColor = vec4(col, mask * ease * 0.85);
    }
)";

// Web shader - #version 100  (identical body to desktop; only header + output differ)
static const char* ACTION_OVERLAY_FS_WEB = R"(#version 100
    precision mediump float;

    varying vec2 fragTexCoord;
    varying vec4 fragColor;

    uniform int   uMode;
    uniform float uProgress;
    uniform float uTime;
    uniform vec2  uResolution;

    float hash21(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
    float hash11(float n){ return fract(sin(n * 17.13) * 43758.5453); }
    float band(float x, float c, float w){ return 1.0 - smoothstep(0.0, w, abs(x - c)); }
    float blob(vec2 p, float r, float soft){ return 1.0 - smoothstep(r - soft, r, length(p)); }

    float sparkle(vec2 p, float s){
        p /= s;
        float ax = abs(p.x), ay = abs(p.y);
        float h = (1.0 - smoothstep(0.0, 1.0, ax)) * (1.0 - smoothstep(0.0, 0.14, ay));
        float v = (1.0 - smoothstep(0.0, 1.0, ay)) * (1.0 - smoothstep(0.0, 0.14, ax));
        float core = 1.0 - smoothstep(0.0, 0.28, length(p));
        return clamp(h + v + core, 0.0, 1.0);
    }

    float heartField(vec2 p){
        float x = p.x, y = p.y;
        float a = x*x + y*y - 1.0;
        return a*a*a - x*x*y*y*y;
    }

    // One column-stream of falling food pellets (used twice for a layered look).
    float feedStream(vec2 sv, float t, float N, float R, float phase,
                    float spdScale, float rad, float soft, float thr){
        float colId = floor(sv.x * N);
        float lane  = (fract(sv.x * N) - 0.5) / N;          // suv-space x offset
        float spd   = (0.5 + hash11(colId) * 0.7) * spdScale;
        float yy    = fract(sv.y * R + t * spd + hash11(colId + phase));
        float dy    = (yy - 0.5) / R;
        float pellet = 1.0 - smoothstep(rad - soft, rad, sqrt(lane*lane + dy*dy));
        float on    = step(thr, hash11(colId + 2.0 + phase));
        return pellet * on;
    }

    void main() {
        vec2  uv = fragTexCoord; uv.y = 1.0 - uv.y;
        float aspect = uResolution.x / max(uResolution.y, 1.0);
        vec2  suv = uv;    suv.x *= aspect;
        vec2  q   = uv - 0.5; q.x *= aspect;

        float p    = clamp(uProgress, 0.0, 1.0);
        float ease = sin(p * 3.14159265);

        vec3  col  = vec3(0.0);
        float mask = 0.0;

        if (uMode == 0) {                    // WASH
            float m = 0.0;
            for (int i = 0; i < 3; i++){
                float fi  = float(i);
                float scl = 6.0 + fi * 3.0;
                vec2  gp  = suv * scl;
                gp.y     -= uTime * (0.6 + fi * 0.25);
                vec2  cell = floor(gp);
                vec2  f    = fract(gp) - 0.5;
                float rnd  = hash21(cell + fi * 31.0);
                float r    = 0.16 + 0.16 * rnd;
                float d    = length(f);
                float body = (1.0 - smoothstep(r - 0.05, r, d)) * 0.45;
                float rim  = band(d, r - 0.03, 0.03) * 0.6;
                m = max(m, body + rim);
            }
            float sweep = band(uv.x, p * 1.2 - 0.1, 0.10) * 0.5;
            m    = max(m, sweep);
            col  = mix(vec3(0.72, 0.88, 1.0), vec3(1.0), m);
            mask = clamp(m, 0.0, 1.0) * 0.8;

        } else if (uMode == 1) {             // GROOM
            float m   = 0.0;
            vec2  gp  = suv * 5.0;
            vec2  cell = floor(gp);
            vec2  f    = fract(gp) - 0.5;
            float rnd  = hash21(cell);
            float on   = step(0.6, rnd);
            float ph   = hash21(cell + 7.0) * 6.2831;
            float tw   = sin(uTime * 3.0 + ph) * 0.5 + 0.5;
            float sz   = 0.22 + 0.14 * hash21(cell + 3.0);
            m += sparkle(f, sz) * tw * on;
            float diag  = (uv.x + uv.y) * 0.5;
            float shine = band(diag, p * 1.4 - 0.2, 0.06) * 0.6;
            m += shine;
            col  = vec3(1.0, 0.95, 0.62);
            mask = clamp(m, 0.0, 1.0) * 0.85;

        } else if (uMode == 2) {             // FEED — layered falling pellets + confined warm glow
            float m = 0.0;
            // two layered pellet streams: main + a smaller/faster one for depth
            m = max(m, feedStream(suv, uTime, 13.0, 4.5,  5.0, 1.0, 0.020, 0.012, 0.22));
            m = max(m, feedStream(suv, uTime, 16.0, 6.5, 19.0, 1.4, 0.015, 0.010, 0.37));
            float crumb = m;
            // confined warm glow at bottom-center — NOT a full-width fill
            float glow = (1.0 - smoothstep(0.0, 0.30, length(q - vec2(0.0, -0.34)))) * 0.18;
            float warm = max(crumb, glow * 0.6);   // glow leans warm/cream, not muddy brown
            m += glow;
            col  = mix(vec3(0.85, 0.55, 0.28), vec3(1.0, 0.85, 0.5), warm);
            mask = clamp(m, 0.0, 1.0) * 0.85;

        } else if (uMode == 3) {             // PET
            float m  = 0.0;
            float pr = fract(uTime * 0.4);
            m += band(length(q), pr * 0.55, 0.04) * (1.0 - pr) * 0.4;
            for (int i = 0; i < 5; i++){
                float fi   = float(i);
                float seed = hash11(fi + 1.0);
                float t    = fract(uTime * 0.32 + seed);
                float hx   = (seed - 0.5) * 0.45 + sin((t + seed) * 6.2831) * 0.05;
                float hy   = -0.32 + t * 0.62;
                float scl  = 5.0 + seed * 2.5;
                vec2  hp   = (q - vec2(hx, hy)) * scl;
                float hf   = heartField(hp);
                float heart = 1.0 - smoothstep(-0.05, 0.06, hf);
                float fade  = smoothstep(0.0, 0.15, t) * (1.0 - smoothstep(0.7, 1.0, t));
                m = max(m, heart * fade);
            }
            col  = vec3(1.0, 0.55, 0.62);
            mask = clamp(m, 0.0, 1.0) * 0.85;

        } else if (uMode == 4) {             // WATER
            float m     = 0.0;
            float N     = 8.0;
            float colId = floor(suv.x * N);
            float lane  = fract(suv.x * N) - 0.5;
            float spd   = 0.6 + hash11(colId) * 0.7;
            float yy    = fract(suv.y + uTime * spd * 0.55 + hash11(colId + 4.0));
            float on    = step(0.35, hash11(colId + 1.0));
            float head  = blob(vec2(lane, (yy - 0.5) * 1.5), 0.12, 0.10);
            float tail  = band(lane, 0.0, 0.05)
                        * (1.0 - smoothstep(0.5, 0.72, yy)) * smoothstep(0.5, 0.55, yy);
            m += (head + tail * 0.5) * on;
            float rp = fract(uTime * 0.7);
            m += band(length(q - vec2(0.0, -0.4)), rp * 0.4, 0.025) * (1.0 - rp)
            * (1.0 - smoothstep(0.0, 0.35, uv.y)) * 0.5;
            col  = vec3(0.45, 0.72, 1.0);
            mask = clamp(m, 0.0, 1.0) * 0.8;
        }

        gl_FragColor = vec4(col, mask * ease * 0.85);
    }
)";


GotchiScene::GotchiScene()
    : Scene(720.0f, 720.0f, {40, 40, 60, 255}) {
    gotchiDir = "gotchis/001";
}

void GotchiScene::init() {
    // Center camera
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);

    // Load biome backgrounds (will set current background based on gotchi's hex position)
    loadBiomeBackgrounds();

    // Create Gotchi with shared vitals and mood from GameState
    // If gameState_ is not set (e.g., in tests), use fallback defaults
    GotchiStats& stats = gameState_ ? gameState_->vitals : defaultStats_;
    GotchiMood& mood = gameState_ ? gameState_->mood : defaultMood_;

    // This scene is always a fixed close-up shot of the gotchi centered on
    // its own 720x720 screen -- gotchiHexQ/gotchiHexR is HexViewScene's
    // hexboard-world tile coordinate (can be far from the origin), not a
    // position on this screen. Reusing it here made the gotchi drift toward
    // whatever tile it last stood on out on the hexboard instead of staying
    // centered.
    Vector2 gotchiPos = {360.0f, 360.0f};
    gotchi = new Gotchi(gotchiPos, stats, mood, gameState_);
    gotchi->setTag("gotchi");
    addActor(gotchi);

    // Initialize the Gotchi (no longer resets vitals - they persist in GameState)
    gotchi->init();

    // Load animation frames using the Gotchi's loader
    gotchi->loadAnimationFrames(gotchiDir);

    // Set initial action AFTER loading frames - this now properly starts idle animation
    gotchi->setAction("idle");

    // Size the gotchi based on actual frame dimensions (native 64px)
    Vector2 frameSize = gotchi->getFrameSize();
    if (frameSize.x > 0 && frameSize.y > 0) {
        gotchi->setSize(frameSize.x, frameSize.y);  // Base size = native frame pixels
    }

    // Framing: set fixed world scale and compute camera zoom to fill ~60% of screen
    gotchi->setScale({ GOTCHI_WORLD_SCALE, GOTCHI_WORLD_SCALE });
    float spriteWorldPx = gotchi->getHeight() * GOTCHI_WORLD_SCALE;      // 64 * 2 = 128px
    float targetPx      = GOTCHI_SCREEN_FRAC * (float)GAME_H;            // 0.60 * 720 = 432px
    float framingZoom   = targetPx / spriteWorldPx;                       // 3.375
    getCamera()->followActor(gotchi);                                     // camera follows the gotchi
    getCamera()->setZoom(framingZoom);

    TraceLog(LOG_INFO, "GOTCHI_FRAME sprPx=%.0f targetPx=%.0f wantZoom=%.3f setZoom=%.3f minZoom=%.3f",
             spriteWorldPx, targetPx, framingZoom,
             getCamera()->getZoom(), getCamera()->minZoomForBounds());

    TraceLog(LOG_INFO, "GOTCHI_INIT idleFrames=%zu animating=%d scale=(%.1f,%.1f) size=(%.0f,%.0f)",
             gotchi->animIdleCount(), (int)gotchi->isAnimating(),
             gotchi->getScale().x, gotchi->getScale().y,
             gotchi->getWidth(), gotchi->getHeight());

    // Initialize action shader with platform-specific version
#if defined(PLATFORM_WEB)
    const char* fs = ACTION_OVERLAY_FS_WEB;
#else
    const char* fs = ACTION_OVERLAY_FS_DESKTOP;
#endif
    actionShader_ = LoadShaderFromMemory(nullptr, fs);
    if (actionShader_.id != 0) {
        locMode_ = GetShaderLocation(actionShader_, "uMode");
        locProgress_ = GetShaderLocation(actionShader_, "uProgress");
        locTime_ = GetShaderLocation(actionShader_, "uTime");
        locResolution_ = GetShaderLocation(actionShader_, "uResolution");

        // Create 1x1 white texture for DrawTexturePro (provides 0..1 UVs)
        Image img = GenImageColor(1, 1, WHITE);
        whitePixel_ = LoadTextureFromImage(img);
        UnloadImage(img);
    }

    // Add navigation buttons
    addButtons();

    // Start the new-game tutorial the first time we ever land on this scene.
    // shouldRun() reads GameState's "tutorial_seen" flag, so this is a no-op
    // on every entry after the tutorial has completed once.
    if (tutorialController_ && !tutorialController_->isActive() && tutorialController_->shouldRun()) {
        tutorialController_->start();
    }

    // Subscribe to CareAction events from ButtonPressed handlers
    // (the event is emitted in handleGotchiAction)
    if (eventBus_) {
        careActionToken_ = eventBus_->subscribe(EventType::CareAction, [this](const Event& e) {
            // Track warmth and hygiene actions for Box C drivers
            // actionCode: 0=Wash, 1=Groom, 2=Feed, 3=Pet, 4=Water
            int actionCode = e.ia;
            float magnitude = e.fa;

            // Warmth actions (pet only) drive affection
            if (isWarmthAction(actionCode)) {
                affectionAccumulator_ += magnitude * magnitude;  // Squared for diminishing returns
            }

            // Hygiene actions (wash/groom) drive mercy
            if (isHygieneAction(actionCode)) {
                hygieneAccumulator_ += magnitude * magnitude;
            }
        });
    }
}

void GotchiScene::addButtons() {
    buttons.clear();
    mergeButton_ = nullptr;  // buttons.clear() just destroyed whatever this pointed to
    lastClickedButton_ = "";
    buttonCooldowns_.clear();
    buttonFeedbackTimer_ = 0.0f;

    // Eight buttons, 2 rows of 4, 2x the old size:
    // Row 1: Explore, Wash, Groom, Feed
    // Row 2: Pet, Water, Sleep (inert placeholder), Merge
    float buttonWidth = 160.0f;
    float buttonHeight = 64.0f;
    float gap = 16.0f;
    float totalWidth = 4 * buttonWidth + 3 * gap;
    float startX = (GAME_W - totalWidth) / 2.0f;
    float row1Y = GAME_H - 160.0f;
    float row2Y = GAME_H - 80.0f;

    std::string mergeLabel = mergeController_ ? mergeController_->mergeButtonLabel() : "Merge";

    std::vector<std::string> row1 = {"EXPLORE", "Wash", "Groom", "Feed"};
    std::vector<std::string> row2 = {"Pet", "Water", "Sleep", mergeLabel};

    for (size_t i = 0; i < row1.size(); i++) {
        float x = startX + i * (buttonWidth + gap);
        addButton(row1[i], x, row1Y, false);
    }
    for (size_t i = 0; i < row2.size(); i++) {
        float x = startX + i * (buttonWidth + gap);
        // The last button in row 2 (index 3) is the merge button
        addButton(row2[i], x, row2Y, (i == 3));
    }

}

void GotchiScene::addButton(const std::string& label, float x, float y, bool isMergeButton) {
    float buttonWidth = 160.0f;
    float buttonHeight = 64.0f;
    Button* btn = new Button({x, y}, buttonWidth, buttonHeight, label);
    btn->setAnchor("top-left");
    btn->setFontSize(20);
    btn->setBackgroundColor({60, 60, 100, 220});
    btn->setHoverColor({100, 100, 160, 240});
    btn->setBorderColor({150, 150, 200, 255});

    if (label == "Sleep") {
        btn->setTooltip("Use this to sleep in HexLand!");
    }

    if (isMergeButton) {
        // Merge button emits MergeRequested on the event bus
        btn->setOnClick([this]() {
            onMergeButtonClicked();
        });
        mergeButton_ = btn;
    } else if (label == "EXPLORE") {
        btn->setOnClick([this]() {
            onExploreButtonClicked();
        });
    } else {
        // Other buttons trigger gotchi actions
        btn->setOnClick([this, label]() {
            handleGotchiAction(label);
        });
    }
    buttons.push_back(std::unique_ptr<Button>(btn));
}

// The merge button callback - emits MergeRequested on the bus
// This is the "Merge" button
void GotchiScene::onMergeButtonClicked() {
    // Emit MergeRequested event
    // The MergeController will decide whether to honor this request based on game state
    if (eventBus_) {
        eventBus_->emit(Event::mergeRequested());
    }
}

void GotchiScene::update(float deltaTime) {
    Scene::update(deltaTime);

    // Detect the sleep-hits-0 collapse BEFORE freezing/updating the gotchi
    // this frame, so the freeze takes effect the same frame sleep bottoms out.
    applySleepCollapseGate();

    // GotchiSim sets state_.collapsed when a vital need bottoms out (only
    // once deathEnabled). That's our one death condition -- route it into
    // Gotchi::isDead() so the existing DeathScene trigger below fires.
    if (gotchi && gameState_ && gameState_->collapsed && !gotchi->isDead()) {
        gotchi->setDead(true);
    }

    // Freeze vitals while the tutorial is actively teaching, or once the
    // gotchi has collapsed from exhaustion. Written directly to GameState
    // (which is what GotchiSim actually reads) rather than through
    // Gotchi::setStatsFrozen() -- that forwards via Gotchi::gameState_,
    // which nothing ever wires up (no setGameState() call site exists), so
    // it was silently a no-op and GotchiSim kept ticking during the tutorial.
    if (gotchi) {
        bool tutorialFreeze = tutorialController_ && tutorialController_->isActive();
        bool collapseFreeze = gameState_ && gameState_->sleepCollapsed;
        if (gameState_) {
            gameState_->statsFrozen = tutorialFreeze || collapseFreeze;
            gameState_->onHexboard = false;
        }
        gotchi->update(deltaTime);

        // Sync gotchi's current hex position to GameState
        if (gameState_) {
            HexCoords currentHex = gotchi->getCurrentHex();
            gameState_->gotchiHexQ = currentHex.q;
            gameState_->gotchiHexR = currentHex.r;
        }
    }

    // Background update based on GameState biome
    if (needsBackgroundUpdate_ && gameState_) {
        updateBackgroundForHex(gameState_->gotchiHexQ, gameState_->gotchiHexR);
        needsBackgroundUpdate_ = false;
    }

    // Death is the ONLY state that auto-transitions on its own -- sleep
    // maxing out only locks buttons and holds "wobble", waiting on the
    // player to press Merge. On death: play the death animation and hold it
    // on screen for DEATH_HOLD_SECONDS. The merge -> DeathScene handoff
    // after that is driven by DeathSequencer (see main.cpp), which keeps
    // running across the scene switch this scene's own update() can't
    // survive -- see its header for why that has to live outside any one
    // scene's update().
    if (gotchi->isDead() && deathHoldTimer_ < 0.0f) {
        deathHoldTimer_ = DEATH_HOLD_SECONDS;
        gotchi->playClip("fallover", false);  // play once, hold last frame
    }
    if (deathHoldTimer_ >= 0.0f) {
        deathHoldTimer_ -= deltaTime;
    }

    // Mouse wheel zoom - allows zooming in/out around cursor position
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) getCamera()->zoomAtScreen(GetMousePosition(), wheel * 0.25f);

    // Update merge button label if seenReality has changed
    if (mergeController_) {
        const char* currentLabel = mergeController_->mergeButtonLabel();
        // Merge is always the last button added in addButtons() (row 2,
        // index 3 of 4 -- i.e. the last of the whole 8-button vector), but
        // indexing that positionally is exactly what broke last time the
        // layout changed (a stale buttons[6] silently started overwriting
        // "Sleep" once the row grew from 7 to 8 buttons). Look it up by the
        // Button object actually flagged as the merge button instead.
        if (mergeButton_) {
            std::string currentButtonLabel = mergeButton_->getLabel();
            if (currentButtonLabel != currentLabel) {
                mergeButton_->setLabel(currentLabel);
            }
        }
    }

    // Sleep only appears once the player has merged at least once
    // (GameState::seenReality) -- it doesn't exist as an option before then.
    if (gameState_) {
        for (auto& btn : buttons) {
            if (btn->getLabel() == "Sleep") {
                btn->setVisible(gameState_->seenReality);
                break;
            }
        }
    }

    // Update button states
    SceneInputHandler* input = getInputHandler();
    applyTutorialLocks();
    for (auto& btn : buttons) {
        btn->update(input, deltaTime);
    }

    // Advance/reveal the tutorial dialog while it belongs to this scene; once
    // an advance crosses into a "hexboard" step, follow it there immediately.
    if (tutorialController_ && tutorialController_->isActive() && tutorialController_->currentScene() == "gotchi") {
        tutorialController_->update(deltaTime);
        if (tutorialController_->currentScene() == "hexboard" && getSceneManager()) {
            static_cast<SceneManager*>(getSceneManager())->switchScene("hexboard");
        } else if (tutorialController_->isFinished()) {
            tutorialController_->finish();

            // The tutorial makes the player actually press Feed/Pet/Wash to
            // teach the buttons, which for real changes hunger/happiness/
            // cleanliness/mood -- reset that back to a fresh starting state
            // once the tutorial is done, so those forced practice presses
            // don't leave the gotchi in a different state than a player who
            // skips nothing sees at the real start of the game.
            if (gameState_) {
                gameState_->vitals.reset();
                gameState_->mood.clearMoodOverlays();
                gameState_->mood.setCurrentMood(GotchiMoodType::MOOD_00_HAPPY);
            }
        }
    }

    // Update cooldown timers
    for (auto& [name, timer] : buttonCooldowns_) {
        if (timer > 0.0f) timer -= deltaTime;
    }

    // Update button feedback timer
    if (buttonFeedbackTimer_ > 0.0f) {
        buttonFeedbackTimer_ -= deltaTime;
        if (buttonFeedbackTimer_ <= 0.0f) {
            lastClickedButton_.clear();
        }
    }

    // Update action overlay timer
    if (actionOverlayTimer_ > 0.0f) {
        actionOverlayTimer_ -= deltaTime;
    }

    // Update simulation time and frame count
    simTime_ += deltaTime;
    frameCount_++;
}

void GotchiScene::draw() {
    // Draw background first (fills 720x720 window)
    // Must be before Scene::draw() so gotchi appears on top
    if (background_.id != 0) {
        DrawTexture(background_, 0, 0, WHITE);
    }

    Scene::draw();

    if (!gotchi) return;

    // ==============================
    // LEFT PANEL: Core Vital Stats
    // ==============================
    int lx = 16;
    int ly = 40;

    // Backing pill so the panel reads over busy background art -- sized to
    // the actual content (header + 6 stat rows), not an oversized guess.
    DrawRectangleRounded({ (float)lx - 10, (float)ly - 12, 225.0f, 140.0f }, 0.12f, 8, {0, 0, 0, 140});

    DrawText("VITAL STATS", lx, ly, 14, {100, 200, 255, 255});
    ly += 18;

    // Health bar
    float health = gotchi->getStats().getNormalizedStat(SecondaryStat::FITALITY);
    DrawText("Health", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});  // bg bar
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * health), 10, {0, 255, 0, 255});  // fg bar
    DrawText(std::to_string(static_cast<int>(health * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 18;

    // Hunger bar (inverted - 0 is full, 100 is starving)
    float hunger = gotchi->getStats().getNormalizedStat(SecondaryStat::FOOD_LEVEL);
    DrawText("Hunger", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * (1.0f - hunger)), 10, {255, 200, 0, 255});
    DrawText(std::to_string(static_cast<int>(hunger * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Energy bar
    float energy = gotchi->getStats().getNormalizedStat(SecondaryStat::ENERGY);
    DrawText("Energy", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * energy), 10, {255, 150, 0, 255});
    DrawText(std::to_string(static_cast<int>(energy * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Thirst bar
    float thirst = gotchi->getStats().getNormalizedStat(SecondaryStat::HYDRATION);
    DrawText("Thirst", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * (1.0f - thirst)), 10, {0, 150, 255, 255});
    DrawText(std::to_string(static_cast<int>(thirst * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Sleep bar (inverted - 0 is rested, 100 is exhausted)
    float sleep = gotchi->getStats().getNormalizedStat(SecondaryStat::SLEEP_DEBT);
    DrawText("Sleep", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * (1.0f - sleep)), 10, {100, 50, 150, 255});
    DrawText(std::to_string(static_cast<int>(sleep * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Hygiene bar
    float hygiene = gotchi->getStats().getNormalizedStat(SecondaryStat::CLEANLINESS);
    DrawText("Hygiene", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * hygiene), 10, {0, 200, 200, 255});
    DrawText(std::to_string(static_cast<int>(hygiene * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 24;

    // ==============================
    // TOP-RIGHT: Current Mood + Needs Attention
    // ==============================
    // Panel is anchored to the right screen edge with a fixed margin and
    // sized to its actual content (title/mood/needs-lines), rather than a
    // fixed left x-position -- the old rx=460 left a big unused gap between
    // the panel and the screen edge, and the pill width used the wrap
    // target width instead of the widest line actually drawn.
    const int panelMargin = 16;
    const int panelPadding = 14;
    const int titleFontSize = 14;
    const int moodFontSize = 24;
    const int needsFontSize = 16;
    const int needsLineHeight = needsFontSize + 5;
    const int maxPanelContentWidth = 280;  // wrap budget for needs-attention text

    // Needs Attention text -- wrapped to fit the wrap budget, computed up
    // front so we know the whole panel's size before drawing its pill.
    std::vector<std::string> needs;
    float happiness = gotchi->getStats().getNormalizedStat(EmotionalStat::HAPPINESS);
    if (hunger > 0.7f) needs.push_back("Hungry");
    if (thirst > 0.7f) needs.push_back("Thirsty");
    if (sleep > 0.7f) needs.push_back("Sleepy");
    if (health < 0.3f) needs.push_back("Sick");
    if (happiness < 0.3f) needs.push_back("Sad");

    std::vector<std::string> needsLines;
    if (!needs.empty()) {
        std::string needsText = "NEEDS ATTENTION: ";
        for (size_t i = 0; i < needs.size(); i++) {
            if (i > 0) needsText += ", ";
            needsText += needs[i];
        }

        std::istringstream wordStream(needsText);
        std::string word, currentLine;
        while (wordStream >> word) {
            std::string testLine = currentLine.empty() ? word : currentLine + " " + word;
            if (MeasureText(testLine.c_str(), needsFontSize) > maxPanelContentWidth && !currentLine.empty()) {
                needsLines.push_back(currentLine);
                currentLine = word;
            } else {
                currentLine = testLine;
            }
        }
        if (!currentLine.empty()) needsLines.push_back(currentLine);
    }

    // Panel content width = widest of title/mood-name/each needs line,
    // clamped to the wrap budget (needs lines never exceed it; title/mood
    // name are short enough in practice that this rarely binds).
    std::string moodName = gotchi->getMood().getMoodName();
    int contentWidth = std::max(MeasureText("CURRENT MOOD", titleFontSize), MeasureText(moodName.c_str(), moodFontSize));
    for (const auto& line : needsLines) {
        contentWidth = std::max(contentWidth, MeasureText(line.c_str(), needsFontSize));
    }
    contentWidth = std::min(contentWidth, maxPanelContentWidth);

    int panelWidth = contentWidth + panelPadding * 2;
    int rx = GAME_W - panelMargin - panelWidth + panelPadding;
    int ry = 40;

    int panelHeight = titleFontSize + 6 + moodFontSize + 10 + (int)needsLines.size() * needsLineHeight;
    DrawRectangleRounded(
        { (float)(rx - panelPadding), (float)(ry - panelPadding),
          (float)panelWidth, (float)(panelHeight + panelPadding * 2) },
        0.12f, 8, {0, 0, 0, 150});

    // Current Mood
    DrawText("CURRENT MOOD", rx, ry, titleFontSize, {255, 220, 100, 255});
    ry += titleFontSize + 6;
    DrawText(moodName.c_str(), rx, ry, moodFontSize, {255, 255, 255, 255});
    ry += moodFontSize + 10;

    // Needs Attention -- plain text on the shared panel background, no
    // separate colored pill of its own.
    for (const auto& line : needsLines) {
        DrawText(line.c_str(), rx, ry, needsFontSize, {255, 140, 140, 255});
        ry += needsLineHeight;
    }

    // Draw button click message above action buttons
    if (!lastClickedButton_.empty()) {
        int msgFontSize = 24;
        int msgWidth = MeasureText(lastClickedButton_.c_str(), msgFontSize);
        int msgX = (GAME_W - msgWidth) / 2;
        int msgY = GAME_H - 210;  // was -240; moved down 30px

        int pillPadding = 8;
        Rectangle msgPill = {
            (float)(msgX - pillPadding), (float)(msgY - pillPadding),
            (float)(msgWidth + pillPadding * 2), (float)(msgFontSize + pillPadding * 2)
        };
        DrawRectangleRounded(msgPill, 0.4f, 8, {0, 0, 0, 160});

        DrawText(lastClickedButton_.c_str(), msgX, msgY, msgFontSize, {255, 255, 255, 255});
    }

    // Draw buttons with cooldown overlay
    for (auto& btn : buttons) {
        // Draw cooldown overlay if active
        float cd = buttonCooldowns_[btn->getLabel()];
        if (cd > 0.0f) {
            Color overlay = {0, 0, 0, (unsigned char)std::min(160.0f, cd * 80.0f)};
            Rectangle bounds = btn->getBounds();
            DrawRectangleRec(bounds, overlay);
        }
        btn->draw();
    }

    // Draw the tutorial dialog on top of everything while it's this scene's turn
    if (tutorialController_ && tutorialController_->isActive() && tutorialController_->currentScene() == "gotchi") {
        tutorialController_->draw();
    }

    // Draw action shader overlay on top of the gotchi
    if (actionOverlayTimer_ > 0.0f && actionOverlayMode_ >= 0) {
        Rectangle worldRect = getGotchiScreenRect();

        // Get the four corners in world space
        Vector2 worldTL = {worldRect.x, worldRect.y};
        Vector2 worldBR = {worldRect.x + worldRect.width, worldRect.y + worldRect.height};

        // Convert to screen coordinates
        Vector2 screenTL = getCamera()->worldToScreen(worldTL);
        Vector2 screenBR = getCamera()->worldToScreen(worldBR);

        Rectangle r = {
            screenTL.x,
            screenTL.y,
            screenBR.x - screenTL.x,
            screenBR.y - screenTL.y
        };

        float progress = 1.0f - (actionOverlayTimer_ / actionOverlayDuration_);
        float t = (float)GetTime();
        Vector2 res = {r.width, r.height};

        BeginShaderMode(actionShader_);
        SetShaderValue(actionShader_, locMode_, &actionOverlayMode_, SHADER_UNIFORM_INT);
        SetShaderValue(actionShader_, locProgress_, &progress, SHADER_UNIFORM_FLOAT);
        SetShaderValue(actionShader_, locTime_, &t, SHADER_UNIFORM_FLOAT);
        SetShaderValue(actionShader_, locResolution_, &res, SHADER_UNIFORM_VEC2);
        DrawTexturePro(whitePixel_,
                       {0, 0, 1, 1},   // full source of the 1x1 tex -> UVs 0..1 across dest
                       r,              // dest = gotchi screen rect
                       {0, 0}, 0.0f, WHITE);
        EndShaderMode();
    }
}

void GotchiScene::addNavigationButton(const std::string& label, const std::string& targetScene, float x, float y) {
    float buttonWidth = 160.0f;
    float buttonHeight = 32.0f;
    Button* btn = new Button({x, y}, buttonWidth, buttonHeight, label);
    btn->setAnchor("center");
    btn->setFontSize(14);
    btn->setBackgroundColor({60, 60, 100, 220});
    btn->setHoverColor({100, 100, 160, 240});
    btn->setBorderColor({150, 150, 200, 255});

    btn->setOnClick([this, targetScene]() {
        if (getSceneManager()) {
            SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
            mgr->switchScene(targetScene);
        }
    });
    buttons.push_back(std::unique_ptr<Button>(btn));
}

// ============================================================================
// Gotchi Action Handler
// ============================================================================
void GotchiScene::handleGotchiAction(const std::string& action) {
    if (!gotchi || gotchi->isDead()) {
        lastClickedButton_ = "Cannot interact - Gotchi is dead";
        buttonFeedbackTimer_ = 1.5f;
        return;
    }

    // Sleeping guard: only "Merge" (to wake) is allowed while sleeping
    if (gotchi->isSleeping() && action != "Merge") {
        lastClickedButton_ = "Gotchi is sleeping - give a break to wake";
        buttonFeedbackTimer_ = 1.5f;
        return;
    }

    float& cooldown = buttonCooldowns_[action];
    if (cooldown > 0.0f) {
        lastClickedButton_ = "Wait - Cooldown: " +
            std::to_string(static_cast<int>(cooldown) + 1) + "s";
        buttonFeedbackTimer_ = 1.0f;
        return;
    }

    auto& stats = gotchi->getStats();
    auto& mood = gotchi->getMood();
    std::string feedback;
    int shaderMode = -1;
    float shaderDur = 0.0f;
    bool success = true;
    int actionCode = -1;  // For CareAction event

    // Every care action gives a direct FITALITY (health) boost, plus stacks
    // a temporary regen-rate boost (decays back to 0 -- see
    // GotchiStats::boostHealthRegen()/healthRegenBoost_) so pressing buttons
    // repeatedly gives a real burst of ongoing recovery, not just one-time
    // flat adds. Previously nothing touched health directly, so it could
    // only ever recover via the slow passive AWAKE_HEALTH_REGEN trickle,
    // which couldn't outrun neglect penalties even with perfect play.
    // Actions with more actual "care" weight (Feed/Water/Sleep addressing a
    // fast-draining need) heal more.
    const float HEALTH_BOOST_MINOR = 8.0f;   // Wash, Groom, Pet
    const float HEALTH_BOOST_MAJOR = 15.0f;  // Feed, Water, Sleep
    const float HEALTH_REGEN_BOOST_MINOR = 15.0f;
    const float HEALTH_REGEN_BOOST_MAJOR = 25.0f;

    if (action == "Wash") {
        gotchi->clean();
        stats.addStat(EmotionalStat::SATISFACTION, 5.0f);
        stats.addStat(SecondaryStat::FITALITY, HEALTH_BOOST_MINOR);
        stats.boostHealthRegen(HEALTH_REGEN_BOOST_MINOR);
        mood.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 5.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_14_PEACEFUL, 8.0f);
        feedback = "Washed - Cleanliness 100%";
        shaderMode = 0; shaderDur = 1.5f;
        actionCode = 0;
    } else if (action == "Groom") {
        gotchi->interact();
        stats.addStat(EmotionalStat::HAPPINESS, 10.0f);
        stats.addStat(EmotionalStat::FRIEDNSHIP, 2.0f); // NOTE: enum is misspelled
        stats.addStat(SecondaryStat::FITALITY, HEALTH_BOOST_MINOR);
        stats.boostHealthRegen(HEALTH_REGEN_BOOST_MINOR);
        mood.addMoodOverlay(GotchiMoodType::MOOD_10_JOYFUL, 5.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_52_BEAUTIFUL, 10.0f);
        feedback = "Groomed - Happy & shiny!";
        shaderMode = 1; shaderDur = 3.0f;
        actionCode = 1;
    } else if (action == "Feed") {
        gotchi->feed();
        stats.addStat(EmotionalStat::SATISFACTION, 5.0f);
        stats.addStat(SecondaryStat::FITALITY, HEALTH_BOOST_MAJOR);
        stats.boostHealthRegen(HEALTH_REGEN_BOOST_MAJOR);
        mood.addMoodOverlay(GotchiMoodType::MOOD_02_SATISFIED, 10.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 5.0f);
        feedback = "Fed - Hunger reduced";
        shaderMode = 2; shaderDur = 3.0f;
        actionCode = 2;
    } else if (action == "Pet") {
        gotchi->interact();
        stats.addStat(EmotionalStat::HAPPINESS, 10.0f);
        stats.addStat(EmotionalStat::LOVE, 5.0f);
        stats.addStat(EmotionalStat::FRIEDNSHIP, 3.0f); // NOTE: enum is misspelled
        stats.addStat(SecondaryStat::FITALITY, HEALTH_BOOST_MINOR);
        stats.boostHealthRegen(HEALTH_REGEN_BOOST_MINOR);
        mood.addMoodOverlay(GotchiMoodType::MOOD_10_JOYFUL, 8.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_30_LOVING, 15.0f);
        feedback = "Petted - Happiness up";
        shaderMode = 3; shaderDur = 2.0f;
        actionCode = 3;
    } else if (action == "Water") {
        // Thirst lives in SecondaryStat::HYDRATION (0 = hydrated, 100 = dehydrated).
        // Giving water REDUCES the dehydration value.
        stats.addStat(SecondaryStat::HYDRATION, -30.0f);
        stats.addStat(EmotionalStat::SATISFACTION, 5.0f);
        stats.addStat(SecondaryStat::FITALITY, HEALTH_BOOST_MAJOR);
        stats.boostHealthRegen(HEALTH_REGEN_BOOST_MAJOR);
        mood.addMoodOverlay(GotchiMoodType::MOOD_12_RELIEVED, 5.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 4.0f);
        feedback = "Watered - Thirst quenched";
        shaderMode = 4; shaderDur = 2.5f;
        actionCode = 4;
    } else if (action == "Merge") {
        // Intentionally inert for now. Rest/sleep will be triggered by the events
        // system, not by this button. Show a neutral message; no stats, no mood,
        // no shader, no cooldown, no sleep/wake state change.
        lastClickedButton_ = "Not available yet";
        buttonFeedbackTimer_ = 1.5f;
        return;   // early-out BEFORE the success block, so no cooldown/shader fires
    } else if (action == "Sleep") {
        stats.setStat(SecondaryStat::SLEEP_DEBT, 0.0f);
        stats.addStat(EmotionalStat::SATISFACTION, 5.0f);
        stats.addStat(SecondaryStat::FITALITY, HEALTH_BOOST_MAJOR);
        stats.boostHealthRegen(HEALTH_REGEN_BOOST_MAJOR);
        mood.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 8.0f);
        feedback = "Slept - Fully rested";
        // No shader overlay for this one -- shaderMode stays -1.
    } else {
        success = false;
    }

    if (success) {
        lastClickedButton_ = feedback;
        buttonFeedbackTimer_ = 3.0f;
        cooldown = 2.0f;
        if (shaderMode >= 0) triggerActionShader(shaderMode, shaderDur);

        if (tutorialController_) tutorialController_->reportAction(action);

        // Emit CareAction event for Box C (drivers computation)
        // Action codes: 0=Wash, 1=Groom, 2=Feed, 3=Pet, 4=Water
        // Magnitude: how much this action contributes (0.0 to 1.0)
        if (eventBus_ && actionCode >= 0) {
            // Warmth actions (pet only) drive affection
            // Hygiene actions (wash/groom) drive mercy
            float magnitude = 1.0f;  // Full care action
            eventBus_->emit(Event::careAction(actionCode, magnitude));
        }
    }
}

Rectangle GotchiScene::getGotchiScreenRect() {
    if (!gotchi) return {0, 0, 0, 0};
    // Get the actor's screen position and size
    float w = gotchi->getWidth() * gotchi->getScale().x;
    float h = gotchi->getHeight() * gotchi->getScale().y;
    return {gotchi->getPosition().x - w/2, gotchi->getPosition().y - h/2, w, h};
}

void GotchiScene::triggerActionShader(int mode, float duration) {
    actionOverlayMode_ = mode;
    actionOverlayDuration_ = duration;
    actionOverlayTimer_ = duration;
}

void GotchiScene::cleanup() {
    Scene::cleanup();

    // Unload background texture
    if (background_.id != 0) {
        UnloadTexture(background_);
        background_ = {0};
    }

    // Unload action shader
    if (actionShader_.id != 0) {
        UnloadShader(actionShader_);
        actionShader_ = {0};
    }

    // Unload 1x1 white texture
    if (whitePixel_.id != 0) {
        UnloadTexture(whitePixel_);
        whitePixel_ = {0};
    }

    // Unsubscribe from CareAction events
    if (eventBus_ && careActionToken_ > 0) {
        eventBus_->unsubscribe(careActionToken_);
        careActionToken_ = 0;
    }

    gotchi = nullptr;
}

void GotchiScene::applyTutorialLocks() {
    bool collapsed = gameState_ && gameState_->sleepCollapsed;

    for (auto& btn : buttons) {
        const std::string& label = btn->getLabel();
        if (label == "DETAILED VITALS") continue;  // never gated

        if (label == "Merge") {
            // Merge is locked at all times except the sleep-collapse gate --
            // the tutorial never unlocks it (see TutorialController's hard
            // "merge" exception), so this check alone covers both cases.
            btn->setEnabled(collapsed);
            continue;
        }

        // Once collapsed, every other button (including EXPLORE) locks so
        // the only way forward is Merge.
        if (collapsed) {
            btn->setEnabled(false);
            continue;
        }

        if (tutorialController_) {
            btn->setEnabled(tutorialController_->isActionUnlocked(label));
        }
    }
}

// ============================================================================
// Background Management
// ============================================================================

void GotchiScene::loadBiomeBackgrounds() {
    // Biome names matching the TileType biomes
    const std::vector<std::string> biomes = {
        "ocean", "sand", "grass", "dirt", "swamp", "ice", "lava"
    };

    for (const auto& biome : biomes) {
        std::string path = "gotchi_backgrounds/" + biome + "_bg.png";
        Texture2D tex = AssetPack::loadTexture(path);
        if (tex.id != 0) {
            biomeBackgrounds_[biome] = tex;
            TraceLog(LOG_INFO, "GOTCHI_BG loaded: %s", path.c_str());
        } else {
            TraceLog(LOG_WARNING, "GOTCHI_BG failed to load: %s", path.c_str());
        }
    }

    // Set background based on gotchiBiome from GameState
    std::string biome = gameState_ ? gameState_->gotchiBiome : "grass";
    if (biomeBackgrounds_.count(biome)) {
        if (background_.id != 0) {
            UnloadTexture(background_);
        }
        background_ = biomeBackgrounds_.at(biome);
        currentBiome_ = biome;
    } else {
        // Fallback to grass if biome not found
        if (background_.id != 0) {
            UnloadTexture(background_);
        }
        background_ = biomeBackgrounds_.count("grass") ? biomeBackgrounds_.at("grass") : AssetPack::loadTexture("gotchi_backgrounds/grass_bg.png");
        currentBiome_ = "grass";
    }
}

void GotchiScene::updateBackgroundForHex(int q, int r) {
    // Look up biome from GameState (updated when leaving HexViewScene)
    if (gameState_) {
        std::string biome = gameState_->gotchiBiome;
        if (biome != currentBiome_ && biomeBackgrounds_.count(biome)) {
            TraceLog(LOG_INFO, "GOTCHI_BG changing from %s to %s", currentBiome_.c_str(), biome.c_str());
            if (background_.id != 0) {
                UnloadTexture(background_);
            }
            background_ = biomeBackgrounds_.at(biome);
            currentBiome_ = biome;
        }
    }
}

void GotchiScene::applySleepCollapseGate() {
    if (!gameState_ || !gotchi) return;

    // Trips off the visible Sleep bar (SLEEP_DEBT, 0=rested/100=exhausted --
    // what the player actually watches on the HUD), not the separate hidden
    // 0..1 "sleep" metronome, which no longer forces or gates anything.
    //
    // Only trip while the gotchi is actually out in the world -- Mode::Story
    // already means merge is irrelevant (we're mid-beat), so don't collapse
    // out from under a story scene.
    bool sleepMaxed = gotchi->getStats().getStat(SecondaryStat::SLEEP_DEBT) >= 100.0f;
    if (!gameState_->sleepCollapsed && sleepMaxed && gameState_->mode == Mode::Gotchi) {
        gameState_->sleepCollapsed = true;
        gotchi->setAction("wobble");
    }
}

void GotchiScene::onExploreButtonClicked() {
    // Switch to hexboard scene
    if (getSceneManager()) {
        SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
        mgr->switchScene("hexboard");
    }
}
