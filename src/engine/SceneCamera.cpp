#include "SceneCamera.hpp"
#include "SceneActor.hpp"
#include "GameConstants.hpp"

SceneCamera::SceneCamera(float x, float y, float zoom)
    : position({x, y}), targetPosition({x, y}), zoom(zoom), targetZoom(zoom), zoomDuration(0.0f), zoomTimer(0.0f),
      rotation(0.0f), targetRotation(0.0f), rotateDuration(0.0f), rotateTimer(0.0f),
      followTarget(nullptr), followSpeed(CAMERA_FOLLOW_SPEED),
      lookaheadEnabled(false), lookaheadDistance(CAMERA_LOOKAHEAD_DISTANCE), lookaheadOffset({0, 0}),
      shakeIntensity(0.0f), shakeDuration(0.0f), shakeTimer(0.0f),
      pulseIntensity(0.0f), pulseDuration(0.0f), pulseTimer(0.0f),
      boundaryEnabled(false), boundaryMinX(0), boundaryMinY(0),
      boundaryMaxX(0), boundaryMaxY(0) {
}

void SceneCamera::update(float deltaTime) {
    updateFollow(deltaTime);
    updateLookahead(deltaTime);
    updateZoom(deltaTime);
    updateRotation(deltaTime);
    updateShake(deltaTime);
    updatePulse(deltaTime);
    if (boundaryEnabled) clampToBoundary();
}

void SceneCamera::setPosition(float x, float y) { position = {x, y}; targetPosition = {x, y}; }
void SceneCamera::setPosition(Vector2 pos)       { position = pos; targetPosition = pos; }
Vector2 SceneCamera::getPosition() const         { return position; }

void SceneCamera::setZoom(float z) {
    zoom = z; targetZoom = z;
    if (zoom < CAMERA_MIN_ZOOM) zoom = CAMERA_MIN_ZOOM;
    if (zoom > CAMERA_MAX_ZOOM) zoom = CAMERA_MAX_ZOOM;
}
float SceneCamera::getZoom() const { return zoom; }
void SceneCamera::addZoom(float dz) { setZoom(zoom + dz); }

void SceneCamera::zoomTo(float targetZ, float duration) {
    targetZoom = targetZ;
    if (targetZoom < CAMERA_MIN_ZOOM) targetZoom = CAMERA_MIN_ZOOM;
    if (targetZoom > CAMERA_MAX_ZOOM) targetZoom = CAMERA_MAX_ZOOM;
    zoomDuration = duration;
    zoomTimer = 0.0f;
}

void SceneCamera::setRotation(float angle) { rotation = angle; targetRotation = angle; }
float SceneCamera::getRotation() const { return rotation; }
void SceneCamera::rotateTo(float targetAngle, float duration) {
    targetRotation = targetAngle; rotateDuration = duration; rotateTimer = 0.0f;
}

void SceneCamera::followActor(SceneActor* actor, float smoothSpeed) { followTarget = actor; followSpeed = smoothSpeed; targetPosition = actor->getPosition(); }
void SceneCamera::followPosition(Vector2 pos, float smoothSpeed) { followTarget = nullptr; targetPosition = pos; followSpeed = smoothSpeed; }
void SceneCamera::stopFollowing() { followTarget = nullptr; }
bool SceneCamera::isFollowing() const { return followTarget != nullptr; }

void SceneCamera::setLookaheadEnabled(bool enabled) { lookaheadEnabled = enabled; }
void SceneCamera::setLookaheadDistance(float distance) { lookaheadDistance = distance; }
bool SceneCamera::isLookaheadEnabled() const { return lookaheadEnabled; }

void SceneCamera::shake(float intensity, float duration) {
    shakeIntensity = intensity; shakeDuration = duration; shakeTimer = 0.0f;
}

void SceneCamera::setBoundary(float minX, float minY, float maxX, float maxY) {
    boundaryEnabled = true;
    boundaryMinX = minX; boundaryMinY = minY;
    boundaryMaxX = maxX; boundaryMaxY = maxY;
}
void SceneCamera::disableBoundary() { boundaryEnabled = false; }

Vector2 SceneCamera::screenToWorld(Vector2 screenPos) const {
    return GetScreenToWorld2D(screenPos, getRaylibCamera());
}
Vector2 SceneCamera::worldToScreen(Vector2 worldPos) const {
    return GetWorldToScreen2D(worldPos, getRaylibCamera());
}

void SceneCamera::pulse(float intensity, float duration) {
    pulseIntensity = intensity; pulseDuration = duration; pulseTimer = 0.0f;
}

Camera2D SceneCamera::getRaylibCamera() const {
    Camera2D camera = {};
    camera.target   = position;
    camera.offset   = { GAME_W / 2.0f, GAME_H / 2.0f };
    camera.rotation = rotation;
    camera.zoom     = zoom * (1.0f + pulseIntensity);
    return camera;
}

void SceneCamera::updateFollow(float deltaTime) {
    if (followTarget) {
        Vector2 targetPos = followTarget->getPosition();
        targetPos.x += lookaheadOffset.x;
        targetPos.y += lookaheadOffset.y;
        position.x = lerp(position.x, targetPos.x, followSpeed * deltaTime);
        position.y = lerp(position.y, targetPos.y, followSpeed * deltaTime);
    } else if (targetPosition.x != position.x || targetPosition.y != position.y) {
        // Smoothly follow a position (not an actor)
        position.x = lerp(position.x, targetPosition.x, followSpeed * deltaTime);
        position.y = lerp(position.y, targetPosition.y, followSpeed * deltaTime);
    }
}

void SceneCamera::updateShake(float deltaTime) {
    if (shakeTimer >= shakeDuration) return;
    shakeTimer += deltaTime;
    float progress = shakeTimer / shakeDuration;
    float currentIntensity = shakeIntensity * (1.0f - progress) * CAMERA_SHAKE_DAMPING;
    position.x += (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;
    position.y += (GetRandomValue(-100, 100) / 100.0f) * currentIntensity;
}

void SceneCamera::updateZoom(float deltaTime) { zoom = lerp(zoom, targetZoom, 8.0f * deltaTime); }

void SceneCamera::updateRotation(float deltaTime) {
    if (rotateTimer >= rotateDuration || rotateDuration == 0.0f) { rotation = targetRotation; return; }
    rotateTimer += deltaTime;
    rotation = lerp(rotation, targetRotation, rotateTimer / rotateDuration);
}

void SceneCamera::updateLookahead(float deltaTime) {
    if (!lookaheadEnabled || !followTarget) { lookaheadOffset = {0, 0}; return; }
    Vector2 velocity = followTarget->getVelocity();
    float speed = std::sqrt(velocity.x * velocity.x + velocity.y * velocity.y);
    if (speed > 0.1f) {
        lookaheadOffset.x = (velocity.x / speed) * lookaheadDistance;
        lookaheadOffset.y = (velocity.y / speed) * lookaheadDistance;
    } else {
        lookaheadOffset = {0, 0};
    }
}

void SceneCamera::updatePulse(float deltaTime) {
    if (pulseTimer >= pulseDuration) { pulseIntensity = 0.0f; return; }
    pulseTimer += deltaTime;
    pulseIntensity *= (1.0f - pulseTimer / pulseDuration);
}

void SceneCamera::clampToBoundary() {
    float hw = (GAME_W / zoom) * 0.5f;
    float hh = (GAME_H / zoom) * 0.5f;
    if (position.x - hw < boundaryMinX) position.x = boundaryMinX + hw;
    if (position.x + hw > boundaryMaxX) position.x = boundaryMaxX - hw;
    if (position.y - hh < boundaryMinY) position.y = boundaryMinY + hh;
    if (position.y + hh > boundaryMaxY) position.y = boundaryMaxY - hh;
}

float SceneCamera::lerp(float a, float b, float t) const {
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    return a + (b - a) * t;
}
