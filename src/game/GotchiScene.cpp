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

    // Load gotchi background (default to grass_bg)
    background_ = AssetPack::loadTexture("gotchi_backgrounds/grass_bg.png");

    // Create Gotchi with shared vitals and mood from GameState
    // If gameState_ is not set (e.g., in tests), use fallback defaults
    GotchiStats& stats = gameState_ ? gameState_->vitals : defaultStats_;
    GotchiMood& mood = gameState_ ? gameState_->mood : defaultMood_;

    gotchi = new Gotchi({360.0f, 360.0f}, stats, mood, gameState_);
    gotchi->setTag("gotchi");
    addActor(gotchi);

    // Initialize the Gotchi (no longer resets vitals - they persist in GameState)
    gotchi->init();

    // Load animation frames using the Gotchi's loader
    gotchi->loadAnimationFrames(gotchiDir);

    // Fix 1: Set initial action AFTER loading frames - this now properly starts idle animation
    gotchi->setAction("idle");

    // Fix 2: Disable wandering so gotchi stays still in idle state
    gotchi->setWanderEnabled(false);

    // Fix 3: Size the gotchi based on actual frame dimensions (native 64px)
    Vector2 frameSize = gotchi->getFrameSize();
    if (frameSize.x > 0 && frameSize.y > 0) {
        gotchi->setSize(frameSize.x, frameSize.y);  // Base size = native frame pixels
    }

    // Framing: set fixed world scale and compute camera zoom to fill ~60% of screen
    gotchi->setScale({ GOTCHI_WORLD_SCALE, GOTCHI_WORLD_SCALE });
    float spriteWorldPx = gotchi->getHeight() * GOTCHI_WORLD_SCALE;      // 64 * 2 = 128px
    float targetPx      = GOTCHI_SCREEN_FRAC * (float)GAME_H;            // 0.60 * 720 = 432px
    float framingZoom   = targetPx / spriteWorldPx;                       // 3.375
    getCamera()->setPosition(360.0f, 360.0f);                             // center on the gotchi
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
    lastClickedButton_ = "";
    buttonCooldowns_.clear();
    buttonFeedbackTimer_ = 0.0f;

    // Add action buttons at the bottom
    // Seven buttons: Wash, Groom, Feed, Pet, Water, Merge, Explore
    float buttonWidth = 80.0f;
    float buttonHeight = 32.0f;
    float totalWidth = 7 * buttonWidth + 6 * 10.0f;  // 7 buttons + 6 gaps
    float startX = (GAME_W - totalWidth) / 2.0f;
    float y = GAME_H - 40.0f;

    std::string mergeLabel = mergeController_ ? mergeController_->mergeButtonLabel() : "Merge";

    std::vector<std::string> labels = {"Wash", "Groom", "Feed", "Pet", "Water", mergeLabel, "EXPLORE"};
    for (size_t i = 0; i < labels.size(); i++) {
        float x = startX + i * (buttonWidth + 10.0f);
        // The 6th button (index 5) is the merge button
        addButton(labels[i], x, y, (i == 5));
    }

    // Add "Detailed Vitals" button at bottom center
    addNavigationButton("DETAILED VITALS", "gotchi_stats", (float)GAME_W / 2.0f, (float)GAME_H - 80);
}

void GotchiScene::addButton(const std::string& label, float x, float y, bool isMergeButton) {
    float buttonWidth = 80.0f;
    float buttonHeight = 32.0f;
    Button* btn = new Button({x, y}, buttonWidth, buttonHeight, label);
    btn->setAnchor("top-left");
    btn->setFontSize(12);
    btn->setBackgroundColor({60, 60, 100, 220});
    btn->setHoverColor({100, 100, 160, 240});
    btn->setBorderColor({150, 150, 200, 255});

    if (isMergeButton) {
        // Merge button emits MergeRequested on the event bus
        btn->setOnClick([this]() {
            onMergeButtonClicked();
        });
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
    // gotchi has collapsed from exhaustion -- see Gotchi::setStatsFrozen()
    // for what this does/doesn't skip.
    if (gotchi) {
        bool tutorialFreeze = tutorialController_ && tutorialController_->isActive();
        bool collapseFreeze = gameState_ && gameState_->sleepCollapsed;
        gotchi->setStatsFrozen(tutorialFreeze || collapseFreeze);
        gotchi->update(deltaTime);

        // One-shot: fire the death transition exactly once. switchScene()'s
        // own currentSceneName guard isn't enough here since the fade takes
        // time to actually leave this scene -- without deathTriggered_ this
        // would re-call switchScene() every frame of the fade and reset its
        // timer forever.
        if (gotchi->isDead() && !deathTriggered_) {
            deathTriggered_ = true;
            if (getSceneManager()) {
                static_cast<SceneManager*>(getSceneManager())->switchScene("death");
            }
        }
    }

    // Mouse wheel zoom - allows zooming in/out around cursor position
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) getCamera()->zoomAtScreen(GetMousePosition(), wheel * 0.25f);

    // Update merge button label if seenReality has changed
    if (mergeController_) {
        const char* currentLabel = mergeController_->mergeButtonLabel();
        // Check if the merge button label needs to be updated
        if (buttons.size() > 5) {
            std::string currentButtonLabel = buttons[5]->getLabel();
            if (currentButtonLabel != currentLabel) {
                buttons[5]->setLabel(currentLabel);
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
    // CENTER: Mood Display
    // ==============================
    int cx = 280;
    int cy = 40;

    // Current Mood (large, prominent)
    DrawText("CURRENT MOOD", cx, cy, 14, {255, 220, 100, 255});
    cy += 20;
    std::string moodName = gotchi->getMood().getMoodName();
    DrawText(moodName.c_str(), cx, cy, 24, {255, 255, 255, 255});
    cy += 35;

    // Mood color indicator
    Color moodTint = gotchi->getMood().getMoodTint();
    DrawRectangle(cx, cy, 40, 40, moodTint);
    DrawRectangleLines(cx, cy, 40, 40, {255, 255, 255, 200});
    cy += 50;

    // Mood description
    DrawText("Mood Drivers:", cx, cy, 12, {150, 150, 200, 255});
    cy += 16;

    // Calculate and display key mood drivers
    float happiness = gotchi->getStats().getNormalizedStat(EmotionalStat::HAPPINESS);
    float excitement = gotchi->getStats().getNormalizedStat(EmotionalStat::EXCITEMENT);
    float satisfaction = gotchi->getStats().getNormalizedStat(EmotionalStat::SATISFACTION);

    std::vector<std::pair<std::string, float>> moodDrivers;
    moodDrivers.push_back({"Happiness", happiness});
    moodDrivers.push_back({"Excitement", excitement});
    moodDrivers.push_back({"Satisfaction", satisfaction});
    moodDrivers.push_back({"Energy", energy});
    moodDrivers.push_back({"Health", health});

    for (const auto& driver : moodDrivers) {
        DrawText((driver.first + ": " + std::to_string(static_cast<int>(driver.second * 100)) + "%").c_str(), cx, cy, 11, {180, 180, 200, 255});
        cy += 14;
    }

    // ==============================
    // TOP-RIGHT: Status & Info
    // ==============================
    int rx = 520;
    int ry = 40;

    // Draw a blinking/running indicator to show simulation is active
    float blinkTimer = std::fmod(simTime_, 1.0f);
    bool blinkOn = blinkTimer < 0.5f;

    if (blinkOn) {
        DrawText("RUNNING", rx, ry, 12, {100, 255, 100, 255});
    } else {
        DrawText("RUNNING", rx, ry, 12, {50, 200, 50, 255});
    }
    ry += 16;

    // Status indicators
    std::string statusText;
    if (gotchi->isSleeping()) {
        statusText = "[SLEEPING] - Needs rest";
        DrawText(statusText.c_str(), rx, ry, 12, {150, 100, 255, 255});
        ry += 16;
    } else if (gotchi->isDead()) {
        statusText = "[DEAD] - Game Over";
        DrawText(statusText.c_str(), rx, ry, 12, {255, 50, 50, 255});
        ry += 16;
    } else if (!gotchi->isActive()) {
        statusText = "[INACTIVE] - Click to interact";
        DrawText(statusText.c_str(), rx, ry, 12, {255, 200, 50, 255});
        ry += 16;
    } else {
        // Check for needs that require action
        std::vector<std::string> needs;
        if (hunger > 0.7f) needs.push_back("Hungry");
        if (thirst > 0.7f) needs.push_back("Thirsty");
        if (sleep > 0.7f) needs.push_back("Sleepy");
        if (health < 0.3f) needs.push_back("Sick");
        if (happiness < 0.3f) needs.push_back("Sad");

        if (!needs.empty()) {
            for (size_t i = 0; i < needs.size(); i++) {
                if (i > 0) statusText += ", ";
                statusText += needs[i];
            }
            statusText = "[NEEDS ATTENTION] " + statusText;
            DrawText(statusText.c_str(), rx, ry, 12, {255, 100, 100, 255});
            ry += 16;
        } else {
            DrawText("[ALL GOOD] - Gotchi is content", rx, ry, 12, {100, 255, 100, 255});
            ry += 16;
        }
    }

    // Show formatted time
    int totalSeconds = static_cast<int>(simTime_);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    std::ostringstream clock;
    clock << "Time: " << (minutes < 10 ? "0" : "") << minutes << ":"
          << (seconds < 10 ? "0" : "") << seconds;
    DrawText(clock.str().c_str(), rx, ry, 11, {200, 200, 200, 255});
    ry += 16;

    // Show tick rate
    std::string tickText = "Ticks: " + std::to_string(static_cast<int>(gotchi->getStats().getStat(SecondaryStat::AGE)));
    DrawText(tickText.c_str(), rx, ry, 11, {150, 150, 200, 255});
    ry += 16;

    // Stats summary
    DrawText("STAT SUMMARY", rx, ry, 12, {150, 150, 255, 255});
    ry += 16;

    std::ostringstream summary;
    summary << "Health: " << static_cast<int>(health * 100)
            << " | Hunger: " << static_cast<int>(hunger * 100)
            << " | Energy: " << static_cast<int>(energy * 100)
            << " | Mood: " << moodName;
    DrawText(summary.str().c_str(), rx, ry, 11, {200, 200, 200, 255});

    // Draw button click message above action buttons
    if (!lastClickedButton_.empty()) {
        int msgWidth = MeasureText(lastClickedButton_.c_str(), 12);
        int msgX = (GAME_W - msgWidth) / 2;
        DrawText(lastClickedButton_.c_str(), msgX, GAME_H - 140, 12, {255, 255, 255, 255});
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

    if (action == "Wash") {
        gotchi->clean();
        stats.addStat(EmotionalStat::SATISFACTION, 5.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 5.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_14_PEACEFUL, 8.0f);
        feedback = "Washed - Cleanliness 100%";
        shaderMode = 0; shaderDur = 1.5f;
        actionCode = 0;
    } else if (action == "Groom") {
        gotchi->interact();
        stats.addStat(EmotionalStat::HAPPINESS, 10.0f);
        stats.addStat(EmotionalStat::FRIEDNSHIP, 2.0f); // NOTE: enum is misspelled
        mood.addMoodOverlay(GotchiMoodType::MOOD_10_JOYFUL, 5.0f);
        mood.addMoodOverlay(GotchiMoodType::MOOD_52_BEAUTIFUL, 10.0f);
        feedback = "Groomed - Happy & shiny!";
        shaderMode = 1; shaderDur = 3.0f;
        actionCode = 1;
    } else if (action == "Feed") {
        gotchi->feed();
        stats.addStat(EmotionalStat::SATISFACTION, 5.0f);
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

void GotchiScene::applySleepCollapseGate() {
    if (!gameState_) return;

    // Only trip while the gotchi is actually out in the world -- Mode::Story
    // already means merge is irrelevant (we're mid-beat), so don't collapse
    // out from under a story scene.
    if (!gameState_->sleepCollapsed && gameState_->sleep <= 0.0f && gameState_->mode == Mode::Gotchi) {
        gameState_->sleepCollapsed = true;
        if (gotchi) gotchi->setAction("wobble");
    }
}

void GotchiScene::onExploreButtonClicked() {
    // Switch to hexboard scene
    if (getSceneManager()) {
        SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
        mgr->switchScene("hexboard");
    }
}
