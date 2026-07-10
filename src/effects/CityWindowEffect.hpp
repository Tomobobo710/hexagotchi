#ifndef CITY_WINDOW_EFFECT_HPP
#define CITY_WINDOW_EFFECT_HPP

#include "SceneEffect.hpp"
#include "raylib.h"
#include "rlights.h"
#include <vector>

// One-off 3D background effect for ApartmentScene: a night city block seen
// through the bedroom window's transparent-cutout glass in apartmentbg.png.
// Three parallel streets of buildings recede into the distance (buildings are
// lit cubes; their windows are flat quads that individually flicker on/off),
// with one building capping the centerline view. Periodically a two-car train
// rushes past just outside the glass; ApartmentScene polls
// consumeTrainShakeRequest() each frame and shakes its own camera for the
// duration reported by getShakeDuration() (effects don't hold a Scene/camera
// reference, same as SchoolSkyEffect).
class CityWindowEffect : public SceneEffect {
public:
    void init() override;
    void update(float deltaTime) override;
    void drawBackground() override;
    void cleanup() override;

    // Returns true once, a little before the train first becomes visible in
    // the window; clears itself after being read so callers only trigger
    // one shake per pass. getShakeDuration() gives the full lead-in + actual
    // on-screen time (both cars) + trail-out span to pass to
    // SceneCamera::shake() -- computed from real visible-window math (see
    // update()), not a fixed guessed number, so it stays correct if the
    // debug camera controls change fovy/distance.
    bool consumeTrainShakeRequest();
    float getShakeDuration() const { return shakeDuration; }

    // SceneEffect's debug-camera interface (see SceneEffect.hpp) -- lets
    // any scene's shared debug-camera controls (SceneDebugCamera.hpp) tune
    // this effect's internal 3D camera without knowing its concrete type.
    bool hasDebugCamera() const override { return true; }
    float getDebugCamDist() const override { return debugCamDist; }
    void setDebugCamDist(float dist) override { debugCamDist = dist; }
    float getDebugPitch() const override { return debugPitchDeg; }
    void setDebugPitch(float deg) override { debugPitchDeg = deg; }
    float getDebugFovy() const override { return debugFovyDeg; }
    void setDebugFovy(float deg) override { debugFovyDeg = deg; }

private:
    struct Building {
        Vector3 basePos;   // center of the building's front face
        Vector3 size;      // width, height, depth
        Color wallColor;
        int windowCols;
        int windowRows;
        // +1: also draw a window grid on the +X side face (this building's
        // inner/centerline-facing side is +X, i.e. it's a "left" building in
        // its row). -1: same but on the -X face ("right" building). 0: no
        // side windows (the single centered cap building has no row-mate
        // beside it, so there's no inward side to matter).
        int sideWindowSign = 0;
        // windowCols*windowRows entries for the front (+Z) face, followed by
        // windowRows*sideDepthCols entries for the side face when
        // sideWindowSign != 0 -- one flat array so both faces share the same
        // per-window flicker/lit bookkeeping in updateWindows().
        std::vector<bool> windowLit;
        std::vector<float> windowTimer;
    };
    std::vector<Building> buildings;

    // Lit shader rig for building walls (windows stay flat/unlit -- they're
    // meant to glow, not receive light). Same LitShader/CreateLight0 pattern
    // as SchoolSkyEffect/MoonEffect: one shared directional light, pointing
    // mostly down and off to the side per this scene's chosen angle.
    Shader shader;
    Light light;
    Model wallModel;       // unit cube, scaled per-building at draw time
    Texture2D whiteTex = {0};

    void buildBuildings();
    // Adds one street row (3 building pairs receding in Z), shifted sideways
    // by rowXOffset and in depth by rowZOffset. The Z offset keeps parallel
    // rows off each other's depth planes (see buildBuildings()).
    void addBuildingRow(float rowXOffset, float rowZOffset);
    // Column count for a building's inner side-face window grid, derived
    // from its depth (the side face's width) the same way the front grid
    // derives column spacing from the building's width -- a narrower
    // building gets fewer side windows instead of the same fixed count
    // stretched thin.
    static int sideWindowCols(const Building& b);
    void drawBuilding(const Building& b) const;
    void updateWindows(Building& b, float deltaTime);
    void drawTrainCar(float x, float trainZ) const;  // x is a world position, not a 0..1 progress value

    // Train: a dark box with lit windows that races across close to the
    // glass (small Z, near the camera) on a slow repeating timer.
    float trainTimer = 0.0f;
    float trainInterval = 14.0f;   // seconds of quiet between passes
    bool trainActive = false;
    float trainT = 0.0f;           // 0..1 progress across its pass
    float trainDuration = 2.8f;    // seconds for a car to cross the full X path
    bool trainShakeRequested = false;
    bool trainShakeFiredThisPass = false;
    float shakeTriggerT = 0.0f;   // trainT value at which the shake should fire
    float shakeDuration = 0.0f;   // computed per-pass; see getShakeDuration()

    // Camera framing, tuned live via the shared debug-camera controls
    // (SceneDebugCamera.hpp). Distance is kept > 0 (the look vector and
    // lens-shift both scale with it, so 0 collapses the view); 0.1 is the
    // floor the debug control clamps to.
    float debugCamDist = 0.1f;
    float debugPitchDeg = -8.0f;
    float debugFovyDeg = 45.0f;
};

#endif // CITY_WINDOW_EFFECT_HPP
