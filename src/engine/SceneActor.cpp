#include "SceneActor.hpp"

SceneActor::SceneActor(Vector2 pos, float w, float h)
    : position(pos), velocity({0, 0}), scale({1.0f, 1.0f}), rotation(0.0f),
      width(w), height(h), friction(ACTOR_DEFAULT_FRICTION), gravityEnabled(false),
      texture({0}), color(WHITE), active(true), visible(true), tag(""), layer(ACTOR_LAYER_MIDGROUND) {
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
}

void SceneActor::draw() {
    if (!visible) return;
    
    if (texture.id != 0) {
        DrawTextureV(texture, position, color);
    } else {
        // Draw placeholder rectangle
        DrawRectangleV(position, {width, height}, color);
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
    return Rectangle{position.x, position.y, width, height};
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
}

void SceneActor::setColor(Color col) {
    color = col;
}

Color SceneActor::getColor() const {
    return color;
}

float SceneActor::distanceTo(const SceneActor* other) const {
    if (!other) return 0.0f;
    Vector2 delta = {other->position.x - position.x, other->position.y - position.y};
    return std::sqrt(delta.x * delta.x + delta.y * delta.y);
}
