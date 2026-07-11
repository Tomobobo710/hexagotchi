#define _USE_MATH_DEFINES
#include "Gotchi.hpp"
#include "AssetPack.hpp"
#include "HexPathFinder.hpp"
#include "Item.hpp"
#include "EventBus.h"
#include "GameState.h"
#include <cmath>
#include <iostream>
#include <limits>

// Gotchi constants
const float GOTCHI_WIDTH = 64.0f;
const float GOTCHI_HEIGHT = 64.0f;
const float GOTCHI_MOVE_SPEED = 50.0f;
const float GOTCHI_WANDER_SPEED = 20.0f;
const float GOTCHI_TICK_RATE = 10.0f;  // Base tick rate in seconds


Gotchi::Gotchi(Vector2 position, GotchiStats& statsRef, GotchiMood& moodRef, GameState* gameState)
    : SceneActor(position, GOTCHI_WIDTH, GOTCHI_HEIGHT),
      stats_(statsRef),
      mood_(moodRef),
      active_(true),
      sleeping_(false),
      dead_(false),
      debugMode_(false),
      wanderEnabled_(true),  // Default to enabled for compatibility
      wanderTimer_(0.0f),
      currentAction_("idle"),
      actionTimer_(0.0f),
      hexSize_(64.0f),  // Default hex size, can be set by scene
      pathIndex_(0),
      followingPath_(false),
      currentState_(GotchiState::IDLE),
      stateTimer_(0.0f),
      hasTarget_(false),
      targetQ_(0),
      targetR_(0),
      world_(nullptr),
      gameState_(gameState) {
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
    animDie_.clear();
    animDieTwo_.clear();
    animDieThree_.clear();
    animBlink_.clear();
    animFlash_.clear();
    animStepping_.clear();
    animRun_.clear();
    animArmswap_.clear();
    animEyetwitch_.clear();
    animGlitch_.clear();
    animLeaking_.clear();
    animLeanover_.clear();
    animSpin_.clear();
    animWiggle_.clear();
    animWobble_.clear();
}


void Gotchi::init() {
    // NOTE: Vitals are now owned by GameState and persist across scenes.
    // We only set the mood and action here, NOT reset stats.
    // The stats_ reference points to GameState::vitals which is shared.

    // Initialize mood to a default state
    mood_.setCurrentMood(GotchiMoodType::MOOD_00_HAPPY);

    // Set initial action
    setAction("idle");

}

void Gotchi::update(float deltaTime) {
    // Skip updates if dead
    if (dead_) {
        return;
    }

    TraceLog(LOG_DEBUG, "GOTCHI_UPDATE following=%d idx=%d pos=(%.1f,%.1f)",
             followingPath_ ? 1 : 0, pathIndex_, position.x, position.y);

    // Update SceneActor base class (physics, etc.)
    SceneActor::update(deltaTime);

    // Vitals/mood ticking now lives centrally in GotchiSim (GameState is the
    // single source of truth); this local loop only drives the action timer.
    actionTimer_ -= deltaTime;
    if (actionTimer_ <= 0) {
        // Action complete, return to idle
        if (!sleeping_ && currentAction_ != "idle") {
            setAction("idle");
        }
    }

    // Animation update
    updateAnimation(deltaTime);

    // Movement logic - only if wander is enabled
    if (!sleeping_ && currentAction_ == "idle" && wanderEnabled_) {
        // Random wandering
        wanderTimer_ -= deltaTime;
        if (wanderTimer_ <= 0) {
            wander(deltaTime);
            wanderTimer_ = 2.0f + (rand() % 3) * 0.5f;  // 2-3.5 seconds
        }
    }

    // State machine update (handles autonomous item-based behavior)
    updateStateMachine(deltaTime);

    // Path-based movement (higher priority than wander/target)
    if (followingPath_ && !currentPath_.empty()) {
        updatePathMovement(deltaTime);
    }

    // Move towards target if set (fallback for non-path movement)
    if (targetPosition_.x != 0 || targetPosition_.y != 0) {
        moveToTarget(deltaTime);
    }
}

void Gotchi::updatePathMovement(float deltaTime) {
    TraceLog(LOG_DEBUG,
        "GOTCHI_UPDATE following=%d idx=%d pos=(%.1f,%.1f) dead=%d sleeping=%d pathN=%d",
        followingPath_ ? 1 : 0, pathIndex_, position.x, position.y,
        dead_ ? 1 : 0, sleeping_ ? 1 : 0, (int)currentPath_.size());

    if (dead_ || sleeping_) return;
    if (currentPath_.empty()) { followingPath_ = false; return; }
    if (pathIndex_ >= static_cast<int>(currentPath_.size())) {
        // Path complete - snap to exact center of final hex and stop
        HexCoords finalHex = currentPath_[currentPath_.size() - 1];
        position = finalHex.toPixel(hexSize_);
        followingPath_ = false;
        setAction("idle");
        velocity = {0, 0};
        return;
    }

    Vector2 targetPos = currentPath_[pathIndex_].toPixel(hexSize_);
    Vector2 delta = { targetPos.x - position.x, targetPos.y - position.y };
    float distance = std::sqrt(delta.x * delta.x + delta.y * delta.y);

    float step = GOTCHI_MOVE_SPEED * 0.8f * deltaTime;  // pixels this frame

    if (distance <= step || distance <= 1.0f) {
        // Snap to exact hex center; advance to the next hex next frame.
        position = targetPos;
        pathIndex_++;
        if (pathIndex_ >= static_cast<int>(currentPath_.size())) {
            followingPath_ = false;
            setAction("idle");
        }
    } else {
        position.x += (delta.x / distance) * step;
        position.y += (delta.y / distance) * step;
        setScale({ delta.x >= 0.0f ? 1.0f : -1.0f, 1.0f });  // face travel direction
        setAction("walk");
    }
    velocity = {0, 0};  // do not fight or depend on base physics
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

    float w = width * std::fabs(scale.x);
    float h = height * std::fabs(scale.y);
    Rectangle dest = { position.x - w * 0.5f, position.y - h * 0.5f, w, h };  // centered

    const Texture2D* drawTexture = nullptr;
    Rectangle src = { 0.0f, 0.0f, 0.0f, 0.0f };   // always initialized

    if (animating && animIsFrameList) {
        if (!animFrames.empty()) {
            int idx = currentFrame % static_cast<int>(animFrames.size());  // bounds-safe
            if (idx < 0) idx = 0;
            drawTexture = &animFrames[idx];
            src = { 0.0f, 0.0f, (float)drawTexture->width, (float)drawTexture->height };
        }
    } else if (animating) {
        drawTexture = &texture;
        src = { (float)(currentFrame * frameWidth), 0.0f, (float)frameWidth, (float)frameHeight };
    } else if (texture.id != 0) {
        drawTexture = &texture;
        src = { 0.0f, 0.0f, (float)texture.width, (float)texture.height };
    }

    // Check if animFrames has valid textures
    bool validFrame = false;
    if (animating && animIsFrameList && !animFrames.empty()) {
        int idx = currentFrame % static_cast<int>(animFrames.size());
        if (idx >= 0 && idx < (int)animFrames.size()) {
            if (animFrames[idx].id != 0) {
                validFrame = true;
            }
        }
    }

    if (drawTexture && drawTexture->id != 0) {
        if (scale.x < 0.0f) src.width  = -src.width;   // horizontal flip
        if (scale.y < 0.0f) src.height = -src.height;  // (harmless; y flip unused)
        DrawTexturePro(*drawTexture, src, dest, { 0.0f, 0.0f }, 0.0f, color);
    } else {
        // No valid texture/frame yet: centered placeholder, and do NOT fall through.
        DrawRectangleV({ dest.x, dest.y }, { w, h }, color);
    }

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
    // Sync sleeping state to GameState for shared simulation
    if (gameState_) {
        gameState_->sleeping = sleeping;
    }
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

    setSleeping(true);
    setAction("sleep");

    mood_.addMoodOverlay(GotchiMoodType::MOOD_06_SLEEPY, 0.0f);  // Clear existing
    mood_.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 30.0f);

    if (debugMode_) {
        std::cout << "[Gotchi] Going to sleep...\n";
    }
}

void Gotchi::wake() {
    setSleeping(false);

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
    // Only early-return if same action AND animating with valid frames
    // This ensures idle animation starts properly after loadAnimationFrames()
    if (currentAction_ == action && animating && !animFrames.empty()) return;

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
    } else if (action == "walk") {
        // Explicit walk animation for path following
        clearAnimation();
        if (!animWalk_.empty()) {
            setAnimationFrames(animWalk_, 0.15f, true);
            play();
        } else if (!animMove_.empty()) {
            // Fallback to move animation
            setAnimationFrames(animMove_, 0.15f, true);
            play();
        }
    } else if (action == "eat") {
        clearAnimation();
        if (!animEat_.empty()) {
            setAnimationFrames(animEat_, 0.15f, false);
            play();
        } else if (!animBounce_.empty()) {
            setAnimationFrames(animBounce_, 0.15f, false);
            play();
        } else if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.2f, true);   // keep the gotchi visible while "eating"
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
        // Map 'play' to available animation (bounce doesn't exist, use idle as fallback)
        clearAnimation();
        if (!animPlay_.empty()) {
            setAnimationFrames(animPlay_, 0.1f, false);
            play();
        } else if (!animBounce_.empty()) {
            setAnimationFrames(animBounce_, 0.1f, false);
            play();
        } else if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.15f, true);
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
        } else if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.25f, true);
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
    } else if (action == "die") {
        // Death animation - try specific variants first, then fallback
        clearAnimation();
        if (!animDie_.empty()) {
            setAnimationFrames(animDie_, 0.2f, false);
            play();
        } else if (!animDieTwo_.empty()) {
            setAnimationFrames(animDieTwo_, 0.2f, false);
            play();
        } else if (!animDieThree_.empty()) {
            setAnimationFrames(animDieThree_, 0.2f, false);
            play();
        }
    } else if (action == "fallover") {
        clearAnimation();
        if (!animDieTwo_.empty()) {
            setAnimationFrames(animDieTwo_, 0.2f, false);
            play();
        } else if (!animDie_.empty()) {
            setAnimationFrames(animDie_, 0.2f, false);
            play();
        }
    } else if (action == "downdie") {
        clearAnimation();
        if (!animDieThree_.empty()) {
            setAnimationFrames(animDieThree_, 0.2f, false);
            play();
        } else if (!animDie_.empty()) {
            setAnimationFrames(animDie_, 0.2f, false);
            play();
        }
    } else if (action == "run") {
        clearAnimation();
        if (!animRun_.empty()) {
            setAnimationFrames(animRun_, 0.1f, true);
            play();
        } else if (!animWalk_.empty()) {
            setAnimationFrames(animWalk_, 0.1f, true);
            play();
        }
    } else if (action == "armswap") {
        clearAnimation();
        if (!animArmswap_.empty()) {
            setAnimationFrames(animArmswap_, 0.15f, false);
            play();
        }
    } else if (action == "eyetwitch") {
        clearAnimation();
        if (!animEyetwitch_.empty()) {
            setAnimationFrames(animEyetwitch_, 0.1f, false);
            play();
        }
    } else if (action == "glitch") {
        clearAnimation();
        if (!animGlitch_.empty()) {
            setAnimationFrames(animGlitch_, 0.1f, false);
            play();
        }
    } else if (action == "leaking") {
        clearAnimation();
        if (!animLeaking_.empty()) {
            setAnimationFrames(animLeaking_, 0.15f, true);
            play();
        }
    } else if (action == "leanover") {
        clearAnimation();
        if (!animLeanover_.empty()) {
            setAnimationFrames(animLeanover_, 0.15f, true);
            play();
        }
    } else if (action == "spin") {
        clearAnimation();
        if (!animSpin_.empty()) {
            setAnimationFrames(animSpin_, 0.15f, true);
            play();
        }
    } else if (action == "wiggle") {
        clearAnimation();
        if (!animWiggle_.empty()) {
            setAnimationFrames(animWiggle_, 0.15f, false);
            play();
        }
    } else if (action == "wobble") {
        clearAnimation();
        if (!animWobble_.empty()) {
            setAnimationFrames(animWobble_, 0.15f, false);
            play();
        }
    } else if (action == "attack") {
        // Map attack to glitch or die animation (no exact attack exists)
        clearAnimation();
        if (!animGlitch_.empty()) {
            setAnimationFrames(animGlitch_, 0.1f, false);
            play();
        } else if (!animDie_.empty()) {
            setAnimationFrames(animDie_, 0.15f, false);
            play();
        } else if (!animHurt_.empty()) {
            setAnimationFrames(animHurt_, 0.15f, false);
            play();
        }
    } else if (action == "blink") {
        // Map blink to blink animation (from idle_two)
        clearAnimation();
        if (!animBlink_.empty()) {
            setAnimationFrames(animBlink_, 0.2f, true);
            play();
        } else if (!animEyetwitch_.empty()) {
            setAnimationFrames(animEyetwitch_, 0.1f, false);
            play();
        } else if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.2f, true);
            play();
        }
    } else if (action == "flash") {
        // Map flash to flash animation (from hurt_two)
        clearAnimation();
        if (!animFlash_.empty()) {
            setAnimationFrames(animFlash_, 0.25f, true);
            play();
        } else if (!animHurt_.empty()) {
            setAnimationFrames(animHurt_, 0.25f, true);
            play();
        }
    } else if (action == "stepping") {
        // Map stepping to stepping animation (from walk_three)
        clearAnimation();
        if (!animStepping_.empty()) {
            setAnimationFrames(animStepping_, 0.15f, true);
            play();
        } else if (!animWalk_.empty()) {
            setAnimationFrames(animWalk_, 0.15f, true);
            play();
        }
    } else if (action == "bounce") {
        // Map bounce to idle animation (no exact bounce exists)
        clearAnimation();
        if (!animBounce_.empty()) {
            setAnimationFrames(animBounce_, 0.15f, false);
            play();
        } else if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.15f, false);
            play();
        }
    } else if (action == "sleep") {
        // Fallback to idle if sleep not available
        clearAnimation();
        if (!animSleep_.empty()) {
            setAnimationFrames(animSleep_, 0.3f, true);
            play();
        } else if (!animIdle_.empty()) {
            setAnimationFrames(animIdle_, 0.3f, true);
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
    animBounce_.clear();
    animHurt_.clear();
    animWalk_.clear();
    animDie_.clear();
    animDieTwo_.clear();
    animDieThree_.clear();
    animBlink_.clear();
    animFlash_.clear();
    animStepping_.clear();
    animRun_.clear();
    animArmswap_.clear();
    animEyetwitch_.clear();
    animGlitch_.clear();
    animLeaking_.clear();
    animLeanover_.clear();
    animSpin_.clear();
    animWiggle_.clear();
    animWobble_.clear();

    // Load animation frames using AssetPack
    // Expected format: basePath_action_00.png, basePath_action_01.png, etc.
    // Returns true if at least idle frames were loaded

    animIdle_ = AssetPack::loadFrames(basePath, "idle");
    animMove_ = AssetPack::loadFrames(basePath, "walk");  // Map move to walk
    animEat_ = AssetPack::loadFrames(basePath, "bounce");  // Map eat to bounce (doesn't exist, returns empty)
    animSleep_ = AssetPack::loadFrames(basePath, "sleep"); // Doesn't exist, returns empty
    animPlay_ = AssetPack::loadFrames(basePath, "bounce"); // Map play to bounce (doesn't exist, returns empty)
    animSad_ = AssetPack::loadFrames(basePath, "hurt");
    animHappy_ = AssetPack::loadFrames(basePath, "happy"); // Doesn't exist, returns empty
    animBounce_ = AssetPack::loadFrames(basePath, "bounce"); // Doesn't exist, returns empty
    animHurt_ = AssetPack::loadFrames(basePath, "hurt");
    animWalk_ = AssetPack::loadFrames(basePath, "walk");
    animDie_ = AssetPack::loadFrames(basePath, "die");
    animDieTwo_ = AssetPack::loadFrames(basePath, "fallover");
    animDieThree_ = AssetPack::loadFrames(basePath, "downdie");
    animBlink_ = AssetPack::loadFrames(basePath, "blink");
    animFlash_ = AssetPack::loadFrames(basePath, "flash");
    animStepping_ = AssetPack::loadFrames(basePath, "stepping");
    animRun_ = AssetPack::loadFrames(basePath, "run");
    animArmswap_ = AssetPack::loadFrames(basePath, "armswap");
    animEyetwitch_ = AssetPack::loadFrames(basePath, "eyetwitch");
    animGlitch_ = AssetPack::loadFrames(basePath, "glitch");
    animLeaking_ = AssetPack::loadFrames(basePath, "leaking");
    animLeanover_ = AssetPack::loadFrames(basePath, "leanover");
    animSpin_ = AssetPack::loadFrames(basePath, "spin");
    animWiggle_ = AssetPack::loadFrames(basePath, "wiggle");
    animWobble_ = AssetPack::loadFrames(basePath, "wobble");


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
    animDie_.clear();
    animDieTwo_.clear();
    animDieThree_.clear();
    animBlink_.clear();
    animFlash_.clear();
    animStepping_.clear();
    animRun_.clear();
    animArmswap_.clear();
    animEyetwitch_.clear();
    animGlitch_.clear();
    animLeaking_.clear();
    animLeanover_.clear();
    animSpin_.clear();
    animWiggle_.clear();
    animWobble_.clear();
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

HexCoords Gotchi::getCurrentHex() const {
    // Use the canonical inverse from HexCoords
    return HexCoords::fromPixel(position, hexSize_);
}

void Gotchi::setPath(const std::vector<HexCoords>& path) {
    currentPath_ = path;
    pathIndex_ = 0;
    followingPath_ = true;

    // Start with walk animation
    setAction("walk");
}

void Gotchi::setWanderEnabled(bool enabled) {
    wanderEnabled_ = enabled;
    // Reset velocity when disabling wander to prevent residual movement
    if (!enabled) {
        velocity = {0.0f, 0.0f};
    }
}

void Gotchi::setDebugMode(bool debug) {
    debugMode_ = debug;
}

bool Gotchi::isDebugMode() const {
    return debugMode_;
}

// ============================================================================
// State Machine Implementation
// ============================================================================

void Gotchi::updateStateMachine(float deltaTime) {
    // Skip state machine if dead or sleeping
    if (dead_ || sleeping_) {
        if (currentState_ != GotchiState::SLEEPING && currentState_ != GotchiState::DEAD) {
            currentState_ = sleeping_ ? GotchiState::SLEEPING : GotchiState::DEAD;
        }
        return;
    }

    switch (currentState_) {
        case GotchiState::IDLE:
            updateIdleState(deltaTime);
            break;
        case GotchiState::PATH_TO_ITEM:
            updatePathToItemState(deltaTime);
            break;
        case GotchiState::CONSUME_ITEM:
            updateConsumeItemState(deltaTime);
            break;
        default:
            break;
    }
}

void Gotchi::updateIdleState(float deltaTime) {
    stateTimer_ += deltaTime;

    // Check for nearby items every tick
    if (world_) {
        Item* nearest = findNearestItem();
        if (nearest) {
            // Found an item! Path to it and consume it
            // Store hex coordinates instead of Item* to avoid dangling pointer
            targetQ_ = nearest->hexQ;
            targetR_ = nearest->hexR;
            hasTarget_ = true;
            HexCoords targetHex(targetQ_, targetR_);
            HexCoords currentHex = getCurrentHex();

            if (currentHex.q == targetHex.q && currentHex.r == targetHex.r) {
                // Already on the item hex - consume immediately
                currentState_ = GotchiState::CONSUME_ITEM;
                stateTimer_ = 0.0f;
            } else {
                // Path to the item
                HexPathFinder pathfinder(world_);
                std::vector<HexCoords> path = pathfinder.findPath(
                    currentHex.q, currentHex.r,
                    targetHex.q, targetHex.r
                );

                if (!path.empty()) {
                    setPath(path);
                    currentState_ = GotchiState::PATH_TO_ITEM;
                    stateTimer_ = 0.0f;
                    if (debugMode_) {
                        std::cout << "[Gotchi] Found item, path length: " << path.size() << "\n";
                    }
                }
            }
        } else {
            // No target found - clear any stale target
            hasTarget_ = false;
        }
    }
}

void Gotchi::updatePathToItemState(float deltaTime) {
    // Re-resolve item each frame using hex coordinates (avoids dangling pointer)
    Item* currentTarget = world_ ? world_->getItemAt(targetQ_, targetR_) : nullptr;

    // Check if we've reached the target
    if (followingPath_ && !currentPath_.empty()) {
        // Check if we're at the last hex of the path (the item's hex)
        HexCoords currentHex = getCurrentHex();
        if (currentHex.q == targetQ_ && currentHex.r == targetR_) {
            // Reached the item hex
            currentState_ = GotchiState::CONSUME_ITEM;
            stateTimer_ = 0.0f;
            if (debugMode_) {
                std::cout << "[Gotchi] Reached item hex, ready to consume\n";
            }
        }
    } else if (!currentTarget) {
        // Target item was consumed by another gotchi or removed
        // Return to idle without consuming
        currentState_ = GotchiState::IDLE;
        stateTimer_ = 0.0f;
        hasTarget_ = false;
    } else {
        // Path following ended unexpectedly (e.g., obstacle) - go back to idle
        currentState_ = GotchiState::IDLE;
        stateTimer_ = 0.0f;
        hasTarget_ = false;
    }
}

void Gotchi::updateConsumeItemState(float deltaTime) {
    stateTimer_ += deltaTime;

    // Consuming takes a short time (1 second)
    if (stateTimer_ >= 1.0f) {
        // Re-resolve item using hex coordinates
        Item* currentTarget = world_ ? world_->getItemAt(targetQ_, targetR_) : nullptr;

        if (currentTarget) {
            consumeItem(currentTarget);
            hasTarget_ = false;
        } else {
            // Target was consumed by someone else - nothing to do
            if (debugMode_) {
                std::cout << "[Gotchi] Target item no longer exists\n";
            }
        }
        currentState_ = GotchiState::IDLE;
        stateTimer_ = 0.0f;
    }
}

void Gotchi::consumeItem(Item* item) {
    if (!item || item->consumed) return;

    item->consumed = true;
    if (debugMode_) {
        std::cout << "[Gotchi] Consuming item: " << item->getStatName() << " value=" << item->value << "\n";
    }

    // Route through the shared care action path - call the same gotchi methods
    // used by the care buttons, which modify stats and emit CareAction
    switch (item->type) {
        case ItemType::FOOD:
            feed();
            break;
        case ItemType::WATER:
            // Water reduces hydration (thirst) - similar to feed for hunger
            // Call feed() as the shared path (both reduce their respective needs)
            feed();
            break;
        case ItemType::MEDICINE:
            // Medicine heals health - similar to the heal button effect
            heal();
            break;
        case ItemType::TOY:
        case ItemType::HAPPINESS:
            // Toys and happiness boost - similar to pet interaction
            interact();
            break;
        case ItemType::CLEANING:
            // Cleaning improves hygiene - similar to the clean button
            clean();
            break;
        case ItemType::SLEEP:
            // Sleep reduces sleep debt - similar to water for hydration
            // Use feed as the shared path
            feed();
            break;
        case ItemType::ENERGY:
            // Energy boost - similar to pet/affection boost
            interact();
            break;
        default:
            // Default to feed
            feed();
            break;
    }

    // Set action for consumption animation
    setAction("eat");
    actionTimer_ = 1.5f;

    if (debugMode_) {
        std::cout << "[Gotchi] Item consumed.\n";
    }
}

Item* Gotchi::findNearestItem() {
    if (!world_) return nullptr;

    HexCoords currentHex = getCurrentHex();
    Item* nearest = nullptr;
    float nearestDist = std::numeric_limits<float>::max();

    const std::vector<Item>& items = world_->getItems();
    for (const auto& item : items) {
        if (item.consumed) continue;

        HexCoords itemHex(item.hexQ, item.hexR);
        // Manhattan distance
        int dist = std::abs(itemHex.q - currentHex.q) + std::abs(itemHex.r - currentHex.r);

        // Only consider items within a reasonable range (10 hexes)
        if (dist < 10 && dist < nearestDist) {
            nearest = const_cast<Item*>(&item);
            nearestDist = dist;
        }
    }

    return nearest;
}

void Gotchi::decideNextAction() {
    // This is called during idle state to decide what to do next
    // For now, it just calls findNearestItem which is already done in updateIdleState
    // This can be expanded later for more complex decision making
}

Item* Gotchi::getItemOnCurrentHex() {
    if (!world_) return nullptr;

    HexCoords currentHex = getCurrentHex();
    return world_->getItemAt(currentHex.q, currentHex.r);
}

Vector2 Gotchi::getFrameSize() const {
    // Return size of first idle frame if available (idle has highest priority)
    if (!animIdle_.empty()) {
        return { (float)animIdle_[0].width, (float)animIdle_[0].height };
    }
    // Fall back to any loaded animation
    if (!animWalk_.empty()) {
        return { (float)animWalk_[0].width, (float)animWalk_[0].height };
    }
    if (!animMove_.empty()) {
        return { (float)animMove_[0].width, (float)animMove_[0].height };
    }
    // Default to base size if no frames loaded
    return { 64.0f, 64.0f };
}
