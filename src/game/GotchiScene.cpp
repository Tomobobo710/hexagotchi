#include "GotchiScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "GotchiStats.hpp"
#include "GotchiMood.hpp"
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
}

void GotchiScene::update(float deltaTime) {
    Scene::update(deltaTime);

    // Update the Gotchi every frame
    if (gotchi) {
        gotchi->update(deltaTime);
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
    std::vector<std::pair<std::string, float>> moodDrivers;
    moodDrivers.push_back({"Happiness", gotchi->getStats().getNormalizedStat(EmotionalStat::HAPPINESS)});
    moodDrivers.push_back({"Excitement", gotchi->getStats().getNormalizedStat(EmotionalStat::EXCITEMENT)});
    moodDrivers.push_back({"Satisfaction", gotchi->getStats().getNormalizedStat(EmotionalStat::SATISFACTION)});
    moodDrivers.push_back({"Energy", energy});
    moodDrivers.push_back({"Health", health});

    for (const auto& driver : moodDrivers) {
        DrawText((driver.first + ": " + std::to_string(static_cast<int>(driver.second * 100)) + "%").c_str(), cx, cy, 11, {180, 180, 200, 255});
        cy += 14;
    }

    // ==============================
    // RIGHT PANEL: Emotional & Social Stats
    // ==============================
    int rx = 520;
    int ry = 40;

    DrawText("EMOTIONAL", rx, ry, 14, {200, 100, 255, 255});
    ry += 18;

    // Emotional bars
    float happiness = gotchi->getStats().getNormalizedStat(EmotionalStat::HAPPINESS);
    DrawText("Happiness", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * happiness), 8, {255, 200, 50, 255});
    ry += 14;

    float excitement = gotchi->getStats().getNormalizedStat(EmotionalStat::EXCITEMENT);
    DrawText("Excitement", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * excitement), 8, {255, 100, 0, 255});
    ry += 14;

    float satisfaction = gotchi->getStats().getNormalizedStat(EmotionalStat::SATISFACTION);
    DrawText("Satisfaction", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * satisfaction), 8, {0, 200, 100, 255});
    ry += 14;

    float love = gotchi->getStats().getNormalizedStat(EmotionalStat::LOVE);
    DrawText("Love/Affection", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * love), 8, {255, 150, 200, 255});
    ry += 14;

    float friendship = gotchi->getStats().getNormalizedStat(EmotionalStat::FRIEDNSHIP);
    DrawText("Friends", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * friendship), 8, {200, 100, 255, 255});
    ry += 20;

    DrawText("SOCIAL", rx, ry, 14, {100, 255, 200, 255});
    ry += 18;

    float popularity = gotchi->getStats().getNormalizedStat(SocialStat::POPULARITY);
    DrawText("Popularity", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * popularity), 8, {100, 255, 100, 255});
    ry += 14;

    float confidence = gotchi->getStats().getNormalizedStat(EmotionalStat::CONFIDENCE);
    DrawText("Confidence", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * confidence), 8, {255, 200, 100, 255});
    ry += 20;

    // ==============================
    // BOTTOM PANEL: Status & Info
    // ==============================
    int bx = 16;
    int by = 580;

    DrawText("STATUS", bx, by, 12, {150, 150, 255, 255});
    by += 16;

    // Status indicators (state icons)
    std::string statusText;
    if (gotchi->isSleeping()) {
        statusText = "[SLEEPING] - Needs rest";
        DrawText(statusText.c_str(), bx, by, 12, {150, 100, 255, 255});
    } else if (gotchi->isDead()) {
        statusText = "[DEAD] - Game Over";
        DrawText(statusText.c_str(), bx, by, 12, {255, 50, 50, 255});
    } else if (!gotchi->isActive()) {
        statusText = "[INACTIVE] - Click to interact";
        DrawText(statusText.c_str(), bx, by, 12, {255, 200, 50, 255});
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
            DrawText(statusText.c_str(), bx, by, 12, {255, 100, 100, 255});
        } else {
            DrawText("[ALL GOOD] - Gotchi is content", bx, by, 12, {100, 255, 100, 255});
        }
    }
    by += 20;

    // Stats summary at bottom
    DrawText("STAT SUMMARY", bx, by, 12, {150, 150, 255, 255});
    by += 16;

    std::ostringstream summary;
    summary << "Health: " << static_cast<int>(health * 100)
            << " | Hunger: " << static_cast<int>(hunger * 100)
            << " | Energy: " << static_cast<int>(energy * 100)
            << " | Mood: " << moodName;
    DrawText(summary.str().c_str(), bx, by, 11, {200, 200, 200, 255});

    // ==============================
    // BOTTOM-RIGHT: Simulation Clock (updates each frame)
    // ==============================
    int rx2 = 520;
    int ry2 = 580;

    // Draw a blinking/running indicator to show simulation is active
    float blinkTimer = std::fmod(simTime_, 1.0f);
    bool blinkOn = blinkTimer < 0.5f;

    if (blinkOn) {
        DrawText("RUNNING", rx2, ry2, 12, {100, 255, 100, 255});
    } else {
        DrawText("RUNNING", rx2, ry2, 12, {50, 200, 50, 255});
    }

    // Show formatted time
    int totalSeconds = static_cast<int>(simTime_);
    int minutes = totalSeconds / 60;
    int seconds = totalSeconds % 60;
    int frames = frameCount_ % 100;

    std::ostringstream clock;
    clock << "Time: " << (minutes < 10 ? "0" : "") << minutes << ":"
          << (seconds < 10 ? "0" : "") << seconds << "  Frame: " << frameCount_;
    DrawText(clock.str().c_str(), rx2, ry2 + 16, 11, {200, 200, 200, 255});

    // Show tick rate (updates per second)
    std::string tickText = "Ticks: " + std::to_string(static_cast<int>(gotchi->getStats().getStat(SecondaryStat::AGE)));
    DrawText(tickText.c_str(), rx2, ry2 + 30, 11, {150, 150, 200, 255});
}

void GotchiScene::cleanup() {
    Scene::cleanup();
    gotchi = nullptr;
}
