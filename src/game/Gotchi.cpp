#define _USE_MATH_DEFINES
#include "Gotchi.hpp"
#include "AssetPack.hpp"
#include <cmath>
#include <iostream>

// Gotchi constants
const float GOTCHI_WIDTH = 64.0f;
const float GOTCHI_HEIGHT = 64.0f;
const float GOTCHI_MOVE_SPEED = 50.0f;
const float GOTCHI_WANDER_SPEED = 20.0f;
const float GOTCHI_TICK_RATE = 10.0f;  // Base tick rate in seconds

Gotchi::Gotchi(Vector2 position, GotchiStats& statsRef, GotchiMood& moodRef)
    : SceneActor(position, GOTCHI_WIDTH, GOTCHI_HEIGHT),
      stats_(statsRef),
      mood_(moodRef),
      active_(true),
      sleeping_(false),
      dead_(false),
      debugMode_(false),
      tickTimer_(0.0f),
      lastUpdate_(0.0f),
      wanderTimer_(0.0f),
      currentAction_("idle"),
      actionTimer_(0.0f) {
    // Set physics properties
    setGravityEnabled(false);
    setFriction(0.95f);
    setLayer(ACTOR_LAYER_FOREGROUND);

    // Initialize animation frames
    animIdle_.clear();
    animMove_.clear();
    animEat_.clear();
    animSleep_.clear();
    animPlay_.clear();
    animSad_.clear();
    animHappy_.clear();
    animBounce_.clear();
    animHurt_.clear();
    animWalk_.clear();
}


void Gotchi::init() {
    // NOTE: Vitals are now owned by GameState and persist across scenes.
    // We only set the mood and action here, NOT reset stats.
    // The stats_ reference points to GameState::vitals which is shared.

    // Initialize mood to a default state
    mood_.setCurrentMood(GotchiMoodType::MOOD_00_HAPPY);

    // Set initial action
    setAction("idle");

    // Debug info
    if (debugMode_) {
        std::cout << "[Gotchi] Initialized at (" << position.x << ", " << position.y << ")\n";
        std::cout << "[Gotchi] Vitals shared with GameState. Current mood: " << mood_.getMoodName() << "\n";
    }
}

void Gotchi::update(float deltaTime) {
    // Skip updates if dead
    if (dead_) {
        return;
    }

    // Update timing
    tickTimer_ += deltaTime;
    lastUpdate_ += deltaTime;

    // Update SceneActor base class (physics, etc.)
    SceneActor::update(deltaTime);

    // Tick-based updates (every GOTCHI_TICK_RATE seconds)
    if (tickTimer_ >= GOTCHI_TICK_RATE) {
        float ticks = tickTimer_ / GOTCHI_TICK_RATE;
        tickTimer_ -= (ticks * GOTCHI_TICK_RATE);

        // Update stats over time
        updateStats(ticks);

        // Update mood based on stats
        mood_.updateMood(deltaTime, stats_);

        // Process mood overlays
        mood_.processMoodOverlays(deltaTime);

        // Update action timer
        actionTimer_ -= deltaTime;
        if (actionTimer_ <= 0) {
            // Action complete, return to idle
            if (!sleeping_ && currentAction_ != "idle") {
                setAction("idle");
            }
        }

        if (debugMode_) {
            // Debug output every 5 ticks
            if (static_cast<int>(lastUpdate_) % 5 == 0) {
                std::cout << "[Gotchi] Tick " << static_cast<int>(lastUpdate_) / 5
                          << " | Mood: " << mood_.getMoodName()
                          << " | Hunger: " << stats_.getHunger()
                          << " | Energy: " << stats_.getEnergy()
                          << "\n";
            }
        }
    }

    // Animation update
    updateAnimation(deltaTime);

    // Movement logic
    if (!sleeping_ && currentAction_ == "idle") {
        // Random wandering
        wanderTimer_ -= deltaTime;
        if (wanderTimer_ <= 0) {
            wander(deltaTime);
            wanderTimer_ = 2.0f + (rand() % 3) * 0.5f;  // 2-3.5 seconds
        }
    }

    // Move towards target if set
    if (targetPosition_.x != 0 || targetPosition_.y != 0) {
        moveToTarget(deltaTime);
    }
}

void Gotchi::updateStats(float ticks) {
    // Age increases with each tick (time passing)
    stats_.addStat(SecondaryStat::AGE, ticks);

    // Core vital stats drain/gain
    stats_.addStat(SecondaryStat::FOOD_LEVEL, 5.0f * ticks);     // Hunger increases
    stats_.addStat(SecondaryStat::HYDRATION, 3.0f * ticks);     // Thirst increases
    stats_.addStat(SecondaryStat::SLEEP_DEBT, 2.0f * ticks);    // Sleep debt increases
    stats_.addStat(SecondaryStat::ENERGY, -3.0f * ticks);       // Energy drains
    stats_.addStat(SecondaryStat::CLEANLINESS, -2.0f * ticks);  // Gets dirty slowly

    // Emotional stats decay
    stats_.addStat(EmotionalStat::HAPPINESS, -1.0f * ticks);
    stats_.addStat(EmotionalStat::EXCITEMENT, -0.5f * ticks);

    // Natural recovery (slow)
    if (!sleeping_) {
        stats_.addStat(SecondaryStat::ENERGY, 0.5f * ticks);
        stats_.addStat(SecondaryStat::FITALITY, 0.2f * ticks);
    } else {
        // Faster recovery when sleeping
        stats_.addStat(SecondaryStat::ENERGY, 5.0f * ticks);
        stats_.addStat(SecondaryStat::SLEEP_DEBT, -8.0f * ticks);
        stats_.addStat(EmotionalStat::HAPPINESS, 1.0f * ticks);
    }

    // Clamp all stats to valid ranges (use the stat's built-in clamping via setStat)
    // Note: We don't use getNormalizedStat here because that returns 0-1 values
    // which would incorrectly clamp all stats to that range
    stats_.setStat(SecondaryStat::FOOD_LEVEL, stats_.getStat(SecondaryStat::FOOD_LEVEL));
    stats_.setStat(SecondaryStat::HYDRATION, stats_.getStat(SecondaryStat::HYDRATION));
    stats_.setStat(SecondaryStat::SLEEP_DEBT, stats_.getStat(SecondaryStat::SLEEP_DEBT));
    stats_.setStat(SecondaryStat::ENERGY, stats_.getStat(SecondaryStat::ENERGY));
    stats_.setStat(SecondaryStat::CLEANLINESS, stats_.getStat(SecondaryStat::CLEANLINESS));
    stats_.setStat(EmotionalStat::HAPPINESS, stats_.getStat(EmotionalStat::HAPPINESS));
    stats_.setStat(EmotionalStat::EXCITEMENT, stats_.getStat(EmotionalStat::EXCITEMENT));

    // Health impacts from unmet needs
    float healthDrain = 0.0f;
    if (stats_.getStat(SecondaryStat::FOOD_LEVEL) > 80) healthDrain += 2.0f;
    if (stats_.getStat(SecondaryStat::HYDRATION) > 80) healthDrain += 2.0f;
    if (stats_.getStat(SecondaryStat::SLEEP_DEBT) > 80) healthDrain += 1.5f;
    if (stats_.getStat(SecondaryStat::CLEANLINESS) > 70) healthDrain += 1.0f;
    if (stats_.getStat(EmotionalStat::HAPPINESS) < 20) healthDrain += 1.0f;

    stats_.addStat(SecondaryStat::FITALITY, -healthDrain * ticks);

    // Cap health at 100
    float currentHealth = stats_.getStat(SecondaryStat::FITALITY);
    if (currentHealth > 100.0f) {
        stats_.setStat(SecondaryStat::FITALITY, 100.0f);
    }

    // Check for death
    if (currentHealth <= 0.0f) {
        dead_ = true;
        setAction("idle");
        if (debugMode_) {
            std::cout << "[Gotchi] Has died!\n";
        }
    }
}

void Gotchi::draw() {
    if (!visible) return;

    // Call parent draw (handles texture and animation)
    SceneActor::draw();

    // Draw debug info if enabled
    if (debugMode_) {
        // Draw stat bars
        float barY = position.y - 20;

        // Health bar
        float healthPct = stats_.getNormalizedStat(SecondaryStat::FITALITY);
        DrawRectangleV({position.x - 30, barY}, {60 * healthPct, 4}, GREEN);
        DrawRectangleLinesEx(Rectangle{position.x - 30, barY, 60, 4}, 1, WHITE);

        // Hunger bar (red)
        float hungerPct = stats_.getNormalizedStat(SecondaryStat::FOOD_LEVEL);
        DrawRectangleV({position.x - 30, barY + 6}, {60 * (1.0f - hungerPct), 4}, RED);
        DrawRectangleLinesEx(Rectangle{position.x - 30, barY + 6, 60, 4}, 1, WHITE);

        // Energy bar (yellow)
        float energyPct = stats_.getNormalizedStat(SecondaryStat::ENERGY);
        DrawRectangleV({position.x - 30, barY + 12}, {60 * energyPct, 4}, YELLOW);
        DrawRectangleLinesEx(Rectangle{position.x - 30, barY + 12, 60, 4}, 1, WHITE);
    }
}

void Gotchi::setActive(bool a) {
    active_ = a;
    if (!active_) {
        pause();
    } else {
        play();
    }
}

bool Gotchi::isActive() const {
    return active_;
}

void Gotchi::setSleeping(bool sleeping) {
    sleeping_ = sleeping;
    if (sleeping) {
        setAction("sleep");
        setFriction(0.99f);  // Don't move while sleeping
    } else {
        setFriction(0.95f);
    }
}

bool Gotchi::isSleeping() const {
    return sleeping_;
}

void Gotchi::setDead(bool dead) {
    dead_ = dead;
    if (dead_) {
        setAction("idle");
        pause();
    }
}

bool Gotchi::isDead() const {
    return dead_;
}

void Gotchi::interact() {
    if (dead_) return;

    // Positive interaction boost
    stats_.addStat(EmotionalStat::HAPPINESS, 15.0f);
    stats_.addStat(EmotionalStat::EXCITEMENT, 20.0f);
    stats_.addStat(SocialStat::FRIENDS, 1.0f);

    // Short-term mood boost
    mood_.addMoodOverlay(GotchiMoodType::MOOD_10_JOYFUL, 5.0f);
    mood_.addMoodOverlay(GotchiMoodType::MOOD_31_PLAYFUL, 3.0f);

    // Set playful action
    setAction("play");
    actionTimer_ = 2.0f;

    if (debugMode_) {
        std::cout << "[Gotchi] Interacted! Happiness increased.\n";
    }
}

void Gotchi::feed() {
    if (dead_) return;

    // Food boosts
    stats_.addStat(SecondaryStat::FOOD_LEVEL, -30.0f);  // Reduce hunger
    stats_.addStat(SecondaryStat::ENERGY, 10.0f);
    stats_.addStat(EmotionalStat::SATISFACTION, 10.0f);

    // Mood boost
    mood_.addMoodOverlay(GotchiMoodType::MOOD_02_SATISFIED, 10.0f);
    mood_.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 5.0f);

    setAction("eat");
    actionTimer_ = 3.0f;

    if (debugMode_) {
        std::cout << "[Gotchi] Fed! Hunger reduced.\n";
    }
}

void Gotchi::play() {
    if (dead_ || sleeping_) return;

    stats_.addStat(EmotionalStat::EXCITEMENT, 25.0f);
    stats_.addStat(EmotionalStat::HAPPINESS, 20.0f);
    stats_.addStat(PhysicalStat::AGILITY, 1.0f);

    mood_.addMoodOverlay(GotchiMoodType::MOOD_01_EXCITED, 8.0f);
    mood_.addMoodOverlay(GotchiMoodType::MOOD_31_PLAYFUL, 5.0f);

    setAction("play");
    actionTimer_ = 4.0f;
}

void Gotchi::clean() {
    if (dead_) return;

    stats_.setStat(SecondaryStat::CLEANLINESS, 100.0f);
    stats_.addStat(EmotionalStat::SATISFACTION, 10.0f);

    mood_.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 5.0f);
    mood_.addMoodOverlay(GotchiMoodType::MOOD_14_PEACEFUL, 8.0f);

    setAction("idle");
    actionTimer_ = 1.0f;

    if (debugMode_) {
        std::cout << "[Gotchi] Cleaned!\n";
    }
}

void Gotchi::heal() {
    if (dead_) return;

    stats_.setStat(SecondaryStat::FITALITY, 100.0f);
    stats_.addStat(EmotionalStat::HAPPINESS, 5.0f);

    mood_.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 5.0f);
    mood_.addMoodOverlay(GotchiMoodType::MOOD_45_HEALTHY, 15.0f);

    setAction("idle");
    actionTimer_ = 1.0f;

    if (debugMode_) {
        std::cout << "[Gotchi] Healed!\n";
    }
}

void Gotchi::sleep() {
    if (dead_) return;

    sleeping_ = true;
    setAction("sleep");
    setFriction(0.99f);

    mood_.addMoodOverlay(GotchiMoodType::MOOD_06_SLEEPY, 0.0f);  // Clear existing
    mood_.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 30.0f);

    if (debugMode_) {
        std::cout << "[Gotchi] Going to sleep...\n";
    }
}

void Gotchi::wake() {
    sleeping_ = false;
    setFriction(0.95f);

    mood_.clearMoodOverlays();
    mood_.addMoodOverlay(GotchiMoodType::MOOD_00_HAPPY, 10.0f);

    setAction("idle");

    if (debugMode_) {
        std::cout << "[Gotchi] Woke up!\n";
    }
}

void Gotchi::setTargetPosition(Vector2 target) {
    targetPosition_ = target;
}

void Gotchi::moveToTarget(float deltaTime) {
    if (dead_ || sleeping_) return;

    Vector2 delta = {targetPosition_.x - position.x, targetPosition_.y - position.y};
    float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    if (distance > 5.0f) {
        // Normalize and move
        float scale = GOTCHI_MOVE_SPEED * deltaTime / distance;
        velocity.x = delta.x * scale;
        velocity.y = delta.y * scale;

        setAction("move");
        actionTimer_ = 0.0f;  // Reset for animation

        // Face direction
        if (delta.x > 0) {
            setScale({1.0f, 1.0f});
        } else {
            setScale({-1.0f, 1.0f});
        }
    } else {
        // Arrived
        targetPosition_ = {0, 0};
        velocity = {0, 0};
        setAction("idle");
    }
}

void Gotchi::wander(float deltaTime) {
    if (dead_ || sleeping_) return;

    // Random direction
    float angle = (rand() % 360) * (PI / 180.0f);
    Vector2 dir = {std::cos(angle), std::sin(angle)};

    velocity.x = dir.x * GOTCHI_WANDER_SPEED;
    velocity.y = dir.y * GOTCHI_WANDER_SPEED;

    setAction("move");
    actionTimer_ = 0.0f;

    // Set facing direction
    if (dir.x > 0) {
        setScale({1.0f, 1.0f});
    } else {
        setScale({-1.0f, 1.0f});
    }

    // Stop after wandering for a bit
    setTargetPosition({position.x + dir.x * 100, position.y + dir.y * 100});
}

void Gotchi::setAction(const std::string& action) {
    if (currentAction_ == action) return;

    currentAction_ = action;
    actionTimer_ = 0.0f;

    // Animation setup based on action
    // Maps logical actions to available animation files
    if (action == "idle") {
        clearAnimation();
        if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.2f, true);
            play();
        }
    } else if (action == "move") {
        // Map 'move' to 'walk' animation
        clearAnimation();
        if (!animMove_.empty()) {
            setAnimationFrames(animMove_, 0.1f, true);
            play();
        }
    } else if (action == "eat") {
        // Map 'eat' to 'bounce' animation (mouth movement)
        clearAnimation();
        if (!animEat_.empty()) {
            setAnimationFrames(animEat_, 0.15f, false);
            play();
        }
    } else if (action == "sleep") {
        clearAnimation();
        if (!animSleep_.empty()) {
            setAnimationFrames(animSleep_, 0.3f, true);
            play();
        } else if (!animIdle_.empty()) {
            // Fallback to idle if sleep not available
            setAnimationFrames(animIdle_, 0.3f, true);
            play();
        }
    } else if (action == "play") {
        // Map 'play' to 'bounce' animation
        clearAnimation();
        if (!animPlay_.empty()) {
            setAnimationFrames(animPlay_, 0.1f, false);
            play();
        } else if (!animBounce_.empty()) {
            setAnimationFrames(animBounce_, 0.1f, false);
            play();
        }
    } else if (action == "sad") {
        // Map 'sad' to 'hurt' animation
        clearAnimation();
        if (!animSad_.empty()) {
            setAnimationFrames(animSad_, 0.25f, true);
            play();
        } else if (!animHurt_.empty()) {
            setAnimationFrames(animHurt_, 0.25f, true);
            play();
        }
    } else if (action == "happy") {
        // Map 'happy' to 'idle' or 'bounce' animation
        clearAnimation();
        if (!animHappy_.empty()) {
            setAnimationFrames(animHappy_, 0.15f, true);
            play();
        } else if (!animBounce_.empty()) {
            setAnimationFrames(animBounce_, 0.15f, true);
            play();
        } else if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.15f, true);
            play();
        }
    }
}

void Gotchi::updateAnimation(float deltaTime) {
    // Animation is updated by SceneActor::update() when animating
    // This is called from Gotchi::update()
}

bool Gotchi::loadAnimationFrames(const std::string& basePath) {
    animIdle_.clear();
    animMove_.clear();
    animEat_.clear();
    animSleep_.clear();
    animPlay_.clear();
    animSad_.clear();
    animHappy_.clear();

    // Load animation frames using AssetPack
    // Expected format: basePath_action_00.png, basePath_action_01.png, etc.
    // Returns true if at least idle frames were loaded

    animIdle_ = AssetPack::loadFrames(basePath, "idle");
    animMove_ = AssetPack::loadFrames(basePath, "walk");  // Map move to walk
    animEat_ = AssetPack::loadFrames(basePath, "bounce");  // Map eat to bounce
    animSleep_ = AssetPack::loadFrames(basePath, "sleep");
    animPlay_ = AssetPack::loadFrames(basePath, "bounce");  // Map play to bounce
    animSad_ = AssetPack::loadFrames(basePath, "hurt");  // Map sad to hurt
    animHappy_ = AssetPack::loadFrames(basePath, "happy");
    animBounce_ = AssetPack::loadFrames(basePath, "bounce");
    animHurt_ = AssetPack::loadFrames(basePath, "hurt");
    animWalk_ = AssetPack::loadFrames(basePath, "walk");

    // Return true if at least idle frames were loaded
    return !animIdle_.empty();
}

void Gotchi::unloadAnimations() {
    animIdle_.clear();
    animMove_.clear();
    animEat_.clear();
    animSleep_.clear();
    animPlay_.clear();
    animSad_.clear();
    animHappy_.clear();
    animBounce_.clear();
    animHurt_.clear();
    animWalk_.clear();
}

std::string Gotchi::serialize() const {
    // Serialize Gotchi state for save/load
    // This would save all stats, mood, position, etc.
    return "";  // Placeholder for serialization
}

void Gotchi::deserialize(const std::string& data) {
    // Deserialize Gotchi state from save file
    // Placeholder for deserialization
}

void Gotchi::setDebugMode(bool debug) {
    debugMode_ = debug;
}

bool Gotchi::isDebugMode() const {
    return debugMode_;
}
