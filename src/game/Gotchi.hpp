#ifndef GOTCHI_HPP
#define GOTCHI_HPP

#include "SceneActor.hpp"
#include "GotchiMood.hpp"
#include "GotchiStats.hpp"
#include <string>
#include <vector>

// Include HexTile for HexCoords definition
#include "HexTile.hpp"

// Gotchi class - the main pet entity
// Extends SceneActor with mood, stats, and AI behavior
class Gotchi : public SceneActor {
public:
    // Constructor
    Gotchi(Vector2 position);
    ~Gotchi() = default;

    // Lifecycle
    void update(float deltaTime) override;
    void draw() override;

    // Initialization
    void init();

    // Stats and mood management
    GotchiStats& getStats() { return stats_; }
    const GotchiStats& getStats() const { return stats_; }

    GotchiMood& getMood() { return mood_; }
    const GotchiMood& getMood() const { return mood_; }

    // Gotchi state
    void setActive(bool a);
    bool isActive() const;

    void setSleeping(bool sleeping);
    bool isSleeping() const;

    void setDead(bool dead);
    bool isDead() const;

    // Interaction
    void interact();  // Player interaction
    void feed();      // Give food
    void play();      // Play with gotchi
    void clean();     // Clean the gotchi
    void heal();      // Heal the gotchi
    void sleep();     // Put to sleep
    void wake();      // Wake up

    // Movement (to be implemented)
    void setTargetPosition(Vector2 target);
    void moveToTarget(float deltaTime);
    void wander(float deltaTime);

    // Path-based movement
    HexCoords getCurrentHex() const;
    void setPath(const std::vector<HexCoords>& path);
    void updatePathMovement(float deltaTime);

    // Hex grid configuration (set by scene)
    void setHexSize(float size) { hexSize_ = size; }
    float getHexSize() const { return hexSize_; }

    // Animation control
    void setAction(const std::string& action);
    void updateAnimation(float deltaTime);

    // Texture loading helpers - public for scene initialization
    bool loadAnimationFrames(const std::string& basePath);
    void unloadAnimations();

    // Animation frame count accessors (for debug/probe)
    size_t animIdleCount() const { return animIdle_.size(); }
    size_t animWalkCount() const { return animWalk_.size(); }
    size_t animMoveCount() const { return animMove_.size(); }

    // Tick-based updates (private)
    void updateStats(float ticks);

    // Serialization (for save/load)
    std::string serialize() const;
    void deserialize(const std::string& data);

    // Debug
    void setDebugMode(bool debug);
    bool isDebugMode() const;

private:
    // Core systems
    GotchiStats stats_;
    GotchiMood mood_;

    // State
    bool active_;
    bool sleeping_;
    bool dead_;
    bool debugMode_;

    // Timing
    float tickTimer_;       // For tick-based updates
    float lastUpdate_;      // Last update time

    // Movement
    Vector2 targetPosition_;
    float wanderTimer_;

    // Animation
    std::string currentAction_;
    float actionTimer_;

    // Animation frames (loaded at init)
    // Note: Only animations that exist in assets are loaded
    // Missing animations are mapped to available ones in setAction()
    std::vector<Texture2D> animIdle_;
    std::vector<Texture2D> animMove_;      // mapped to walk
    std::vector<Texture2D> animEat_;       // mapped to bounce
    std::vector<Texture2D> animSleep_;
    std::vector<Texture2D> animPlay_;      // mapped to bounce
    std::vector<Texture2D> animSad_;       // mapped to hurt
    std::vector<Texture2D> animHappy_;     // mapped to idle/bounce
    std::vector<Texture2D> animBounce_;    // available animation
    std::vector<Texture2D> animHurt_;      // available animation
    std::vector<Texture2D> animWalk_;      // available animation

private:
    // Path-based movement state
    std::vector<HexCoords> currentPath_;
    int pathIndex_;
    bool followingPath_;

    // Hex grid configuration (set by scene)
    float hexSize_;
};

#endif // GOTCHI_HPP
