#include "SceneActor.hpp"

SceneActor::SceneActor(Vector2 pos, float w, float h)
    : position(pos), velocity({0, 0}), scale({1.0f, 1.0f}), rotation(0.0f),
      width(w), height(h), friction(ACTOR_DEFAULT_FRICTION), gravityEnabled(false),
      waypointIndex(-1), waypointSpeed(0.0f),
      texture({0}), color(WHITE),
      frameWidth(0), frameHeight(0), frameCount(1), frameDuration(0.1f),
      animLoop(true), currentFrame(0), frameTimer(0.0f), animPlaying(true),
      animating(false), animIsFrameList(false),
      active(true), visible(true), tag(""), layer(ACTOR_LAYER_MIDGROUND),
      clickable(false), hovered(false), pressed(false), showClickFeedback(true) {
}

void SceneActor::update(float deltaTime) {
    // Apply gravity
    if (gravityEnabled) {
        velocity.y += ACTOR_GRAVITY * deltaTime;
    }

    // Apply friction
    velocity.x *= friction;
    velocity.y *= friction;

    // Apply velocity
    position.x += velocity.x * deltaTime;
    position.y += velocity.y * deltaTime;

    // Advance waypoint movement (see moveTo()) -- independent of the
    // velocity/friction/gravity physics above, since scripted scenario
    // movement should walk exactly where it's told, not drift or decelerate.
    if (waypointIndex >= 0 && waypointIndex < (int)waypoints.size()) {
        Vector2 target = waypoints[waypointIndex];
        Vector2 delta = {target.x - position.x, target.y - position.y};
        float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);

        if (dist <= ACTOR_WAYPOINT_ARRIVAL_DIST) {
            position = target;
            waypointIndex++;
            if (waypointIndex >= (int)waypoints.size()) {
                waypointIndex = -1;  // Reached the last waypoint -- done.
            }
        } else {
            float step = waypointSpeed * deltaTime;
            if (step >= dist) {
                position = target;
            } else {
                position.x += delta.x / dist * step;
                position.y += delta.y / dist * step;
            }
        }
    }

    // Advance sprite-sheet animation
    if (animating && animPlaying && frameCount > 1) {
        frameTimer += deltaTime;
        while (frameTimer >= frameDuration) {
            frameTimer -= frameDuration;
            int next = currentFrame + 1;
            if (next >= frameCount) {
                if (animLoop) {
                    currentFrame = 0;
                } else {
                    currentFrame = frameCount - 1;
                    animPlaying = false;
                    break;
                }
            } else {
                currentFrame = next;
            }
        }
    }
}

void SceneActor::draw() {
    if (!visible) return;

    const Texture2D* drawTexture = &texture;
    Rectangle src;
    bool drewSprite = true;

    if (animating && animIsFrameList) {
        if (animFrames.empty()) {
            DrawRectangleV(position, {width, height}, color);
            drewSprite = false;
        } else {
            drawTexture = &animFrames[currentFrame];
            src = {0.0f, 0.0f, (float)drawTexture->width, (float)drawTexture->height};
        }
    } else if (animating) {
        src = {(float)(currentFrame * frameWidth), 0.0f, (float)frameWidth, (float)frameHeight};
    } else if (texture.id != 0) {
        src = {0.0f, 0.0f, (float)texture.width, (float)texture.height};
    } else {
        // Draw placeholder rectangle
        DrawRectangleV(position, {width, height}, color);
        drewSprite = false;
    }

    if (drewSprite) {
        Rectangle dest = {position.x, position.y, width * scale.x, height * scale.y};
        Vector2 origin = {dest.width / 2.0f, dest.height / 2.0f};
        dest.x += origin.x;
        dest.y += origin.y;
        DrawTexturePro(*drawTexture, src, dest, origin, rotation, color);
    }

    // Clickable hover/press feedback: a translucent tint over the actor's bounds.
    if (clickable && showClickFeedback && (hovered || pressed)) {
        Rectangle bounds = getBounds();
        DrawRectangleRec(bounds, pressed ? ACTOR_PRESSED_TINT : ACTOR_HOVER_TINT);
    }
}

void SceneActor::setPosition(Vector2 pos) {
    position = pos;
}

Vector2 SceneActor::getPosition() const {
    return position;
}

void SceneActor::move(Vector2 delta) {
    position.x += delta.x;
    position.y += delta.y;
}

void SceneActor::moveTo(const std::vector<Vector2>& newWaypoints, float speed) {
    waypoints = newWaypoints;
    waypointSpeed = speed;
    waypointIndex = waypoints.empty() ? -1 : 0;
}

bool SceneActor::isMoving() const {
    return waypointIndex >= 0;
}

void SceneActor::stopMoving() {
    waypointIndex = -1;
}

void SceneActor::setRotation(float angle) {
    rotation = angle;
}

float SceneActor::getRotation() const {
    return rotation;
}

void SceneActor::setScale(Vector2 s) {
    scale = s;
}

Vector2 SceneActor::getScale() const {
    return scale;
}

void SceneActor::setVelocity(Vector2 vel) {
    velocity = vel;
}

Vector2 SceneActor::getVelocity() const {
    return velocity;
}

void SceneActor::addVelocity(Vector2 vel) {
    velocity.x += vel.x;
    velocity.y += vel.y;
}

void SceneActor::setSize(float w, float h) {
    width = w;
    height = h;
}

float SceneActor::getWidth() const {
    return width;
}

float SceneActor::getHeight() const {
    return height;
}

Rectangle SceneActor::getBounds() const {
    return Rectangle{position.x, position.y, width * scale.x, height * scale.y};
}

bool SceneActor::isCollidingWith(const SceneActor* other) const {
    if (!other) return false;
    return CheckCollisionRecs(getBounds(), other->getBounds());
}

Vector2 SceneActor::getCenter() const {
    return Vector2{position.x + width / 2.0f, position.y + height / 2.0f};
}

void SceneActor::setFriction(float f) {
    friction = f;
}

float SceneActor::getFriction() const {
    return friction;
}

void SceneActor::setGravityEnabled(bool enabled) {
    gravityEnabled = enabled;
}

bool SceneActor::isGravityEnabled() const {
    return gravityEnabled;
}

void SceneActor::setActive(bool a) {
    active = a;
}

bool SceneActor::isActive() const {
    return active;
}

void SceneActor::setVisible(bool v) {
    visible = v;
}

bool SceneActor::isVisible() const {
    return visible;
}

void SceneActor::setTag(const std::string& t) {
    tag = t;
}

std::string SceneActor::getTag() const {
    return tag;
}

void SceneActor::setLayer(int l) {
    layer = l;
}

int SceneActor::getLayer() const {
    return layer;
}

void SceneActor::setTexture(Texture2D tex) {
    texture = tex;
    clearAnimation();
}

void SceneActor::setColor(Color col) {
    color = col;
}

Color SceneActor::getColor() const {
    return color;
}

void SceneActor::setAnimation(int fw, int fh, int count, float duration, bool loop) {
    frameWidth = fw;
    frameHeight = fh;
    frameCount = count > 0 ? count : 1;
    frameDuration = duration;
    animLoop = loop;
    currentFrame = 0;
    frameTimer = 0.0f;
    animPlaying = true;
    animating = true;
    animIsFrameList = false;
    animFrames.clear();
}

void SceneActor::setAnimationFrames(const std::vector<Texture2D>& frames, float duration, bool loop) {
    animFrames = frames;
    frameCount = (int)animFrames.size() > 0 ? (int)animFrames.size() : 1;
    frameDuration = duration;
    animLoop = loop;
    currentFrame = 0;
    frameTimer = 0.0f;
    animPlaying = true;
    animating = true;
    animIsFrameList = true;
}

void SceneActor::clearAnimation() {
    animating = false;
    animIsFrameList = false;
    animFrames.clear();
    currentFrame = 0;
    frameTimer = 0.0f;
    animPlaying = true;
}

void SceneActor::play() {
    animPlaying = true;
}

void SceneActor::pause() {
    animPlaying = false;
}

void SceneActor::stop() {
    animPlaying = false;
    currentFrame = 0;
    frameTimer = 0.0f;
}

void SceneActor::setFrame(int frame) {
    if (frameCount <= 0) return;
    currentFrame = (frame % frameCount + frameCount) % frameCount;
    frameTimer = 0.0f;
}

int SceneActor::getFrame() const {
    return currentFrame;
}

bool SceneActor::isAnimating() const {
    return animating;
}

bool SceneActor::isAnimationFinished() const {
    return animating && !animLoop && !animPlaying && currentFrame == frameCount - 1;
}

void SceneActor::setClickable(bool c) {
    clickable = c;
    if (!clickable) {
        hovered = false;
        pressed = false;
    }
}

bool SceneActor::isClickable() const {
    return clickable;
}

void SceneActor::setOnClick(std::function<void()> callback) {
    onClick = callback;
}

void SceneActor::setShowClickFeedback(bool show) {
    showClickFeedback = show;
}

bool SceneActor::isHovered() const {
    return hovered;
}

bool SceneActor::isPressed() const {
    return pressed;
}

void SceneActor::updateClickState(bool hoveredNow, bool pressedEdge, bool releasedEdge) {
    if (!clickable) return;

    hovered = hoveredNow;

    if (hovered && pressedEdge) {
        pressed = true;
    }

    if (releasedEdge) {
        if (pressed && hovered && onClick) {
            onClick();
        }
        pressed = false;
    }
}

float SceneActor::distanceTo(const SceneActor* other) const {
    if (!other) return 0.0f;
    Vector2 delta = {other->position.x - position.x, other->position.y - position.y};
    return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}
