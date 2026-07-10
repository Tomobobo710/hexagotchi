#include "GotchiStatsScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "GotchiStats.hpp"
#include "GotchiMood.hpp"
#include "SceneManager.hpp"
#include "EventBus.h"
#include "GameState.h"
#include <iostream>
#include <sstream>
#include <iomanip>

// Global event bus (extern'd in main.cpp)
extern EventBus globalEventBus;

GotchiStatsScene::GotchiStatsScene()
    : Scene(720.0f, 720.0f, {40, 40, 60, 255}), simTime_(0.0f), frameCount_(0) {
    gotchiDir = "gotchis/001";
}

// ============================================================================
// Enum-to-string helpers
// ============================================================================

std::string GotchiStatsScene::modeToString(Mode mode) const {
    switch (mode) {
        case Mode::Gotchi:    return "Gotchi";
        case Mode::HexBoard:  return "HexBoard";
        case Mode::Story:     return "Story";
        default:              return "Unknown";
    }
}

std::string GotchiStatsScene::firstMergeToString(FirstMerge fm) const {
    switch (fm) {
        case FirstMerge::NotYet:      return "NotYet";
        case FirstMerge::Immediate:   return "Immediate";
        case FirstMerge::AfterABit:   return "AfterABit";
        case FirstMerge::AfterNeglect:return "AfterNeglect";
        default:                      return "Unknown";
    }
}

// ============================================================================
// Value visitor for flags map
// ============================================================================

struct ValuePrinter {
    std::string operator()(bool v) const { return v ? "true" : "false"; }
    std::string operator()(int v) const { return std::to_string(v); }
    std::string operator()(float v) const {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(2) << v;
        return oss.str();
    }
    std::string operator()(const std::string& v) const { return v; }
};

void GotchiStatsScene::init() {
    Scene::init();
    // Center camera
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);

    // Create Gotchi with shared vitals and mood from GameState
    GotchiStats& stats = gameState_ ? gameState_->vitals : defaultStats_;
    GotchiMood& mood = gameState_ ? gameState_->mood : defaultMood_;

    gotchi = new Gotchi({360.0f, 360.0f}, stats, mood);
    gotchi->setTag("gotchi");
    gotchi->setWanderEnabled(false);   // Stop floating in idle state
    addActor(gotchi);

    // Framing: set fixed world scale and compute camera zoom to fill ~60% of screen
    gotchi->setScale({ GOTCHI_WORLD_SCALE, GOTCHI_WORLD_SCALE });
    float spriteWorldPx = gotchi->getHeight() * GOTCHI_WORLD_SCALE;      // 64 * 2 = 128px
    float targetPx      = GOTCHI_SCREEN_FRAC * (float)GAME_H;            // 0.60 * 720 = 432px
    float framingZoom   = targetPx / spriteWorldPx;                       // 3.375
    getCamera()->setPosition(360.0f, 360.0f);                             // center on the gotchi
    getCamera()->setZoom(framingZoom);

    TraceLog(LOG_INFO, "GOTCHI_STATS_FRAME sprPx=%.0f targetPx=%.0f wantZoom=%.3f setZoom=%.3f minZoom=%.3f",
             spriteWorldPx, targetPx, framingZoom,
             getCamera()->getZoom(), getCamera()->minZoomForBounds());

    // Initialize the Gotchi (no longer resets vitals - they persist in GameState)
    gotchi->init();

    // Load animation frames using the Gotchi's loader
    // This populates the Gotchi's internal frame arrays for each action
    gotchi->loadAnimationFrames(gotchiDir);

    // Set initial action (will use the loaded frames)
    gotchi->setAction("idle");

    // Subscribe to CareAction events for the DEBUG panel
    careActionToken_ = globalEventBus.subscribe(EventType::CareAction, [this](const Event& e) {
        this->lastCareAction_ = e.ia;
        this->careActionCount_++;
    });

    // Add navigation button
    addNavigationButton("< BACK TO GOTCHI", "gotchi", (float)GAME_W / 2.0f, (float)GAME_H - 30);
}

void GotchiStatsScene::addNavigationButton(const std::string& label, const std::string& targetScene, float x, float y) {
    float buttonWidth = 200.0f;
    float buttonHeight = 40.0f;
    Button* btn = new Button({x, y}, buttonWidth, buttonHeight, label);
    btn->setAnchor("center");
    btn->setFontSize(16);
    btn->setBackgroundColor({60, 60, 100, 220});
    btn->setHoverColor({100, 100, 160, 240});
    btn->setBorderColor({150, 150, 200, 255});

    btn->setOnClick([this, targetScene]() {
        if (getSceneManager()) {
            SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
            mgr->switchScene(targetScene);
        }
    });
    buttons.push_back(std::unique_ptr<Button>(btn));
}

void GotchiStatsScene::update(float deltaTime) {
    Scene::update(deltaTime);

    // Update the Gotchi every frame
    if (gotchi) {
        gotchi->update(deltaTime);
    }

    // Mouse wheel zoom - allows zooming in/out around cursor position
    float wheel = GetMouseWheelMove();
    if (wheel != 0.0f) getCamera()->zoomAtScreen(GetMousePosition(), wheel * 0.25f);

    // Update button states
    SceneInputHandler* input = getInputHandler();
    for (auto& btn : buttons) {
        btn->update(input, deltaTime);
    }

    // Update simulation time and frame count
    simTime_ += deltaTime;
    frameCount_++;
}

void GotchiStatsScene::draw() {
    Scene::draw();

    // Draw scene title
    DrawText("GOTCHI STATISTICS", 16, 8, 18, {180, 180, 255, 255});

    if (!gotchi) return;

    const GotchiStats& stats = gotchi->getStats();

    // ==============================
    // DEBUG PANEL (left side - right 3rd of screen)
    // ==============================
    // Only show DEBUG panel when we have access to globalGameState
    // Draw semi-transparent dark background
    int debugX = 480;
    int debugY = 40;
    DrawRectangle(debugX, debugY, 224, 600, {20, 20, 30, 200});

    int dx = debugX + 10;
    int dy = debugY + 15;
    int lineH = 14;

    // DEBUG header
    DrawText("DEBUG STATE READOUT", dx, dy, 12, {100, 255, 100, 255});
    dy += 18;

    // Check if we can access globalGameState
    if (gameState_) {
        // Helper to print a line
        auto printLine = [&](const std::string& label, const std::string& value) {
            DrawText((label + ": " + value).c_str(), dx, dy, 11, {200, 200, 200, 255});
            dy += lineH;
        };

        // Hidden drivers
        printLine("drivers.affection", std::to_string(gameState_->drivers.affection));
        printLine("drivers.mercy", std::to_string(gameState_->drivers.mercy));
        printLine("drivers.survival", std::to_string(gameState_->drivers.survival));

        // Metronome / neglect
        printLine("sleep", std::to_string(gameState_->sleep));
        printLine("grime", std::to_string(gameState_->grime));

        // Flow
        printLine("mode", modeToString(gameState_->mode));
        printLine("seenReality", gameState_->seenReality ? "true" : "false");
        printLine("deathEnabled", gameState_->deathEnabled ? "true" : "false");
        printLine("collapsed", gameState_->collapsed ? "true" : "false");
        printLine("firstMergeBucket", firstMergeToString(gameState_->firstMergeBucket));
        printLine("storyBeatIndex", std::to_string(gameState_->storyBeatIndex));
        printLine("mergeCount", std::to_string(gameState_->mergeCount));
        printLine("mergeLockTimer", std::to_string(gameState_->mergeLockTimer));
        printLine("engagedCareSide", gameState_->engagedCareSide ? "true" : "false");
        printLine("engagedStorySide", gameState_->engagedStorySide ? "true" : "false");

        // Meta
        printLine("playtimeSeconds", std::to_string(static_cast<int>(gameState_->playtimeSeconds)));

        // ENGINE NEEDS (the engine copy, not yet synced to visible bars)
        printLine("ENGINE NEEDS:", "");
        dy += 2;
        printLine("  needs.hunger", std::to_string(gameState_->needs.hunger));
        printLine("  needs.hygiene", std::to_string(gameState_->needs.hygiene));
        printLine("  needs.affection", std::to_string(gameState_->needs.affection));
        printLine("  needs.energy", std::to_string(gameState_->needs.energy));

        // Flags
        printLine("flags:", "");
        dy += 2;
        for (const auto& [key, value] : gameState_->flags) {
            std::string printed = std::visit(ValuePrinter{}, value);
            printLine("  " + key, printed);
        }

        // CareAction observation (from subscription)
        printLine("CareAction:", "");
        dy += 2;
        printLine("  last", lastCareAction_ >= 0 ? std::to_string(lastCareAction_) : "N/A");
        printLine("  count", std::to_string(careActionCount_));
    } else {
        DrawText("globalGameState not available", dx, dy, 11, {150, 150, 150, 255});
        dy += lineH;
        DrawText("(this scene needs gameState_ set)", dx, dy, 10, {100, 100, 100, 200});
    }

    // ==============================
    // LEFT COLUMN: Core Vital Stats (moved right, now on left side)
    // ==============================
    int lx = 16;
    int ly = 40;
    DrawText("CORE VITALS", lx, ly, 14, {100, 200, 255, 255});
    ly += 18;

    // Health bar
    float health = stats.getNormalizedStat(SecondaryStat::FITALITY);
    DrawText("Health", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 70, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 70, ly + 2, static_cast<int>(120 * health), 10, {0, 255, 0, 255});
    DrawText(std::to_string(static_cast<int>(health * 100)).c_str(), lx + 195, ly, 12, {200, 200, 200, 255});
    ly += 18;

    // Hunger bar (inverted - 0 is full, 100 is starving)
    float hunger = stats.getNormalizedStat(SecondaryStat::FOOD_LEVEL);
    DrawText("Hunger", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 70, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 70, ly + 2, static_cast<int>(120 * (1.0f - hunger)), 10, {255, 200, 0, 255});
    DrawText(std::to_string(static_cast<int>(hunger * 100)).c_str(), lx + 195, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Energy bar
    float energy = stats.getNormalizedStat(SecondaryStat::ENERGY);
    DrawText("Energy", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 70, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 70, ly + 2, static_cast<int>(120 * energy), 10, {255, 150, 0, 255});
    DrawText(std::to_string(static_cast<int>(energy * 100)).c_str(), lx + 195, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Thirst bar
    float thirst = stats.getNormalizedStat(SecondaryStat::HYDRATION);
    DrawText("Thirst", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 70, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 70, ly + 2, static_cast<int>(120 * (1.0f - thirst)), 10, {0, 150, 255, 255});
    DrawText(std::to_string(static_cast<int>(thirst * 100)).c_str(), lx + 195, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Sleep bar (inverted - 0 is rested, 100 is exhausted)
    float sleep = stats.getNormalizedStat(SecondaryStat::SLEEP_DEBT);
    DrawText("Sleep", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 70, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 70, ly + 2, static_cast<int>(120 * (1.0f - sleep)), 10, {100, 50, 150, 255});
    DrawText(std::to_string(static_cast<int>(sleep * 100)).c_str(), lx + 195, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Hygiene bar
    float hygiene = stats.getNormalizedStat(SecondaryStat::CLEANLINESS);
    DrawText("Hygiene", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 70, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 70, ly + 2, static_cast<int>(120 * hygiene), 10, {0, 200, 200, 255});
    DrawText(std::to_string(static_cast<int>(hygiene * 100)).c_str(), lx + 195, ly, 12, {200, 200, 200, 255});
    ly += 24;

    // ==============================
    // SOCIAL COLUMN (left, between CORE VITALS and EMOTIONAL)
    // ==============================
    int sx = 16;
    int sy = 240;

    DrawText("SOCIAL", sx, sy, 14, {100, 255, 200, 255});
    sy += 18;

    float popularity = stats.getNormalizedStat(SocialStat::POPULARITY);
    DrawText("Popularity", sx, sy, 11, {220, 220, 220, 255});
    DrawRectangle(sx + 70, sy + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(sx + 70, sy + 2, static_cast<int>(90 * popularity), 8, {100, 255, 100, 255});
    sy += 14;

    float friendship = stats.getNormalizedStat(EmotionalStat::FRIEDNSHIP);
    DrawText("Friendship", sx, sy, 11, {220, 220, 220, 255});
    DrawRectangle(sx + 70, sy + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(sx + 70, sy + 2, static_cast<int>(90 * friendship), 8, {200, 100, 255, 255});
    sy += 14;

    float empathy = stats.getNormalizedStat(SocialStat::EMPATHY);
    DrawText("Empathy", sx, sy, 11, {220, 220, 220, 255});
    DrawRectangle(sx + 70, sy + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(sx + 70, sy + 2, static_cast<int>(90 * empathy), 8, {150, 200, 255, 255});
    sy += 20;

    // ==============================
    // EMOTIONAL COLUMN (left, below SOCIAL)
    // ==============================
    int ex = 16;
    int ey = 340;

    DrawText("EMOTIONAL", ex, ey, 14, {200, 100, 255, 255});
    ey += 18;

    float happiness = stats.getNormalizedStat(EmotionalStat::HAPPINESS);
    DrawText("Happiness", ex, ey, 11, {220, 220, 220, 255});
    DrawRectangle(ex + 70, ey + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(ex + 70, ey + 2, static_cast<int>(90 * happiness), 8, {255, 200, 50, 255});
    ey += 14;

    float excitement = stats.getNormalizedStat(EmotionalStat::EXCITEMENT);
    DrawText("Excitement", ex, ey, 11, {220, 220, 220, 255});
    DrawRectangle(ex + 70, ey + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(ex + 70, ey + 2, static_cast<int>(90 * excitement), 8, {255, 100, 0, 255});
    ey += 14;

    float satisfaction = stats.getNormalizedStat(EmotionalStat::SATISFACTION);
    DrawText("Satisfaction", ex, ey, 11, {220, 220, 220, 255});
    DrawRectangle(ex + 70, ey + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(ex + 70, ey + 2, static_cast<int>(90 * satisfaction), 8, {0, 200, 100, 255});
    ey += 14;

    float love = stats.getNormalizedStat(EmotionalStat::LOVE);
    DrawText("Love/Affection", ex, ey, 11, {220, 220, 220, 255});
    DrawRectangle(ex + 70, ey + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(ex + 70, ey + 2, static_cast<int>(90 * love), 8, {255, 150, 200, 255});
    ey += 14;

    float confidence = stats.getNormalizedStat(EmotionalStat::CONFIDENCE);
    DrawText("Confidence", ex, ey, 11, {220, 220, 220, 255});
    DrawRectangle(ex + 70, ey + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(ex + 70, ey + 2, static_cast<int>(90 * confidence), 8, {255, 200, 100, 255});
    ey += 20;

    // ==============================
    // BOTTOM CENTER: Mood & Status
    // ==============================
    int mx2 = 16;
    int my2 = 540;

    // Current Mood
    DrawText("CURRENT MOOD", mx2, my2, 14, {255, 220, 100, 255});
    my2 += 18;
    std::string moodName = gotchi->getMood().getMoodName();
    DrawText(moodName.c_str(), mx2, my2, 20, {255, 255, 255, 255});
    my2 += 30;

    // Mood color indicator
    Color moodTint = gotchi->getMood().getMoodTint();
    DrawRectangle(mx2, my2, 50, 50, moodTint);
    DrawRectangleLines(mx2, my2, 50, 50, {255, 255, 255, 200});
    my2 += 60;

    // Age
    float age = stats.getStat(SecondaryStat::AGE);
    DrawText(("Age: " + std::to_string(static_cast<int>(age))).c_str(), mx2, my2, 12, {200, 200, 200, 255});
    my2 += 16;

    // Show tick rate
    std::string tickText = "Ticks: " + std::to_string(static_cast<int>(gotchi->getStats().getStat(SecondaryStat::AGE)));
    DrawText(tickText.c_str(), mx2, my2, 12, {150, 150, 200, 255});
    my2 += 16;

    // Draw buttons
    for (auto& btn : buttons) {
        btn->draw();
    }
}

void GotchiStatsScene::cleanup() {
    Scene::cleanup();
    // Unsubscribe from CareAction events
    if (careActionToken_ != 0) {
        globalEventBus.unsubscribe(careActionToken_);
        careActionToken_ = 0;
    }
    gotchi = nullptr;
    buttons.clear();
}
