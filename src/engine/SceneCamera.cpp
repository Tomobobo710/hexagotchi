#include "SceneCamera.hpp"
#include "SceneActor.hpp"
#include "GameConstants.hpp"
#include <cmath>

SceneCamera::SceneCamera(float x, float y, float zoom)
    : position({x, y}), targetPosition({x, y}), zoom(zoom), targetZoom(zoom), zoomDuration(0.0f), zoomTimer(0.0f),
      rotation(0.0f), targetRotation(0.0f), rotateDuration(0.0f), rotateTimer(0.0f),
      followTarget(nullptr), followSpeed(CAMERA_FOLLOW_SPEED),
      panning(false),
      lookaheadEnabled(false), lookaheadDistance(CAMERA_LOOKAHEAD_DISTANCE), lookaheadOffset({0, 0}),
      shakeIntensity(0.0f), shakeDuration(0.0f), shakeTimer(0.0f),
      wideViewEnabled(false),
      pulseIntensity(0.0f), pulseDuration(0.0f), pulseTimer(0.0f),
      boundaryEnabled(false), boundaryMinX(0), boundaryMinY(0),
      boundaryMaxX(0), boundaryMaxY(0), cachedMinZoom(CAMERA_MIN_ZOOM),
      // Pan margin (overscroll padding)
      panMargin(1000.0f),
      // Inertia state
      velocity({0, 0}), prevPosition({x, y}), lastPanMouse({0, 0}), inertiaEnabled(true),
      inertiaFriction(6.0f), minFlingSpeed(40.0f), velTau(0.06f),
      // Zoom animation state
      zoomAnchorWorld({0, 0}), zoomAnchorScreen({0, 0}), zoomLerpRate(12.0f), zoomAnimating(false) {
}

void SceneCamera::update(float deltaTime) {
    if (deltaTime <= 0.0f) deltaTime = 1.0f / 60.0f;

    // Handle pan momentum and update velocity during active pan
    if (panning) {
        // Estimate velocity from the movement applied this frame
        Vector2 raw = {(position.x - prevPosition.x) / deltaTime,
                       (position.y - prevPosition.y) / deltaTime};
        float a = 1.0f - expf(-deltaTime / velTau);   // framerate independent EMA weight
        velocity.x += (raw.x - velocity.x) * a;
        velocity.y += (raw.y - velocity.y) * a;
    } else if (inertiaEnabled) {
        // Apply momentum when not panning
        float speed2 = velocity.x * velocity.x + velocity.y * velocity.y;
        if (speed2 > minFlingSpeed * minFlingSpeed) {
            position.x += velocity.x * deltaTime;
            position.y += velocity.y * deltaTime;
            float decay = expf(-inertiaFriction * deltaTime);  // framerate independent
            velocity.x *= decay;
            velocity.y *= decay;
        } else {
            // Stop if below threshold
            velocity = {0.0f, 0.0f};
        }
    }

    // Handle zoom animation (before other updates to avoid lerp conflicts)
    if (zoomAnimating) {
        float a = 1.0f - expf(-zoomLerpRate * deltaTime);
        zoom += (targetZoom - zoom) * a;
        if (fabsf(targetZoom - zoom) < 0.0005f) { zoom = targetZoom; zoomAnimating = false; }

        // Re-pin: place position so zoomAnchorWorld projects back to zoomAnchorScreen
        float ox = GAME_W * 0.5f, oy = GAME_H * 0.5f;
        position.x = zoomAnchorWorld.x - (zoomAnchorScreen.x - ox) / zoom;
        position.y = zoomAnchorWorld.y - (zoomAnchorScreen.y - oy) / zoom;
    }

    // Update other camera systems
    updateFollow(deltaTime);
    updateLookahead(deltaTime);
    updateZoom(deltaTime);
    updateRotation(deltaTime);
    updateShake(deltaTime);
    updatePulse(deltaTime);

    // Clamp is the single authority on position. Capture pre-clamp so we can
    // kill momentum on any axis that hit a wall.
    Vector2 pre = position;
    if (boundaryEnabled) clampToBoundary();
    if (position.x != pre.x) velocity.x = 0.0f;
    if (position.y != pre.y) velocity.y = 0.0f;

    // Keep targetPosition coherent with manual pan/inertia moves (which drive
    // position directly, bypassing targetPosition entirely). Skipped while
    // followPosition()'s lerp is still chasing targetPosition -- otherwise
    // this would stomp the target back onto position every frame and stall
    // the lerp permanently (see focusCameraOn() callers).
    bool movingViaPanOrInertia = panning || (inertiaEnabled && (velocity.x != 0.0f || velocity.y != 0.0f));
    if (!isFollowing() && movingViaPanOrInertia) targetPosition = position;

    // Wide view overrides everything above, every frame it's enabled. Note
    // this runs BEFORE each scene's own update() (SceneCamera::update() is
    // called from the top of Scene::update(), and a scene's ambient logic
    // runs after that returns) -- so a one-shot override here alone would
    // still get stomped by a scene's unconditional
    // "getCamera()->setPosition(...); getCamera()->setZoom(1.0f);" later in
    // the same frame. Each world scene's ambient block additionally checks
    // !isWideViewEnabled() before re-pinning, same pattern as the existing
    // isShaking() guard, so the two together make wide view actually stick.
    if (wideViewEnabled && boundaryEnabled) {
        float boundaryW = boundaryMaxX - boundaryMinX;
        float boundaryH = boundaryMaxY - boundaryMinY;
        float fitZoom = fminf((float)GAME_W / boundaryW, (float)GAME_H / boundaryH);
        zoom = fitZoom;
        targetZoom = fitZoom;
        position = {boundaryMinX + boundaryW * 0.5f, boundaryMinY + boundaryH * 0.5f};
        targetPosition = position;
    }

    prevPosition = position;
}

void SceneCamera::panUpdate(Vector2 mouseScreen) {
    if (!panning) {
        panning = true;
        lastPanMouse = mouseScreen;
        prevPosition = position;
        velocity = {0.0f, 0.0f};
        return;  // No motion on the grab frame, no spike
    }
    Vector2 d = {mouseScreen.x - lastPanMouse.x,
                 mouseScreen.y - lastPanMouse.y};
    // Drag-to-move: world moves with the cursor, 1:1 in world space
    position.x -= d.x / zoom;
    position.y -= d.y / zoom;
    lastPanMouse = mouseScreen;  // ALWAYS update, unconditional
}

void SceneCamera::panEnd() {
    panning = false;
}

void SceneCamera::setPosition(float x, float y) {
    position = {x, y};
    // Clamp immediately, not just once per frame in update(). setPosition()
    // can be called AFTER update() has already run for this frame (e.g. a
    // scene's ambient re-pin runs after Scene::update()), and draw() reads
    // position directly -- so an unclamped set here would render
    // out-of-bounds for a full frame and, if re-set every frame, forever.
    // Clamping at the setter makes it impossible to hold an off-boundary
    // position regardless of call order or a bad hand-typed value.
    if (boundaryEnabled) clampToBoundary();
    targetPosition = position;
}
void SceneCamera::setPosition(Vector2 pos) { setPosition(pos.x, pos.y); }
Vector2 SceneCamera::getPosition() const         { return position; }

void SceneCamera::setZoom(float z) {
    float lo = minZoomForBounds();
    zoom = z; targetZoom = z;
    if (zoom < lo) zoom = lo;
    if (zoom > CAMERA_MAX_ZOOM) zoom = CAMERA_MAX_ZOOM;
}
float SceneCamera::getZoom() const { return zoom; }
void SceneCamera::addZoom(float dz) { setZoom(zoom + dz); }

void SceneCamera::zoomTo(float targetZ, float duration) {
    float lo = minZoomForBounds();
    targetZoom = targetZ;
    if (targetZoom < lo) targetZoom = lo;
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

void SceneCamera::setPanning(bool panning) { this->panning = panning; }
bool SceneCamera::isPanning() const { return panning; }

void SceneCamera::setLookaheadEnabled(bool enabled) { lookaheadEnabled = enabled; }
void SceneCamera::setLookaheadDistance(float distance) { lookaheadDistance = distance; }
bool SceneCamera::isLookaheadEnabled() const { return lookaheadEnabled; }

void SceneCamera::shake(float intensity, float duration) {
    shakeIntensity = intensity; shakeDuration = duration; shakeTimer = 0.0f;
    // Park the follow at the current position so updateFollow() isn't lerping
    // toward some other target while the shake runs -- otherwise it pulls the
    // per-frame shake offset straight back out and no shake is visible (this
    // bit OfficeScene, which re-issues followPosition every frame to track a
    // walking actor). Callers wanting the camera to move AND shake should
    // re-aim after the shake settles.
    followTarget = nullptr;
    targetPosition = position;
}

bool SceneCamera::isShaking() const { return shakeTimer < shakeDuration; }

void SceneCamera::setWideViewEnabled(bool enabled) { wideViewEnabled = enabled; }
bool SceneCamera::isWideViewEnabled() const { return wideViewEnabled; }
void SceneCamera::toggleWideView() { wideViewEnabled = !wideViewEnabled; }

void SceneCamera::setBoundary(float minX, float minY, float maxX, float maxY) {
    boundaryEnabled = true;
    boundaryMinX = minX; boundaryMinY = minY;
    boundaryMaxX = maxX; boundaryMaxY = maxY;
    // Cache min zoom for fast access
    float worldW = boundaryMaxX - boundaryMinX;
    float worldH = boundaryMaxY - boundaryMinY;
    float zx = (float)GAME_W / worldW;
    float zy = (float)GAME_H / worldH;
    // Use fminf to allow zooming out to see the whole map
    // Apply panMargin so minZoom is based on world + overscroll padding
    float margin = panMargin / fminf(zx, zy);  // convert screen px margin to world units at min zoom
    float minWorldW = worldW + margin;
    float minWorldH = worldH + margin;
    float minZX = (float)GAME_W / minWorldW;
    float minZY = (float)GAME_H / minWorldH;
    cachedMinZoom = fminf(minZX, minZY);
}

void SceneCamera::disableBoundary() { boundaryEnabled = false; }

float SceneCamera::minZoomForBounds() const {
    return cachedMinZoom;
}

void SceneCamera::setPanMargin(float margin) {
    panMargin = margin;
    // Recalculate cached min zoom when margin changes
    float worldW = boundaryMaxX - boundaryMinX;
    float worldH = boundaryMaxY - boundaryMinY;
    if (worldW > 0 && worldH > 0 && boundaryEnabled) {
        float zx = (float)GAME_W / worldW;
        float zy = (float)GAME_H / worldH;
        float marginWorld = panMargin / fminf(zx, zy);
        float minWorldW = worldW + marginWorld;
        float minWorldH = worldH + marginWorld;
        float minZX = (float)GAME_W / minWorldW;
        float minZY = (float)GAME_H / minWorldH;
        cachedMinZoom = fminf(minZX, minZY);
    }
}

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
    // Snap camera position to whole pixels to prevent shimmering effect
    // when zooming or scrolling. This ensures pixel-perfect rendering
    // by aligning the camera target to the pixel grid.
    camera.target   = { roundf(position.x), roundf(position.y) };
    camera.offset   = { GAME_W / 2.0f, GAME_H / 2.0f };
    camera.rotation = rotation;
    camera.zoom     = zoom * (1.0f + pulseIntensity);
    return camera;
}

void SceneCamera::updateFollow(float deltaTime) {
    // Skip interpolation when panning to avoid snap-back
    if (panning) {
        return;
    }

    // Threshold for position comparison - ignore tiny differences to prevent
    // camera from constantly micro-adjusting due to floating-point precision
    const float POSITION_THRESHOLD = 0.01f;

    if (followTarget) {
        Vector2 targetPos = followTarget->getPosition();
        targetPos.x += lookaheadOffset.x;
        targetPos.y += lookaheadOffset.y;
        position.x = lerp(position.x, targetPos.x, followSpeed * deltaTime);
        position.y = lerp(position.y, targetPos.y, followSpeed * deltaTime);
    } else if (std::fabs(targetPosition.x - position.x) > POSITION_THRESHOLD ||
               std::fabs(targetPosition.y - position.y) > POSITION_THRESHOLD) {
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
    Vector2 vel = followTarget->getVelocity();
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    if (speed > 0.1f) {
        lookaheadOffset.x = (vel.x / speed) * lookaheadDistance;
        lookaheadOffset.y = (vel.y / speed) * lookaheadDistance;
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
    // Visible half-width/half-height, shrunk by a fixed inward safety
    // margin (see BOUNDARY_SAFETY_INSET_X/_Y) -- this is the hard clamp
    // every scene's setPosition()/zoomTo()/focus shot is subject to, using
    // the TRUE boundary rect (not inflated by panMargin, which is
    // drag-overscroll UX padding, a different concern -- see the field
    // comment in the header). Without this, a story-camera focus shot near
    // the scene edge (e.g. an actor placed near a wall) could center on
    // empty space past the boundary, or leave no room for shake before
    // revealing it. This is unconditional -- wide view overrides
    // position/zoom entirely after this runs (see update()), so it's
    // unaffected by either inset.
    // The safety margin scales with zoom, same as the visible half-extent it
    // adds to -- otherwise a flat pixel inset (90px) is a tiny sliver when
    // zoomed out but eats half the frame when zoomed in (at zoom 2 the visible
    // half-height is only 180px, so a flat 90 inset would block the camera
    // from ever reaching the lower half of a room). Dividing by zoom keeps the
    // margin a consistent fraction of what's ACTUALLY in view at any zoom.
    float hw = (GAME_W * 0.5f + BOUNDARY_SAFETY_INSET_X) / zoom;
    float hh = (GAME_H * 0.5f + BOUNDARY_SAFETY_INSET_Y) / zoom;

    float loX = boundaryMinX + hw, hiX = boundaryMaxX - hw;
    if (loX <= hiX) { if (position.x < loX) position.x = loX; if (position.x > hiX) position.x = hiX; }
    else           { position.x = (boundaryMinX + boundaryMaxX) * 0.5f; }

    float loY = boundaryMinY + hh, hiY = boundaryMaxY - hh;
    if (loY <= hiY) { if (position.y < loY) position.y = loY; if (position.y > hiY) position.y = hiY; }
    else           { position.y = (boundaryMinY + boundaryMaxY) * 0.5f; }
}

float SceneCamera::lerp(float a, float b, float t) const {
    if (t > 1.0f) t = 1.0f;
    if (t < 0.0f) t = 0.0f;
    return a + (b - a) * t;
}

// Zoom API - smooth zoom-to-cursor animation
void SceneCamera::zoomAtScreen(Vector2 mouseScreen, float deltaZoom) {
    float lo = minZoomForBounds();
    float hi = CAMERA_MAX_ZOOM;
    targetZoom = fmaxf(lo, fminf(hi, targetZoom + deltaZoom));
    zoomAnchorScreen = mouseScreen;
    zoomAnchorWorld  = screenToWorld(mouseScreen);  // world point under cursor now
    zoomAnimating = true;
}

void SceneCamera::setZoomLerpRate(float rate) {
    zoomLerpRate = rate;
}

void SceneCamera::setInertiaEnabled(bool enabled) {
    inertiaEnabled = enabled;
}
