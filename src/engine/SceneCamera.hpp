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
    void stopFollowing();
    bool isFollowing() const;
    
    // Lookahead - camera looks ahead in direction of movement
    void setLookaheadEnabled(bool enabled);
    void setLookaheadDistance(float distance);
    bool isLookaheadEnabled() const;
    
    // Screen shake effect
    void shake(float intensity, float duration);
    
    // Boundary clamping
    void setBoundary(float minX, float minY, float maxX, float maxY);
    void disableBoundary();
    
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
    
    // Lookahead
    bool lookaheadEnabled;
    float lookaheadDistance;
    Vector2 lookaheadOffset;
    
    // Screen shake
    float shakeIntensity;
    float shakeDuration;
    float shakeTimer;
    
    // Pulse
    float pulseIntensity;
    float pulseDuration;
    float pulseTimer;
    
    // Boundaries
    bool boundaryEnabled;
    float boundaryMinX, boundaryMinY;
    float boundaryMaxX, boundaryMaxY;
    
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
