#ifndef SCENE_CAMERA_HPP
#define SCENE_CAMERA_HPP

#include "raylib.h"
#include <cmath>

// Camera constants
const float CAMERA_DEFAULT_ZOOM = 1.0f;
const float CAMERA_MIN_ZOOM = 0.5f;
const float CAMERA_MAX_ZOOM = 5.0f;
const float CAMERA_FOLLOW_SPEED = 5.0f;
const float CAMERA_SHAKE_DAMPING = 0.95f;
const float CAMERA_LOOKAHEAD_DISTANCE = 100.0f;

class SceneActor; // Forward declaration

class SceneCamera {
public:
    // Constructor
    SceneCamera(float x, float y, float zoom = CAMERA_DEFAULT_ZOOM);
    
    // Update - call this each frame
    void update(float deltaTime);
    
    // Position control
    void setPosition(float x, float y);
    void setPosition(Vector2 pos);
    Vector2 getPosition() const;
    
    // Zoom control
    void setZoom(float zoom);
    float getZoom() const;
    void addZoom(float deltaZoom);
    void zoomTo(float targetZoom, float duration);
    
    // Rotation control
    void setRotation(float angle);
    float getRotation() const;
    void rotateTo(float targetAngle, float duration);
    
    // Following actor
    void followActor(SceneActor* actor, float smoothSpeed = CAMERA_FOLLOW_SPEED);
    void followPosition(Vector2 pos, float smoothSpeed = CAMERA_FOLLOW_SPEED);
    void stopFollowing();
    bool isFollowing() const;

    // Panning mode - disables automatic position interpolation
    void setPanning(bool panning);
    bool isPanning() const;

    // Panning API - for drag-to-move with fling
    void panUpdate(Vector2 mouseScreen);
    void panEnd();

    // Zoom API - smooth zoom-to-cursor animation
    void zoomAtScreen(Vector2 mouseScreen, float deltaZoom);
    void setZoomLerpRate(float rate);
    void setInertiaEnabled(bool enabled);

    // Lookahead - camera looks ahead in direction of movement
    void setLookaheadEnabled(bool enabled);
    void setLookaheadDistance(float distance);
    bool isLookaheadEnabled() const;
    
    // Screen shake effect
    void shake(float intensity, float duration);
    bool isShaking() const;

    // Wide view: zooms out to show the whole scene (using the boundary rect
    // set via setBoundary()) instead of the scene's normal gameplay framing.
    // Every scene's ambient logic re-pins position/zoom unconditionally each
    // frame (same reason isShaking() exists) -- so this is enforced as the
    // last step of update(), after everything else, rather than being a
    // one-shot setPosition()/setZoom() call a scene's own update() would
    // immediately stomp back out.
    void setWideViewEnabled(bool enabled);
    bool isWideViewEnabled() const;
    void toggleWideView();
    
    // Boundary clamping
    void setBoundary(float minX, float minY, float maxX, float maxY);
    void setPanMargin(float margin);
    void disableBoundary();
    float minZoomForBounds() const;
    
    // Coordinate conversion
    Vector2 screenToWorld(Vector2 screenPos) const;
    Vector2 worldToScreen(Vector2 worldPos) const;
    
    // Get the internal raylib camera
    Camera2D getRaylibCamera() const;
    
    // Pulse effect - useful for emphasis
    void pulse(float intensity, float duration);
    
private:
    // Core position
    Vector2 position;
    Vector2 targetPosition;

    // Zoom and rotation
    float zoom;
    float targetZoom;
    float zoomDuration;
    float zoomTimer;

    float rotation;
    float targetRotation;
    float rotateDuration;
    float rotateTimer;

    // Following
    SceneActor* followTarget;
    float followSpeed;

    // Panning state - when true, disables automatic position interpolation
    bool panning;

    // Lookahead
    bool lookaheadEnabled;
    float lookaheadDistance;
    Vector2 lookaheadOffset;

    // Screen shake
    float shakeIntensity;
    float shakeDuration;
    float shakeTimer;

    // Wide view
    bool wideViewEnabled;

    // Pulse
    float pulseIntensity;
    float pulseDuration;
    float pulseTimer;

    // Boundaries
    bool boundaryEnabled;
    float boundaryMinX, boundaryMinY;
    float boundaryMaxX, boundaryMaxY;

    // Cached min zoom (computed once when boundary changes)
    float cachedMinZoom;

    // Pan margin (overscroll padding in world units) -- only affects how far
    // a user's drag-to-pan can pull past the boundary edge (see panUpdate()/
    // panEnd()), NOT the hard clamp every setPosition()/zoomTo()/focus shot
    // is subject to. clampToBoundary() uses safetyInset instead (see below);
    // panMargin being large is fine/intentional for drag UX, since that's a
    // separate, deliberately generous overscroll allowance.
    float panMargin;

    // Inward safety margin (screen pixels) subtracted from the visible
    // half-width/half-height in clampToBoundary() -- keeps the camera's
    // actual visible frame at least this far inside the true boundary rect
    // at all times, so panning/zooming to frame an off-center actor (or
    // camera shake) never reveals empty space past the scene's edge.
    static constexpr float BOUNDARY_SAFETY_INSET = 30.0f;

    // Inertia state (for fling momentum)
    Vector2 velocity;
    Vector2 prevPosition;
    Vector2 lastPanMouse;
    bool    inertiaEnabled;

    // Inertia tuning (default values, can be tuned via setters)
    float inertiaFriction;   // per-sec exponential decay
    float minFlingSpeed;     // world u/s below which we snap to rest
    float velTau;            // velocity smoothing time constant

    // Zoom animation state (for smooth zoom-to-cursor)
    Vector2 zoomAnchorWorld;   // world point to keep pinned during zoom
    Vector2 zoomAnchorScreen;  // screen point it should stay under
    float   zoomLerpRate;      // per-sec, default 12.0
    bool    zoomAnimating;

    // Helper methods
    void updateFollow(float deltaTime);
    void updateShake(float deltaTime);
    void updateZoom(float deltaTime);
    void updateRotation(float deltaTime);
    void updateLookahead(float deltaTime);
    void updatePulse(float deltaTime);
    void clampToBoundary();
    float lerp(float a, float b, float t) const;
};

#endif // SCENE_CAMERA_HPP
