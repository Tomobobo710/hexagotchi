#include "GotchiScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "GotchiStats.hpp"
#include "GotchiMood.hpp"
#include "SceneManager.hpp"
#include "EventBus.h"
#include "MergeController.hpp"
#include <iostream>
#include <sstream>

GotchiScene::GotchiScene()
    : Scene(720.0f, 720.0f, {40, 40, 60, 255}) {
    gotchiDir = "gotchis/001";
}

void GotchiScene::init() {
    // Center camera
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);

    // Create Gotchi at center of screen
    gotchi = new Gotchi({360.0f, 360.0f});
    gotchi->setTag("gotchi");
    gotchi->setScale({4.0f, 4.0f});  // Scale up for visibility
    addActor(gotchi);

    // Initialize the Gotchi
    gotchi->init();

    // Load animation frames using the Gotchi's loader
    // This populates the Gotchi's internal frame arrays for each action
    gotchi->loadAnimationFrames(gotchiDir);

    // Set initial action (will use the loaded frames)
    gotchi->setAction("idle");

    // Add navigation buttons
    addButtons();
}

void GotchiScene::addButtons() {
    buttons.clear();
    lastClickedButton_ = "";

    // Add action buttons at the bottom
    // Six buttons: Wash, Groom, Feed, Pet, Play, Give a Break (merge button)
    float buttonWidth = 80.0f;
    float buttonHeight = 32.0f;
    float totalWidth = 6 * buttonWidth + 5 * 10.0f;  // 6 buttons + 5 gaps
    float startX = (GAME_W - totalWidth) / 2.0f;
    float y = GAME_H - 40.0f;

    std::vector<std::string> labels = {"Wash", "Groom", "Feed", "Pet", "Play", "Give a Break"};
    for (size_t i = 0; i < labels.size(); i++) {
        float x = startX + i * (buttonWidth + 10.0f);
        // The 6th button (index 5) is the merge button
        addButton(labels[i], x, y, (i == 5));
    }

    // Add "Detailed Vitals" button at bottom center
    addNavigationButton("DETAILED VITALS", "gotchi_stats", (float)GAME_W / 2.0f, (float)GAME_H - 80);
}

void GotchiScene::addButton(const std::string& label, float x, float y, bool isMergeButton) {
    float buttonWidth = 80.0f;
    float buttonHeight = 32.0f;
    Button* btn = new Button({x, y}, buttonWidth, buttonHeight, label);
    btn->setAnchor("top-left");
    btn->setFontSize(12);
    btn->setBackgroundColor({60, 60, 100, 220});
    btn->setHoverColor({100, 100, 160, 240});
    btn->setBorderColor({150, 150, 200, 255});

    if (isMergeButton) {
        // Merge button emits MergeRequested on the event bus
        btn->setOnClick([this]() {
            onMergeButtonClicked();
        });
    } else {
        // Other buttons just set the last clicked message
        btn->setOnClick([this, label]() {
            lastClickedButton_ = label + " was clicked";
        });
    }
    buttons.push_back(std::unique_ptr<Button>(btn));
}

// The merge button callback - emits MergeRequested on the bus
// This is the "Give a Break" button which becomes "Merge" after first merge
void GotchiScene::onMergeButtonClicked() {
    // Emit MergeRequested event
    // The MergeController will decide whether to honor this request based on game state
    if (eventBus_) {
        eventBus_->emit(Event::mergeRequested());
    }
}

void GotchiScene::update(float deltaTime) {
    Scene::update(deltaTime);

    // Update the Gotchi every frame
    if (gotchi) {
        gotchi->update(deltaTime);
    }

    // Update button states
    SceneInputHandler* input = getInputHandler();
    for (auto& btn : buttons) {
        btn->update(input, deltaTime);
    }

    // Update simulation time and frame count
    simTime_ += deltaTime;
    frameCount_++;
}

void GotchiScene::draw() {
    Scene::draw();

    // Draw scene title and instructions
    DrawText("GOTCHI SCENE", 16, 8, 18, {180, 180, 255, 255});
    DrawText("1: Game  2: Boss  3: Input  4: Sprite  5: Hexboard  8: Gotchi  ESC: Exit", GAME_W - 320, 8, 12, {140, 140, 180, 255});

    if (!gotchi) return;

    // ==============================
    // LEFT PANEL: Core Vital Stats
    // ==============================
    int lx = 16;
    int ly = 40;
    DrawText("VITAL STATS", lx, ly, 14, {100, 200, 255, 255});
    ly += 18;

    // Health bar
    float health = gotchi->getStats().getNormalizedStat(SecondaryStat::FITALITY);
    DrawText("Health", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});  // bg bar
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * health), 10, {0, 255, 0, 255});  // fg bar
    DrawText(std::to_string(static_cast<int>(health * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 18;

    // Hunger bar (inverted - 0 is full, 100 is starving)
    float hunger = gotchi->getStats().getNormalizedStat(SecondaryStat::FOOD_LEVEL);
    DrawText("Hunger", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * (1.0f - hunger)), 10, {255, 200, 0, 255});
    DrawText(std::to_string(static_cast<int>(hunger * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Energy bar
    float energy = gotchi->getStats().getNormalizedStat(SecondaryStat::ENERGY);
    DrawText("Energy", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * energy), 10, {255, 150, 0, 255});
    DrawText(std::to_string(static_cast<int>(energy * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Thirst bar
    float thirst = gotchi->getStats().getNormalizedStat(SecondaryStat::HYDRATION);
    DrawText("Thirst", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * (1.0f - thirst)), 10, {0, 150, 255, 255});
    DrawText(std::to_string(static_cast<int>(thirst * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Sleep bar (inverted - 0 is rested, 100 is exhausted)
    float sleep = gotchi->getStats().getNormalizedStat(SecondaryStat::SLEEP_DEBT);
    DrawText("Sleep", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * (1.0f - sleep)), 10, {100, 50, 150, 255});
    DrawText(std::to_string(static_cast<int>(sleep * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 16;

    // Hygiene bar
    float hygiene = gotchi->getStats().getNormalizedStat(SecondaryStat::CLEANLINESS);
    DrawText("Hygiene", lx, ly, 12, {200, 200, 200, 255});
    DrawRectangle(lx + 60, ly + 2, 120, 10, {50, 50, 50, 255});
    DrawRectangle(lx + 60, ly + 2, static_cast<int>(120 * hygiene), 10, {0, 200, 200, 255});
    DrawText(std::to_string(static_cast<int>(hygiene * 100)).c_str(), lx + 185, ly, 12, {200, 200, 200, 255});
    ly += 24;

    // ==============================
    // CENTER: Mood Display
    // ==============================
    int cx = 280;
    int cy = 40;

    // Current Mood (large, prominent)
    DrawText("CURRENT MOOD", cx, cy, 14, {255, 220, 100, 255});
    cy += 20;
    std::string moodName = gotchi->getMood().getMoodName();
    DrawText(moodName.c_str(), cx, cy, 24, {255, 255, 255, 255});
    cy += 35;

    // Mood color indicator
    Color moodTint = gotchi->getMood().getMoodTint();
    DrawRectangle(cx, cy, 40, 40, moodTint);
    DrawRectangleLines(cx, cy, 40, 40, {255, 255, 255, 200});
    cy += 50;

    // Mood description
    DrawText("Mood Drivers:", cx, cy, 12, {150, 150, 200, 255});
    cy += 16;

    // Calculate and display key mood drivers
    float happiness = gotchi->getStats().getNormalizedStat(EmotionalStat::HAPPINESS);
    float excitement = gotchi->getStats().getNormalizedStat(EmotionalStat::EXCITEMENT);
    float satisfaction = gotchi->getStats().getNormalizedStat(EmotionalStat::SATISFACTION);

    std::vector<std::pair<std::string, float>> moodDrivers;
    moodDrivers.push_back({"Happiness", happiness});
    moodDrivers.push_back({"Excitement", excitement});
    moodDrivers.push_back({"Satisfaction", satisfaction});
    moodDrivers.push_back({"Energy", energy});
    moodDrivers.push_back({"Health", health});

    for (const auto& driver : moodDrivers) {
        DrawText((driver.first + ": " + std::to_string(static_cast<int>(driver.second * 100)) + "%").c_str(), cx, cy, 11, {180, 180, 200, 255});
        cy += 14;
    }

    // ==============================
    // TOP-RIGHT: Status & Info
    // ==============================
    int rx = 520;
    int ry = 40;

    // Draw a blinking/running indicator to show simulation is active
    float blinkTimer = std::fmod(simTime_, 1.0f);
    bool blinkOn = blinkTimer < 0.5f;

    if (blinkOn) {
        DrawText("RUNNING", rx, ry, 12, {100, 255, 100, 255});
    } else {
        DrawText("RUNNING", rx, ry, 12, {50, 200, 50, 255});
    }
    ry += 16;

    // Status indicators
    std::string statusText;
    if (gotchi->isSleeping()) {
        statusText = "[SLEEPING] - Needs rest";
        DrawText(statusText.c_str(), rx, ry, 12, {150, 100, 255, 255});
        ry += 16;
    } else if (gotchi->isDead()) {
        statusText = "[DEAD] - Game Over";
        DrawText(statusText.c_str(), rx, ry, 12, {255, 50, 50, 255});
        ry += 16;
    } else if (!gotchi->isActive()) {
        statusText = "[INACTIVE] - Click to interact";
        DrawText(statusText.c_str(), rx, ry, 12, {255, 200, 50, 255});
        ry += 16;
    } else {
        // Check for needs that require action
        std::vector<std::string> needs;
        if (hunger > 0.7f) needs.push_back("Hungry");
        if (thirst > 0.7f) needs.push_back("Thirsty");
        if (sleep > 0.7f) needs.push_back("Sleepy");
        if (health < 0.3f) needs.push_back("Sick");
        if (happiness < 0.3f) needs.push_back("Sad");

        if (!needs.empty()) {
            for (size_t i = 0; i < needs.size(); i++) {
                if (i > 0) statusText += ", ";
                statusText += needs[i];
            }
            statusText = "[NEEDS ATTENTION] " + statusText;
            DrawText(statusText.c_str(), rx, ry, 12, {255, 100, 100, 255});
            ry += 16;
        } else {
            DrawText("[ALL GOOD] - Gotchi is content", rx, ry, 12, {100, 255, 100, 255});
            ry += 16;
        }
    }

    // Show formatted time
    int totalSeconds = static_cast<int>(simTime_);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;

    std::ostringstream clock;
    clock << "Time: " << (minutes < 10 ? "0" : "") << minutes << ":"
          << (seconds < 10 ? "0" : "") << seconds;
    DrawText(clock.str().c_str(), rx, ry, 11, {200, 200, 200, 255});
    ry += 16;

    // Show tick rate
    std::string tickText = "Ticks: " + std::to_string(static_cast<int>(gotchi->getStats().getStat(SecondaryStat::AGE)));
    DrawText(tickText.c_str(), rx, ry, 11, {150, 150, 200, 255});
    ry += 16;

    // Stats summary
    DrawText("STAT SUMMARY", rx, ry, 12, {150, 150, 255, 255});
    ry += 16;

    std::ostringstream summary;
    summary << "Health: " << static_cast<int>(health * 100)
            << " | Hunger: " << static_cast<int>(hunger * 100)
            << " | Energy: " << static_cast<int>(energy * 100)
            << " | Mood: " << moodName;
    DrawText(summary.str().c_str(), rx, ry, 11, {200, 200, 200, 255});

    // Draw button click message above action buttons
    if (!lastClickedButton_.empty()) {
        int msgWidth = MeasureText(lastClickedButton_.c_str(), 12);
        int msgX = (GAME_W - msgWidth) / 2;
        DrawText(lastClickedButton_.c_str(), msgX, GAME_H - 140, 12, {255, 255, 255, 255});
    }

    // Draw buttons
    for (auto& btn : buttons) {
        btn->draw();
    }
}

void GotchiScene::addNavigationButton(const std::string& label, const std::string& targetScene, float x, float y) {
    float buttonWidth = 160.0f;
    float buttonHeight = 32.0f;
    Button* btn = new Button({x, y}, buttonWidth, buttonHeight, label);
    btn->setAnchor("center");
    btn->setFontSize(14);
    btn->setBackgroundColor({60, 60, 100, 220});
    btn->setHoverColor({100, 100, 160, 240});
    btn->setBorderColor({150, 150, 200, 255});

    btn->setOnClick([this, targetScene]() {
        if (getSceneManager()) {
            SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
            mgr->switchScene(targetScene, TransitionEffect::FADE, 0.5f);
        }
    });
    buttons.push_back(std::unique_ptr<Button>(btn));
}

void GotchiScene::cleanup() {
    Scene::cleanup();
    gotchi = nullptr;
}
