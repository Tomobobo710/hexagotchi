#ifndef GOTCHI_HPP
#define GOTCHI_HPP

#include "SceneActor.hpp"
#include "GotchiMood.hpp"
#include "GotchiStats.hpp"
#include <string>
#include <vector>

// Include HexTile for HexCoords definition
#include "HexTile.hpp"

// Include Item for item-based decision making
#include "Item.hpp"

// Forward declarations
class GameState;
class HexWorld;
class EventBus;

// Gotchi class - the main pet entity
// Extends SceneActor with mood, stats, and AI behavior
// Vitals are now owned by GameState - Gotchi holds a reference
class Gotchi : public SceneActor {
public:
    // Constructor - vitals and mood are now passed from GameState (shared ownership)
    Gotchi(Vector2 position, GotchiStats& statsRef, GotchiMood& moodRef);
    ~Gotchi() = default;

    // Lifecycle
    void update(float deltaTime) override;
    void draw() override;

    // Initialization - no longer resets vitals (they persist in GameState)
    void init();

    // Stats and mood management - reference the shared vitals
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

    // State machine for autonomous behavior
    enum class GotchiState {
        IDLE,              // Default state - wandering or waiting
        PATH_TO_ITEM,      // Moving to consume an item
        CONSUME_ITEM,      // Currently consuming an item
        SLEEPING,          // Sleeping
        DEAD               // Dead
    };

    // Set the world for item detection
    void setWorld(HexWorld* world) { world_ = world; }

    // Set the event bus for emitting CareAction events
    void setEventBus(EventBus* bus) { eventBus_ = bus; }

    // State machine control
    void updateStateMachine(float deltaTime);
    void consumeItem(Item* item);
    Item* findNearestItem();
    void decideNextAction();

    // State machine helper methods
    void updateIdleState(float deltaTime);
    void updatePathToItemState(float deltaTime);
    void updateConsumeItemState(float deltaTime);

    // Item on current hex
    Item* getItemOnCurrentHex();

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
    // Core systems - references to shared vitals owned by GameState
    GotchiStats& stats_;
    GotchiMood& mood_;

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
    std::vector<Texture2D> animDie_;       // death animation
    std::vector<Texture2D> animDieTwo_;    // death animation variant 2
    std::vector<Texture2D> animDieThree_;  // death animation variant 3
    std::vector<Texture2D> animRun_;       // running animation
    std::vector<Texture2D> animArmswap_;   // armswap animation
    std::vector<Texture2D> animEyetwitch_; // eyetwitch animation
    std::vector<Texture2D> animGlitch_;    // glitch animation
    std::vector<Texture2D> animLeaking_;   // leaking animation
    std::vector<Texture2D> animLeanover_;  // leanover animation
    std::vector<Texture2D> animSpin_;      // spin animation
    std::vector<Texture2D> animWiggle_;    // wiggle animation
    std::vector<Texture2D> animWobble_;    // wobble animation

private:
    // Path-based movement state
    std::vector<HexCoords> currentPath_;
    int pathIndex_;
    bool followingPath_;

    // Hex grid configuration (set by scene)
    float hexSize_;

    // State machine state
    GotchiState currentState_;
    float stateTimer_;      // Timer for state transitions

    // Hex-coordinate targeting (replaces dangling Item* pointer)
    bool hasTarget_;        // True if there's a valid target item
    int targetQ_;           // Hex coordinate of target item
    int targetR_;           // Hex coordinate of target item

    // HexWorld reference for item detection
    HexWorld* world_;

    // Event bus for emitting CareAction events
    EventBus* eventBus_ = nullptr;
};

#endif // GOTCHI_HPP
