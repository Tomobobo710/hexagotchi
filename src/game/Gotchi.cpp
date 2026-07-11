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
const float GOTCHI_TICK_RATE = 10.0f;  // Base tick rate in seconds

namespace {
    // Logical action name -> real asset-file prefix under assets/gotchis/NNN/.
    // Only names actually used anywhere in the codebase are listed (grep for
    // `setAction("` / `playClip("` across src/ if you're adding a new one).
    // A name with no entry here, or whose target prefix has no loaded
    // frames, falls back to "idle" in playClip() -- so a missing clip reads
    // as "still idling", never a blank/frozen sprite.
    const std::unordered_map<std::string, std::string>& clipAliases() {
        static const std::unordered_map<std::string, std::string> table = {
            {"idle",   "idle"},
            {"walk",   "walk"},
            {"move",   "walk"},    // legacy name for the same clip
            {"wobble", "wobble"},  // sleep-collapse hold pose
            // "eat", "play", "sleep" have no dedicated art in
            // assets/gotchis/001 (no eat_*/play_*/sleep_*.png exist) --
            // deliberately NOT aliased to a substitute clip here. playClip()
            // falls back to idle for any name that isn't in this table or
            // whose target has no frames, so care actions still show the
            // gotchi idling (not frozen/invisible) until real art exists.
        };
        return table;
    }

    // Seconds per frame, per clip -- defaults to DEFAULT_CLIP_FRAME_DURATION
    // for any prefix not listed here. Wobble runs at 4x that (25% speed),
    // per request, since 0.15s read as too fast for the sleep-collapse hold.
    constexpr float DEFAULT_CLIP_FRAME_DURATION = 0.15f;
    float clipFrameDuration(const std::string& prefix) {
        if (prefix == "wobble") return DEFAULT_CLIP_FRAME_DURATION * 4.0f;
        return DEFAULT_CLIP_FRAME_DURATION;
    }
}

Gotchi::Gotchi(Vector2 position, GotchiStats& statsRef, GotchiMood& moodRef, GameState* gameState)
    : SceneActor(position, GOTCHI_WIDTH, GOTCHI_HEIGHT),
      stats_(statsRef),
      mood_(moodRef),
      active_(true),
      sleeping_(false),
      dead_(false),
      debugMode_(false),
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
}

void Gotchi::init() {
    // NOTE: Vitals are now owned by GameState and persist across scenes.
    // We only set the mood and action here, NOT reset stats.
    // The stats_ reference points to GameState::vitals which is shared.

    // Initialize mood to a default state
    mood_.setCurrentMood(GotchiMoodType::MOOD_00_HAPPY);

    // Set initial action
    playClip("idle");
}

void Gotchi::update(float deltaTime) {
    // Skip everything except the animation clip while dead -- the death/
    // fallover clip should still play out (and hold its last frame) instead
    // of freezing on whatever frame it happened to be on the instant
    // setDead(true) was called.
    if (dead_) {
        advanceClipFrame(deltaTime);
        return;
    }

    // Update SceneActor base class (physics, etc.)
    SceneActor::update(deltaTime);

    // Advance the current animation clip's frame timer.
    advanceClipFrame(deltaTime);

    // State machine update (handles autonomous item-based behavior)
    updateStateMachine(deltaTime);

    // Path-based movement (only real, player/state-machine-driven movement --
    // wander/random-target drifting has been removed entirely: it moved the
    // gotchi and re-triggered "walk" on its own schedule with no gate for
    // "the gotchi should be holding still right now" states like the
    // sleep-collapse wobble, which it kept silently overriding).
    if (followingPath_ && !currentPath_.empty()) {
        updatePathMovement(deltaTime);
    }
}

void Gotchi::updatePathMovement(float deltaTime) {
    if (dead_ || sleeping_) return;
    if (currentPath_.empty()) { followingPath_ = false; return; }
    if (pathIndex_ >= static_cast<int>(currentPath_.size())) {
        // Path complete - snap to exact center of final hex and stop
        HexCoords finalHex = currentPath_[currentPath_.size() - 1];
        position = finalHex.toPixel(hexSize_);
        followingPath_ = false;
        playClip("idle");
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
            playClip("idle");
        }
    } else {
        position.x += (delta.x / distance) * step;
        position.y += (delta.y / distance) * step;
        setScale({ delta.x >= 0.0f ? 1.0f : -1.0f, 1.0f });  // face travel direction
        playClip("walk");
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

    const std::vector<Texture2D>* frames = activeFrames();
    const Texture2D* drawTexture = nullptr;
    Rectangle src = { 0.0f, 0.0f, 0.0f, 0.0f };

    if (frames && !frames->empty()) {
        int idx = currentFrameIndex_ % static_cast<int>(frames->size());
        if (idx < 0) idx = 0;
        drawTexture = &(*frames)[idx];
        src = { 0.0f, 0.0f, (float)drawTexture->width, (float)drawTexture->height };
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
    setFriction(sleeping ? 0.99f : 0.95f);  // Don't move while sleeping
    playClip("idle");  // no dedicated sleep art (see clipAliases()) -- idle stands in
}

bool Gotchi::isSleeping() const {
    return sleeping_;
}

void Gotchi::setDead(bool dead) {
    dead_ = dead;
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
}

void Gotchi::clean() {
    if (dead_) return;

    stats_.setStat(SecondaryStat::CLEANLINESS, 100.0f);
    stats_.addStat(EmotionalStat::SATISFACTION, 10.0f);

    mood_.addMoodOverlay(GotchiMoodType::MOOD_13_CALM, 5.0f);
    mood_.addMoodOverlay(GotchiMoodType::MOOD_14_PEACEFUL, 8.0f);

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

    if (debugMode_) {
        std::cout << "[Gotchi] Healed!\n";
    }
}

void Gotchi::sleep() {
    if (dead_) return;

    setSleeping(true);

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

    if (debugMode_) {
        std::cout << "[Gotchi] Woke up!\n";
    }
}

const std::vector<Texture2D>* Gotchi::activeFrames() const {
    auto it = clips_.find(currentClip_);
    if (it == clips_.end() || it->second.empty()) return nullptr;
    return &it->second;
}

void Gotchi::playClip(const std::string& logicalName, bool loop) {
    // Resolve the logical name to an asset prefix, falling back to idle if
    // there's no alias or the aliased clip has no loaded frames -- see
    // clipAliases() for the whole table and why some names intentionally
    // aren't in it.
    std::string prefix = logicalName;
    auto aliasIt = clipAliases().find(logicalName);
    if (aliasIt != clipAliases().end()) {
        prefix = aliasIt->second;
    }

    auto clipIt = clips_.find(prefix);
    if (clipIt == clips_.end() || clipIt->second.empty()) {
        prefix = "idle";
    }

    if (currentClip_ == prefix) return;  // already playing this clip -- don't restart it

    currentClip_ = prefix;
    currentFrameIndex_ = 0;
    frameTimer_ = 0.0f;
    currentClipLoops_ = loop;
    currentClipFrameDuration_ = clipFrameDuration(prefix);
}

void Gotchi::advanceClipFrame(float deltaTime) {
    const std::vector<Texture2D>* frames = activeFrames();
    if (!frames || frames->size() <= 1) return;

    frameTimer_ += deltaTime;
    while (frameTimer_ >= currentClipFrameDuration_) {
        frameTimer_ -= currentClipFrameDuration_;
        int next = currentFrameIndex_ + 1;
        if (next >= (int)frames->size()) {
            if (currentClipLoops_) {
                currentFrameIndex_ = 0;
            } else {
                currentFrameIndex_ = (int)frames->size() - 1;
                if (onClipFinished_) onClipFinished_();
                break;
            }
        } else {
            currentFrameIndex_ = next;
        }
    }
}

bool Gotchi::loadAnimationFrames(const std::string& basePath) {
    // Properly UnloadTexture()s any frames from a prior load -- see
    // unloadAnimations() for why that matters.
    unloadAnimations();

    // Load every real asset prefix that exists under basePath. This is the
    // ONE place that decides what clips exist; playClip() only ever looks
    // things up here, it never has its own per-action loading logic.
    static const char* PREFIXES[] = {
        "idle", "walk", "wobble", "run", "wiggle", "spin", "blink", "flash",
        "hurt", "glitch", "armswap", "eyetwitch", "leaking", "leanover",
        "stepping", "die", "fallover", "downdie",
    };
    for (const char* prefix : PREFIXES) {
        auto frames = AssetPack::loadFrames(basePath, prefix);
        if (!frames.empty()) {
            clips_[prefix] = std::move(frames);
        }
    }

    currentClip_ = "idle";
    currentFrameIndex_ = 0;
    frameTimer_ = 0.0f;

    return clips_.count("idle") != 0;
}

void Gotchi::unloadAnimations() {
    // Every loadAnimationFrames() call issues a fresh GPU texture per PNG
    // (~130 files for one gotchi). GotchiScene::init() reconstructs a
    // brand-new Gotchi (and reloads all its frames) every single time the
    // scene is re-entered, so this MUST actually UnloadTexture() everything
    // -- not just clear the map -- or each round-trip through another scene
    // piles up another full set of leaked GPU textures on top of the last.
    for (auto& entry : clips_) {
        for (Texture2D& tex : entry.second) {
            if (tex.id != 0) UnloadTexture(tex);
        }
    }
    clips_.clear();
}

Gotchi::~Gotchi() {
    unloadAnimations();
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

void Gotchi::setHexPosition(int q, int r) {
    // Convert hex coordinates to pixel position
    HexCoords hexCoords(q, r);
    position = hexCoords.toPixel(hexSize_);
    // Clear any active path to prevent conflicts
    currentPath_.clear();
    pathIndex_ = 0;
    followingPath_ = false;
    playClip("idle");
}

void Gotchi::setPath(const std::vector<HexCoords>& path) {
    currentPath_ = path;
    pathIndex_ = 0;
    followingPath_ = true;

    // Start with walk animation
    playClip("walk");
}

void Gotchi::setDebugMode(bool debug) {
    debugMode_ = debug;
}

bool Gotchi::isDebugMode() const {
    return debugMode_;
}

size_t Gotchi::animIdleCount() const {
    auto it = clips_.find("idle");
    return it == clips_.end() ? 0 : it->second.size();
}

size_t Gotchi::animWalkCount() const {
    auto it = clips_.find("walk");
    return it == clips_.end() ? 0 : it->second.size();
}

size_t Gotchi::animMoveCount() const {
    return animWalkCount();  // "move" was always an alias for "walk"
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
    // Return size of the idle clip's first frame if available.
    auto it = clips_.find("idle");
    if (it != clips_.end() && !it->second.empty()) {
        return { (float)it->second[0].width, (float)it->second[0].height };
    }
    // Fall back to any loaded clip.
    for (const auto& entry : clips_) {
        if (!entry.second.empty()) {
            return { (float)entry.second[0].width, (float)entry.second[0].height };
        }
    }
    // Default to base size if no frames loaded
    return { 64.0f, 64.0f };
}
