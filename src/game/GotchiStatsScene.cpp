#include "GotchiStatsScene.hpp"
#include "AssetPack.hpp"
#include "GameConstants.hpp"
#include "GotchiStats.hpp"
#include "GotchiMood.hpp"
#include "SceneManager.hpp"
#include <iostream>
#include <sstream>

GotchiStatsScene::GotchiStatsScene()
    : Scene(720.0f, 720.0f, {40, 40, 60, 255}), simTime_(0.0f), frameCount_(0) {
    gotchiDir = "gotchis/001";
}

void GotchiStatsScene::init() {
    Scene::init();
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
            mgr->switchScene(targetScene, TransitionEffect::FADE, 0.5f);
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
    // LEFT COLUMN: Core Vital Stats
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
    // MIDDLE COLUMN: Emotional Stats
    // ==============================
    int mx = 250;
    int my = 40;

    DrawText("EMOTIONAL", mx, my, 14, {200, 100, 255, 255});
    my += 18;

    float happiness = stats.getNormalizedStat(EmotionalStat::HAPPINESS);
    DrawText("Happiness", mx, my, 11, {220, 220, 220, 255});
    DrawRectangle(mx + 70, my + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(mx + 70, my + 2, static_cast<int>(90 * happiness), 8, {255, 200, 50, 255});
    my += 14;

    float excitement = stats.getNormalizedStat(EmotionalStat::EXCITEMENT);
    DrawText("Excitement", mx, my, 11, {220, 220, 220, 255});
    DrawRectangle(mx + 70, my + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(mx + 70, my + 2, static_cast<int>(90 * excitement), 8, {255, 100, 0, 255});
    my += 14;

    float satisfaction = stats.getNormalizedStat(EmotionalStat::SATISFACTION);
    DrawText("Satisfaction", mx, my, 11, {220, 220, 220, 255});
    DrawRectangle(mx + 70, my + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(mx + 70, my + 2, static_cast<int>(90 * satisfaction), 8, {0, 200, 100, 255});
    my += 14;

    float love = stats.getNormalizedStat(EmotionalStat::LOVE);
    DrawText("Love/Affection", mx, my, 11, {220, 220, 220, 255});
    DrawRectangle(mx + 70, my + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(mx + 70, my + 2, static_cast<int>(90 * love), 8, {255, 150, 200, 255});
    my += 14;

    float confidence = stats.getNormalizedStat(EmotionalStat::CONFIDENCE);
    DrawText("Confidence", mx, my, 11, {220, 220, 220, 255});
    DrawRectangle(mx + 70, my + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(mx + 70, my + 2, static_cast<int>(90 * confidence), 8, {255, 200, 100, 255});
    my += 20;

    // ==============================
    // RIGHT COLUMN: Social Stats
    // ==============================
    int rx = 480;
    int ry = 40;

    DrawText("SOCIAL", rx, ry, 14, {100, 255, 200, 255});
    ry += 18;

    float popularity = stats.getNormalizedStat(SocialStat::POPULARITY);
    DrawText("Popularity", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * popularity), 8, {100, 255, 100, 255});
    ry += 14;

    float friendship = stats.getNormalizedStat(EmotionalStat::FRIEDNSHIP);
    DrawText("Friendship", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * friendship), 8, {200, 100, 255, 255});
    ry += 14;

    float empathy = stats.getNormalizedStat(SocialStat::EMPATHY);
    DrawText("Empathy", rx, ry, 11, {220, 220, 220, 255});
    DrawRectangle(rx + 70, ry + 2, 90, 8, {50, 50, 50, 255});
    DrawRectangle(rx + 70, ry + 2, static_cast<int>(90 * empathy), 8, {150, 200, 255, 255});
    ry += 20;

    // ==============================
    // BOTTOM PANEL: Physical & Other Stats
    // ==============================
    int bx = 16;
    int by = 320;

    DrawText("PHYSICAL", bx, by, 14, {255, 200, 100, 255});
    by += 18;

    float strength = stats.getNormalizedStat(PhysicalStat::STRENGTH);
    DrawText("Strength", bx, by, 11, {220, 220, 220, 255});
    DrawRectangle(bx + 70, by + 2, 100, 8, {50, 50, 50, 255});
    DrawRectangle(bx + 70, by + 2, static_cast<int>(100 * strength), 8, {255, 100, 100, 255});
    by += 14;

    float agility = stats.getNormalizedStat(PhysicalStat::AGILITY);
    DrawText("Agility", bx, by, 11, {220, 220, 220, 255});
    DrawRectangle(bx + 70, by + 2, 100, 8, {50, 50, 50, 255});
    DrawRectangle(bx + 70, by + 2, static_cast<int>(100 * agility), 8, {255, 150, 50, 255});
    by += 14;

    float endurance = stats.getNormalizedStat(PhysicalStat::ENDURANCE);
    DrawText("Endurance", bx, by, 11, {220, 220, 220, 255});
    DrawRectangle(bx + 70, by + 2, 100, 8, {50, 50, 50, 255});
    DrawRectangle(bx + 70, by + 2, static_cast<int>(100 * endurance), 8, {255, 200, 50, 255});
    by += 14;

    float reflex = stats.getNormalizedStat(PhysicalStat::REFLEX);
    DrawText("Reflex", bx, by, 11, {220, 220, 220, 255});
    DrawRectangle(bx + 70, by + 2, 100, 8, {50, 50, 50, 255});
    DrawRectangle(bx + 70, by + 2, static_cast<int>(100 * reflex), 8, {255, 250, 50, 255});
    by += 14;

    float vision = stats.getNormalizedStat(PhysicalStat::VISION);
    DrawText("Vision", bx, by, 11, {220, 220, 220, 255});
    DrawRectangle(bx + 70, by + 2, 100, 8, {50, 50, 50, 255});
    DrawRectangle(bx + 70, by + 2, static_cast<int>(100 * vision), 8, {200, 255, 50, 255});
    by += 20;

    // ==============================
    // BOTTOM-RIGHT: Status & Info
    // ==============================
    int rx2 = 520;
    int ry2 = 580;

    // Current Mood
    DrawText("CURRENT MOOD", rx2, ry2, 12, {255, 220, 100, 255});
    ry2 += 16;
    std::string moodName = gotchi->getMood().getMoodName();
    DrawText(moodName.c_str(), rx2, ry2, 16, {255, 255, 255, 255});
    ry2 += 24;

    // Age
    float age = stats.getStat(SecondaryStat::AGE);
    DrawText(("Age: " + std::to_string(static_cast<int>(age))).c_str(), rx2, ry2, 12, {200, 200, 200, 255});
    ry2 += 16;

    // Show tick rate
    std::string tickText = "Ticks: " + std::to_string(static_cast<int>(gotchi->getStats().getStat(SecondaryStat::AGE)));
    DrawText(tickText.c_str(), rx2, ry2, 11, {150, 150, 200, 255});

    // Draw buttons
    for (auto& btn : buttons) {
        btn->draw();
    }
}

void GotchiStatsScene::cleanup() {
    Scene::cleanup();
    gotchi = nullptr;
    buttons.clear();
}
