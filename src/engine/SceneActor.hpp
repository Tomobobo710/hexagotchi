#ifndef SCENE_ACTOR_HPP
#define SCENE_ACTOR_HPP

#include "raylib.h"
#include <string>
#include <cmath>
#include <vector>
#include <functional>

// Actor constants
const float ACTOR_DEFAULT_FRICTION = 0.98f;
const float ACTOR_GRAVITY = 300.0f;
const int ACTOR_LAYER_BACKGROUND = 0;
const int ACTOR_LAYER_MIDGROUND = 1;
const int ACTOR_LAYER_FOREGROUND = 2;
const int ACTOR_LAYER_UI = 3;

// How close (world units) position must get to a waypoint before moveTo()
// advances to the next one -- small enough to look like the actor actually
// reached the spot, large enough that a single frame's movement at typical
// scenario walk speeds doesn't overshoot and oscillate.
const float ACTOR_WAYPOINT_ARRIVAL_DIST = 2.0f;

// Clickable overlay tint colors (drawn as a tint rect over the actor's bounds)
const Color ACTOR_HOVER_TINT = {255, 255, 255, 60};
const Color ACTOR_PRESSED_TINT = {0, 0, 0, 70};

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

    // Waypoint movement: walks position in a straight line through each
    // point in `waypoints`, in order, at `speed` (world units/second),
    // advancing to the next point once within ACTOR_WAYPOINT_ARRIVAL_DIST of
    // the current one. update() advances this every frame. Calling moveTo()
    // again (e.g. from a later scenario line) replaces any in-progress move.
    // Not connected to velocity/friction/gravity -- this is a separate,
    // simpler position-only mover for scripted scenario movement, not
    // physics-driven motion.
    void moveTo(const std::vector<Vector2>& waypoints, float speed);
    bool isMoving() const;
    void stopMoving();  // Cancels any in-progress moveTo(), leaving position where it is.

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

    // Sprite-sheet animation. The texture is treated as a horizontal strip of
    // equal-size frames. Once set, draw() shows the current frame instead of
    // the whole texture; call update() each frame to advance it.
    void setAnimation(int frameWidth, int frameHeight, int frameCount,
                       float frameDuration, bool loop = true);

    // Frame-list animation: each frame is its own already-loaded Texture2D
    // (e.g. one PNG per frame, as with assets/gotchis/*/<action>_NN.png), in
    // display order. Caller owns/unloads the textures, same convention as
    // setTexture(). Use AssetPack::loadFrames() to build the vector from a
    // numbered-file naming convention packed into assets.rres.
    void setAnimationFrames(const std::vector<Texture2D>& frames,
                             float frameDuration, bool loop = true);

    void clearAnimation();  // Revert to drawing the whole texture, no animation
    void play();
    void pause();
    void stop();  // Pause and reset to frame 0
    void setFrame(int frame);
    int getFrame() const;
    bool isAnimating() const;
    bool isAnimationFinished() const;  // True once a non-looping animation completes

    // Utility
    float distanceTo(const SceneActor* other) const;

    // Clickability. When enabled, Scene::update() hit-tests this actor's
    // bounds against the mouse each frame (in world space, so it accounts for
    // camera position/zoom) and fires onClick on a press-and-release both
    // over the actor -- same semantics as Button. draw() shows a subtle tint
    // overlay on hover/press unless disabled with setShowClickFeedback(false).
    void setClickable(bool clickable);
    bool isClickable() const;
    void setOnClick(std::function<void()> callback);
    void setShowClickFeedback(bool show);
    bool isHovered() const;
    bool isPressed() const;

    // Called by Scene::update() -- not intended to be called directly.
    void updateClickState(bool hoveredNow, bool pressedEdge, bool releasedEdge);

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

    // Waypoint movement (see moveTo())
    std::vector<Vector2> waypoints;
    int waypointIndex;      // Index into waypoints of the point currently being sought, or -1 if idle.
    float waypointSpeed;
    
    // Rendering
    Texture2D texture;
    Color color;

    // Sprite-sheet animation
    int frameWidth;
    int frameHeight;
    int frameCount;
    float frameDuration;
    bool animLoop;
    int currentFrame;
    float frameTimer;
    bool animPlaying;
    bool animating;      // A sprite sheet or frame list is set (vs. a plain static texture)
    bool animIsFrameList; // True: draw from animFrames; false: slice `texture` as a strip
    std::vector<Texture2D> animFrames;

    // State
    bool active;
    bool visible;
    std::string tag;
    int layer;

    // Clickability
    bool clickable;
    bool hovered;
    bool pressed;
    bool showClickFeedback;
    std::function<void()> onClick;
};

#endif // SCENE_ACTOR_HPP
