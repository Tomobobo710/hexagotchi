#ifndef SCENE_ACTOR_HPP
#define SCENE_ACTOR_HPP

#include "raylib.h"
#include <string>
#include <cmath>

// Actor constants
const float ACTOR_DEFAULT_FRICTION = 0.98f;
const float ACTOR_GRAVITY = 300.0f;
const int ACTOR_LAYER_BACKGROUND = 0;
const int ACTOR_LAYER_MIDGROUND = 1;
const int ACTOR_LAYER_FOREGROUND = 2;
const int ACTOR_LAYER_UI = 3;

class SceneActor {
public:
    // Constructor
    SceneActor(Vector2 position, float width, float height);
    virtual ~SceneActor() = default;
    
    // Lifecycle - override these in subclasses
    virtual void update(float deltaTime);
    virtual void draw();
    
    // Transform
    void setPosition(Vector2 pos);
    Vector2 getPosition() const;
    void move(Vector2 delta);
    
    void setRotation(float angle);
    float getRotation() const;
    
    // Scale
    void setScale(Vector2 scale);
    Vector2 getScale() const;
    
    // Velocity
    void setVelocity(Vector2 vel);
    Vector2 getVelocity() const;
    void addVelocity(Vector2 vel);
    
    // Size
    void setSize(float w, float h);
    float getWidth() const;
    float getHeight() const;
    
    // Bounds and collision
    Rectangle getBounds() const;
    bool isCollidingWith(const SceneActor* other) const;
    Vector2 getCenter() const;
    
    // Physics
    void setFriction(float f);
    float getFriction() const;
    void setGravityEnabled(bool enabled);
    bool isGravityEnabled() const;
    
    // State
    void setActive(bool a);
    bool isActive() const;
    
    void setVisible(bool v);
    bool isVisible() const;
    
    // Tag and layer
    void setTag(const std::string& t);
    std::string getTag() const;
    
    void setLayer(int l);
    int getLayer() const;
    
    // Texture
    void setTexture(Texture2D tex);
    void setColor(Color col);
    Color getColor() const;
    
    // Utility
    float distanceTo(const SceneActor* other) const;
    
protected:
    // Transform
    Vector2 position;
    Vector2 velocity;
    Vector2 scale;
    float rotation;
    float width, height;
    
    // Physics
    float friction;
    bool gravityEnabled;
    
    // Rendering
    Texture2D texture;
    Color color;
    
    // State
    bool active;
    bool visible;
    std::string tag;
    int layer;
};

#endif // SCENE_ACTOR_HPP
