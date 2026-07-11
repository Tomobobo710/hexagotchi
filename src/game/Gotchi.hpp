#ifndef GOTCHI_HPP
#define GOTCHI_HPP

#include "SceneActor.hpp"
#include "GotchiMood.hpp"
#include "GotchiStats.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>

// Include HexTile for HexCoords definition
#include "HexTile.hpp"

// Include Item for item-based decision making
#include "Item.hpp"

// Include GameState for statsFrozen access (must be after GotchiStats/GotchiMood includes)
#include "GameState.h"

// Forward declarations
class HexWorld;
class EventBus;

// Gotchi class - the main pet entity
// Extends SceneActor with mood, stats, and AI behavior
// Vitals are now owned by GameState - Gotchi holds a reference
class Gotchi : public SceneActor {
public:
    // Constructor - vitals and mood are now passed from GameState (shared ownership)
    // GameState is passed for sleeping state synchronization
    Gotchi(Vector2 position, GotchiStats& statsRef, GotchiMood& moodRef, GameState* gameState = nullptr);
    ~Gotchi();

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

    // Set the shared GameState for synchronization
    void setGameState(GameState* state) { gameState_ = state; }


    // Interaction
    void interact();  // Player interaction
    void feed();      // Give food
    void play();      // Play with gotchi
    void clean();     // Clean the gotchi
    void heal();      // Heal the gotchi
    void sleep();     // Put to sleep
    void wake();      // Wake up

    // Freezes vital-stat drain/gain and mood updates while true (tutorial
    // hand-holding: the player shouldn't see hunger/energy/etc moving while
    // they're still being taught what the buttons do). Animation, movement,
    // and the state machine keep running as normal. Vitals/mood ticking now
    // lives in GotchiSim, so this just forwards to the shared GameState flag
    // it reads every frame.
    void setStatsFrozen(bool frozen) { if (gameState_) gameState_->statsFrozen = frozen; }
    bool isStatsFrozen() const { return gameState_ && gameState_->statsFrozen; }

    // Path-based movement
    HexCoords getCurrentHex() const;
    void setPath(const std::vector<HexCoords>& path);
    void updatePathMovement(float deltaTime);
    bool isFollowingPath() const { return followingPath_; }

    // Set gotchi's position from hex coordinates
    void setHexPosition(int q, int r);

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

    // ------------------------------------------------------------------
    // Animation system
    // ------------------------------------------------------------------
    // One clip is "current" at a time; playClip() switches the clip and
    // holds it until something else calls playClip() again -- there is no
    // separate timer that silently reverts to idle behind the scenes. A
    // clip that should return to idle on its own (e.g. a one-shot "eat"
    // animation) does so explicitly via onClipFinished, not via a
    // fire-and-forget timer a caller has to remember to set.
    //
    // Loading: loadAnimationFrames(basePath) populates clips_ from
    // whatever prefixes actually exist under basePath (see
    // ANIMATION_ALIASES below for the logical-name -> asset-prefix map).
    // Logical names with no matching art fall back to "idle" so a missing
    // clip reads as "still idling", not "frozen on a blank frame".
    void playClip(const std::string& logicalName, bool loop = true);
    const std::string& currentClip() const { return currentClip_; }

    // Fires once, the frame a non-looping clip reaches its last frame.
    // Scenes/Gotchi's own state machine can react to this instead of a
    // side-channel actionTimer_ (e.g. return to idle after eating).
    void setOnClipFinished(std::function<void()> cb) { onClipFinished_ = cb; }

    // Texture loading - public for scene initialization
    bool loadAnimationFrames(const std::string& basePath);
    void unloadAnimations();

    // Frame size accessor (for proper sizing based on actual sprite)
    Vector2 getFrameSize() const;

    // Kept for existing debug/probe call sites.
    size_t animIdleCount() const;
    size_t animWalkCount() const;
    size_t animMoveCount() const;

    // Back-compat shim for existing callers (GotchiScene/HexViewScene/
    // GotchiStatsScene all call setAction("idle") etc. today) -- forwards
    // straight to playClip(). New code should call playClip() directly.
    void setAction(const std::string& action) { playClip(action); }

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

    // Shared GameState reference for synchronization
    GameState* gameState_ = nullptr;

    // ------------------------------------------------------------------
    // Animation state (see the public section above for the API)
    // ------------------------------------------------------------------
    // One map of already-loaded clips, keyed by the ASSET prefix (e.g.
    // "idle", "wobble", "walk") -- not by logical action name. Populated
    // entirely by loadAnimationFrames(); nothing else mutates it, so
    // there's exactly one place that can leak or mis-load a texture.
    std::unordered_map<std::string, std::vector<Texture2D>> clips_;

    std::string currentClip_ = "idle";  // asset prefix currently playing
    int currentFrameIndex_ = 0;
    float frameTimer_ = 0.0f;
    bool currentClipLoops_ = true;
    float currentClipFrameDuration_ = 0.15f;  // seconds per frame for the current clip -- see clipFrameDuration()
    std::function<void()> onClipFinished_;

    void advanceClipFrame(float deltaTime);
    const std::vector<Texture2D>* activeFrames() const;

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
