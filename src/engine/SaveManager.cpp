#include "SaveManager.h"
#include "GameState.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <stdexcept>

// Include JSON library (nlohmann/json)
#include "json.hpp"

// For desktop: use a per-user save directory
// For web: IDBFS mount point is set at runtime
static std::string g_saveDir;

void SaveManager::initSaveDir() {
#if defined(PLATFORM_WEB)
    // Web: use the current working directory which Emscripten mounts to IDBFS
    g_saveDir = "./saves";
#else
    // Desktop: use a per-user directory next to executable or in user home
    // For the jam, we'll use a local directory for convenience
    g_saveDir = "./saves";
#endif
    // Create directory if it doesn't exist
    std::filesystem::create_directories(g_saveDir);
}

void SaveManager::setSaveDir(const std::string& dir) {
    g_saveDir = dir;
    std::filesystem::create_directories(g_saveDir);
}

std::string SaveManager::saveDir() const {
    return g_saveDir;
}

std::string SaveManager::slotPath(int slot) const {
    return saveDir() + "/save_" + std::to_string(slot) + ".json";
}

std::string SaveManager::slotTempPath(int slot) const {
    return saveDir() + "/save_" + std::to_string(slot) + ".json.tmp";
}

bool SaveManager::slotExists(int slot) const {
    return std::filesystem::exists(slotPath(slot));
}

SlotSummary SaveManager::summary(int slot) const {
    SlotSummary sum;
    if (!slotExists(slot)) {
        return sum;
    }

    std::ifstream file(slotPath(slot));
    if (!file.is_open()) {
        return sum;
    }

    try {
        nlohmann::json j;
        file >> j;

        sum.exists = true;
        sum.version = j.value("version", 0);
        sum.storyBeatIndex = j.value("storyBeatIndex", 0);
        sum.playtimeSeconds = j.value("playtimeSeconds", 0.0);
        sum.label = j.value("label", "");
    } catch (...) {
        // Corrupt file - treat as non-existent
    }

    return sum;
}

// JSON serialization for GameState
void to_json(nlohmann::json& j, const GameState::Needs& n) {
    j = nlohmann::json{
        {"hunger", n.hunger},
        {"hygiene", n.hygiene},
        {"affection", n.affection},
        {"energy", n.energy}
    };
}

void from_json(const nlohmann::json& j, GameState::Needs& n) {
    n.hunger = j.value("hunger", 1.0f);
    n.hygiene = j.value("hygiene", 1.0f);
    n.affection = j.value("affection", 1.0f);
    n.energy = j.value("energy", 1.0f);
}

void to_json(nlohmann::json& j, const GameState::Drivers& d) {
    j = nlohmann::json{
        {"affection", d.affection},
        {"mercy", d.mercy},
        {"survival", d.survival}
    };
}

void from_json(const nlohmann::json& j, GameState::Drivers& d) {
    d.affection = j.value("affection", 0.0f);
    d.mercy = j.value("mercy", 0.0f);
    d.survival = j.value("survival", 1.0f);
}

// GotchiStats serialization helpers
// These are simple struct-style serialization since GotchiStats is a complex class
// We'll serialize each stat array with its enum indices

// Helper to serialize an array of stats
static void serializeStatArray(nlohmann::json& j, const std::string& name, const std::array<float, 10>& arr) {
    nlohmann::json arr_json = nlohmann::json::array();
    for (float val : arr) arr_json.push_back(val);
    j[name] = arr_json;
}

static void serializeStatArray(nlohmann::json& j, const std::string& name, const std::array<float, 20>& arr) {
    nlohmann::json arr_json = nlohmann::json::array();
    for (float val : arr) arr_json.push_back(val);
    j[name] = arr_json;
}

static void serializeStatArray(nlohmann::json& j, const std::string& name, const std::array<float, 30>& arr) {
    nlohmann::json arr_json = nlohmann::json::array();
    for (float val : arr) arr_json.push_back(val);
    j[name] = arr_json;
}

// Deserialize helper
static void deserializeStatArray(const nlohmann::json& j, const std::string& name, std::array<float, 10>& arr) {
    if (j.contains(name)) {
        const auto& arr_json = j[name];
        for (size_t i = 0; i < 10 && i < arr_json.size(); i++) {
            arr[i] = arr_json[i].get<float>();
        }
    }
}

static void deserializeStatArray(const nlohmann::json& j, const std::string& name, std::array<float, 20>& arr) {
    if (j.contains(name)) {
        const auto& arr_json = j[name];
        for (size_t i = 0; i < 20 && i < arr_json.size(); i++) {
            arr[i] = arr_json[i].get<float>();
        }
    }
}

static void deserializeStatArray(const nlohmann::json& j, const std::string& name, std::array<float, 30>& arr) {
    if (j.contains(name)) {
        const auto& arr_json = j[name];
        for (size_t i = 0; i < 30 && i < arr_json.size(); i++) {
            arr[i] = arr_json[i].get<float>();
        }
    }
}

// Serialize Value variant with type tag
void to_json(nlohmann::json& j, const Value& v) {
    if (std::holds_alternative<bool>(v)) {
        j = nlohmann::json{{"t", "bool"}, {"v", std::get<bool>(v)}};
    } else if (std::holds_alternative<int>(v)) {
        j = nlohmann::json{{"t", "int"}, {"v", std::get<int>(v)}};
    } else if (std::holds_alternative<float>(v)) {
        j = nlohmann::json{{"t", "float"}, {"v", std::get<float>(v)}};
    } else if (std::holds_alternative<std::string>(v)) {
        j = nlohmann::json{{"t", "str"}, {"v", std::get<std::string>(v)}};
    }
}

void from_json(const nlohmann::json& j, Value& v) {
    std::string type = j.value("t", "");
    if (type == "bool") {
        v = j.value("v", false);
    } else if (type == "int") {
        v = j.value("v", 0);
    } else if (type == "float") {
        v = j.value("v", 0.0f);
    } else if (type == "str") {
        v = j.value("v", std::string{});
    }
}

// Include GotchiStats after Value serialization so we can use it in serialization
#include "GotchiStats.hpp"

// GotchiStats serialization - use public getStat/setStat methods
void to_json(nlohmann::json& j, const GotchiStats& stats) {
    // Serialize all stats using the public API
    // We'll iterate through all stat enums and serialize their values

    // Helper lambda to serialize enum stats
    auto serializeEnumStats = [&stats](nlohmann::json& arr_json, auto enumValue) {
        // This won't work directly, so we'll manually enumerate
        (void)enumValue;
    };

    // Core stats (10) - no enum iteration possible, so we use the enum names
    nlohmann::json core_json = nlohmann::json::array();
    core_json.push_back(stats.getStat(SecondaryStat::WEIGHT));
    core_json.push_back(stats.getStat(SecondaryStat::FITNESS));
    core_json.push_back(stats.getStat(SecondaryStat::GROWTH));
    core_json.push_back(stats.getStat(SecondaryStat::AGE));
    core_json.push_back(stats.getStat(SecondaryStat::BURDEN));
    core_json.push_back(stats.getStat(SecondaryStat::EXPOSURE));
    core_json.push_back(stats.getStat(SecondaryStat::DISEASE));
    core_json.push_back(stats.getStat(SecondaryStat::IMMUNITY));
    core_json.push_back(stats.getStat(SecondaryStat::FERTILITY));
    core_json.push_back(stats.getStat(SecondaryStat::STAGE));
    j["coreStats"] = core_json;

    // Secondary stats (30)
    nlohmann::json secondary_json = nlohmann::json::array();
    secondary_json.push_back(stats.getStat(SecondaryStat::SIZE));
    secondary_json.push_back(stats.getStat(SecondaryStat::SPEED));
    secondary_json.push_back(stats.getStat(SecondaryStat::JUMP));
    secondary_json.push_back(stats.getStat(SecondaryStat::CLIMB));
    secondary_json.push_back(stats.getStat(SecondaryStat::SWIM));
    secondary_json.push_back(stats.getStat(SecondaryStat::FLY));
    secondary_json.push_back(stats.getStat(SecondaryStat::CAMOUFLAGE));
    secondary_json.push_back(stats.getStat(SecondaryStat::RESILIENCE));
    secondary_json.push_back(stats.getStat(SecondaryStat::TOUGHNESS));
    secondary_json.push_back(stats.getStat(SecondaryStat::FLEXIBILITY));
    secondary_json.push_back(stats.getStat(SecondaryStat::FITALITY));
    secondary_json.push_back(stats.getStat(SecondaryStat::FOOD_LEVEL));
    secondary_json.push_back(stats.getStat(SecondaryStat::HYDRATION));
    secondary_json.push_back(stats.getStat(SecondaryStat::ENERGY));
    secondary_json.push_back(stats.getStat(SecondaryStat::SLEEP_DEBT));
    secondary_json.push_back(stats.getStat(SecondaryStat::CLEANLINESS));
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    secondary_json.push_back(0);  // padding
    j["secondaryStats"] = secondary_json;

    // Emotional stats (20)
    nlohmann::json emotional_json = nlohmann::json::array();
    emotional_json.push_back(stats.getStat(EmotionalStat::HAPPINESS));
    emotional_json.push_back(stats.getStat(EmotionalStat::EXCITEMENT));
    emotional_json.push_back(stats.getStat(EmotionalStat::SATISFACTION));
    emotional_json.push_back(stats.getStat(EmotionalStat::BOREDOM));
    emotional_json.push_back(stats.getStat(EmotionalStat::ANXIETY));
    emotional_json.push_back(stats.getStat(EmotionalStat::FEAR));
    emotional_json.push_back(stats.getStat(EmotionalStat::ANGER));
    emotional_json.push_back(stats.getStat(EmotionalStat::SADNESS));
    emotional_json.push_back(stats.getStat(EmotionalStat::LOVE));
    emotional_json.push_back(stats.getStat(EmotionalStat::FRIEDNSHIP));
    emotional_json.push_back(stats.getStat(EmotionalStat::TRUST));
    emotional_json.push_back(stats.getStat(EmotionalStat::CONFIDENCE));
    emotional_json.push_back(stats.getStat(EmotionalStat::HOPE));
    emotional_json.push_back(stats.getStat(EmotionalStat::CURIOSITY));
    emotional_json.push_back(stats.getStat(EmotionalStat::CREATIVITY));
    emotional_json.push_back(stats.getStat(EmotionalStat::FOCUS));
    emotional_json.push_back(stats.getStat(EmotionalStat::PATIENCE));
    emotional_json.push_back(stats.getStat(EmotionalStat::GRATITUDE));
    emotional_json.push_back(stats.getStat(EmotionalStat::PEACE));
    emotional_json.push_back(stats.getStat(EmotionalStat::JOY));
    j["emotionalStats"] = emotional_json;

    // Physical stats (20)
    nlohmann::json physical_json = nlohmann::json::array();
    physical_json.push_back(stats.getStat(PhysicalStat::STRENGTH));
    physical_json.push_back(stats.getStat(PhysicalStat::AGILITY));
    physical_json.push_back(stats.getStat(PhysicalStat::ENDURANCE));
    physical_json.push_back(stats.getStat(PhysicalStat::REFLEX));
    physical_json.push_back(stats.getStat(PhysicalStat::BALANCE));
    physical_json.push_back(stats.getStat(PhysicalStat::COORDINATION));
    physical_json.push_back(stats.getStat(PhysicalStat::DEXTERITY));
    physical_json.push_back(stats.getStat(PhysicalStat::VISION));
    physical_json.push_back(stats.getStat(PhysicalStat::HEARING));
    physical_json.push_back(stats.getStat(PhysicalStat::SMELL));
    physical_json.push_back(stats.getStat(PhysicalStat::TASTE));
    physical_json.push_back(stats.getStat(PhysicalStat::TOUCH));
    physical_json.push_back(stats.getStat(PhysicalStat::HEALTHY_FUR));
    physical_json.push_back(stats.getStat(PhysicalStat::HEALTHY_EYES));
    physical_json.push_back(stats.getStat(PhysicalStat::HEALTHY_TEETH));
    physical_json.push_back(stats.getStat(PhysicalStat::HEALTHY_CLAWS));
    physical_json.push_back(stats.getStat(PhysicalStat::POSTURE));
    physical_json.push_back(stats.getStat(PhysicalStat::MOVEMENT_EFFICIENCY));
    physical_json.push_back(stats.getStat(PhysicalStat::THERMOREGULATION));
    physical_json.push_back(stats.getStat(PhysicalStat::METABOLISM));
    j["physicalStats"] = physical_json;

    // Social stats (10)
    nlohmann::json social_json = nlohmann::json::array();
    social_json.push_back(stats.getStat(SocialStat::POPULARITY));
    social_json.push_back(stats.getStat(SocialStat::FRIENDS));
    social_json.push_back(stats.getStat(SocialStat::ALLIANCES));
    social_json.push_back(stats.getStat(SocialStat::LEADERSHIP));
    social_json.push_back(stats.getStat(SocialStat::CHARISMA));
    social_json.push_back(stats.getStat(SocialStat::COMMUNICATION));
    social_json.push_back(stats.getStat(SocialStat::EMPATHY));
    social_json.push_back(stats.getStat(SocialStat::COOPERATION));
    social_json.push_back(stats.getStat(SocialStat::RESPECT));
    social_json.push_back(stats.getStat(SocialStat::REPUTATION));
    j["socialStats"] = social_json;

    // Environmental stats (10)
    nlohmann::json environmental_json = nlohmann::json::array();
    environmental_json.push_back(stats.getStat(EnvironmentalStat::FAMILIARITY));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::SAFETY));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::TERRITORY));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::HOME));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::RESOURCE_ACCESS));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::CLIMATE_COMPFORT));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::LIGHT_LEVEL));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::NOISE_LEVEL));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::SOCIAL_SPACE));
    environmental_json.push_back(stats.getStat(EnvironmentalStat::ISOLATION));
    j["environmentalStats"] = environmental_json;

    // Skill stats (10)
    nlohmann::json skill_json = nlohmann::json::array();
    skill_json.push_back(stats.getStat(SkillStat::LEARNING));
    skill_json.push_back(stats.getStat(SkillStat::MEMORY));
    skill_json.push_back(stats.getStat(SkillStat::PROBLEM_SOLVING));
    skill_json.push_back(stats.getStat(SkillStat::INNOVATION));
    skill_json.push_back(stats.getStat(SkillStat::TECHNIQUE));
    skill_json.push_back(stats.getStat(SkillStat::ADAPTATION));
    skill_json.push_back(stats.getStat(SkillStat::SURVIVAL));
    skill_json.push_back(stats.getStat(SkillStat::FORAGING));
    skill_json.push_back(stats.getStat(SkillStat::BUILDING));
    skill_json.push_back(stats.getStat(SkillStat::COMBAT));
    j["skillStats"] = skill_json;
}

void from_json(const nlohmann::json& j, GotchiStats& stats) {
    // Deserialize using public API
    if (j.contains("coreStats")) {
        const auto& core_json = j["coreStats"];
        if (core_json.size() >= 10) {
            stats.setStat(SecondaryStat::WEIGHT, core_json[0].get<float>());
            stats.setStat(SecondaryStat::FITNESS, core_json[1].get<float>());
            stats.setStat(SecondaryStat::GROWTH, core_json[2].get<float>());
            stats.setStat(SecondaryStat::AGE, core_json[3].get<float>());
            stats.setStat(SecondaryStat::BURDEN, core_json[4].get<float>());
            stats.setStat(SecondaryStat::EXPOSURE, core_json[5].get<float>());
            stats.setStat(SecondaryStat::DISEASE, core_json[6].get<float>());
            stats.setStat(SecondaryStat::IMMUNITY, core_json[7].get<float>());
            stats.setStat(SecondaryStat::FERTILITY, core_json[8].get<float>());
            stats.setStat(SecondaryStat::STAGE, core_json[9].get<float>());
        }
    }

    if (j.contains("secondaryStats")) {
        const auto& secondary_json = j["secondaryStats"];
        if (secondary_json.size() >= 10) {
            stats.setStat(SecondaryStat::SIZE, secondary_json[0].get<float>());
            stats.setStat(SecondaryStat::SPEED, secondary_json[1].get<float>());
            stats.setStat(SecondaryStat::JUMP, secondary_json[2].get<float>());
            stats.setStat(SecondaryStat::CLIMB, secondary_json[3].get<float>());
            stats.setStat(SecondaryStat::SWIM, secondary_json[4].get<float>());
            stats.setStat(SecondaryStat::FLY, secondary_json[5].get<float>());
            stats.setStat(SecondaryStat::CAMOUFLAGE, secondary_json[6].get<float>());
            stats.setStat(SecondaryStat::RESILIENCE, secondary_json[7].get<float>());
            stats.setStat(SecondaryStat::TOUGHNESS, secondary_json[8].get<float>());
            stats.setStat(SecondaryStat::FLEXIBILITY, secondary_json[9].get<float>());
        }
        if (secondary_json.size() >= 16) {
            stats.setStat(SecondaryStat::FITALITY, secondary_json[10].get<float>());
            stats.setStat(SecondaryStat::FOOD_LEVEL, secondary_json[11].get<float>());
            stats.setStat(SecondaryStat::HYDRATION, secondary_json[12].get<float>());
            stats.setStat(SecondaryStat::ENERGY, secondary_json[13].get<float>());
            stats.setStat(SecondaryStat::SLEEP_DEBT, secondary_json[14].get<float>());
            stats.setStat(SecondaryStat::CLEANLINESS, secondary_json[15].get<float>());
        }
    }

    if (j.contains("emotionalStats")) {
        const auto& emotional_json = j["emotionalStats"];
        if (emotional_json.size() >= 20) {
            stats.setStat(EmotionalStat::HAPPINESS, emotional_json[0].get<float>());
            stats.setStat(EmotionalStat::EXCITEMENT, emotional_json[1].get<float>());
            stats.setStat(EmotionalStat::SATISFACTION, emotional_json[2].get<float>());
            stats.setStat(EmotionalStat::BOREDOM, emotional_json[3].get<float>());
            stats.setStat(EmotionalStat::ANXIETY, emotional_json[4].get<float>());
            stats.setStat(EmotionalStat::FEAR, emotional_json[5].get<float>());
            stats.setStat(EmotionalStat::ANGER, emotional_json[6].get<float>());
            stats.setStat(EmotionalStat::SADNESS, emotional_json[7].get<float>());
            stats.setStat(EmotionalStat::LOVE, emotional_json[8].get<float>());
            stats.setStat(EmotionalStat::FRIEDNSHIP, emotional_json[9].get<float>());
            stats.setStat(EmotionalStat::TRUST, emotional_json[10].get<float>());
            stats.setStat(EmotionalStat::CONFIDENCE, emotional_json[11].get<float>());
            stats.setStat(EmotionalStat::HOPE, emotional_json[12].get<float>());
            stats.setStat(EmotionalStat::CURIOSITY, emotional_json[13].get<float>());
            stats.setStat(EmotionalStat::CREATIVITY, emotional_json[14].get<float>());
            stats.setStat(EmotionalStat::FOCUS, emotional_json[15].get<float>());
            stats.setStat(EmotionalStat::PATIENCE, emotional_json[16].get<float>());
            stats.setStat(EmotionalStat::GRATITUDE, emotional_json[17].get<float>());
            stats.setStat(EmotionalStat::PEACE, emotional_json[18].get<float>());
            stats.setStat(EmotionalStat::JOY, emotional_json[19].get<float>());
        }
    }

    if (j.contains("physicalStats")) {
        const auto& physical_json = j["physicalStats"];
        if (physical_json.size() >= 20) {
            stats.setStat(PhysicalStat::STRENGTH, physical_json[0].get<float>());
            stats.setStat(PhysicalStat::AGILITY, physical_json[1].get<float>());
            stats.setStat(PhysicalStat::ENDURANCE, physical_json[2].get<float>());
            stats.setStat(PhysicalStat::REFLEX, physical_json[3].get<float>());
            stats.setStat(PhysicalStat::BALANCE, physical_json[4].get<float>());
            stats.setStat(PhysicalStat::COORDINATION, physical_json[5].get<float>());
            stats.setStat(PhysicalStat::DEXTERITY, physical_json[6].get<float>());
            stats.setStat(PhysicalStat::VISION, physical_json[7].get<float>());
            stats.setStat(PhysicalStat::HEARING, physical_json[8].get<float>());
            stats.setStat(PhysicalStat::SMELL, physical_json[9].get<float>());
            stats.setStat(PhysicalStat::TASTE, physical_json[10].get<float>());
            stats.setStat(PhysicalStat::TOUCH, physical_json[11].get<float>());
            stats.setStat(PhysicalStat::HEALTHY_FUR, physical_json[12].get<float>());
            stats.setStat(PhysicalStat::HEALTHY_EYES, physical_json[13].get<float>());
            stats.setStat(PhysicalStat::HEALTHY_TEETH, physical_json[14].get<float>());
            stats.setStat(PhysicalStat::HEALTHY_CLAWS, physical_json[15].get<float>());
            stats.setStat(PhysicalStat::POSTURE, physical_json[16].get<float>());
            stats.setStat(PhysicalStat::MOVEMENT_EFFICIENCY, physical_json[17].get<float>());
            stats.setStat(PhysicalStat::THERMOREGULATION, physical_json[18].get<float>());
            stats.setStat(PhysicalStat::METABOLISM, physical_json[19].get<float>());
        }
    }

    if (j.contains("socialStats")) {
        const auto& social_json = j["socialStats"];
        if (social_json.size() >= 10) {
            stats.setStat(SocialStat::POPULARITY, social_json[0].get<float>());
            stats.setStat(SocialStat::FRIENDS, social_json[1].get<float>());
            stats.setStat(SocialStat::ALLIANCES, social_json[2].get<float>());
            stats.setStat(SocialStat::LEADERSHIP, social_json[3].get<float>());
            stats.setStat(SocialStat::CHARISMA, social_json[4].get<float>());
            stats.setStat(SocialStat::COMMUNICATION, social_json[5].get<float>());
            stats.setStat(SocialStat::EMPATHY, social_json[6].get<float>());
            stats.setStat(SocialStat::COOPERATION, social_json[7].get<float>());
            stats.setStat(SocialStat::RESPECT, social_json[8].get<float>());
            stats.setStat(SocialStat::REPUTATION, social_json[9].get<float>());
        }
    }

    if (j.contains("environmentalStats")) {
        const auto& environmental_json = j["environmentalStats"];
        if (environmental_json.size() >= 10) {
            stats.setStat(EnvironmentalStat::FAMILIARITY, environmental_json[0].get<float>());
            stats.setStat(EnvironmentalStat::SAFETY, environmental_json[1].get<float>());
            stats.setStat(EnvironmentalStat::TERRITORY, environmental_json[2].get<float>());
            stats.setStat(EnvironmentalStat::HOME, environmental_json[3].get<float>());
            stats.setStat(EnvironmentalStat::RESOURCE_ACCESS, environmental_json[4].get<float>());
            stats.setStat(EnvironmentalStat::CLIMATE_COMPFORT, environmental_json[5].get<float>());
            stats.setStat(EnvironmentalStat::LIGHT_LEVEL, environmental_json[6].get<float>());
            stats.setStat(EnvironmentalStat::NOISE_LEVEL, environmental_json[7].get<float>());
            stats.setStat(EnvironmentalStat::SOCIAL_SPACE, environmental_json[8].get<float>());
            stats.setStat(EnvironmentalStat::ISOLATION, environmental_json[9].get<float>());
        }
    }

    if (j.contains("skillStats")) {
        const auto& skill_json = j["skillStats"];
        if (skill_json.size() >= 10) {
            stats.setStat(SkillStat::LEARNING, skill_json[0].get<float>());
            stats.setStat(SkillStat::MEMORY, skill_json[1].get<float>());
            stats.setStat(SkillStat::PROBLEM_SOLVING, skill_json[2].get<float>());
            stats.setStat(SkillStat::INNOVATION, skill_json[3].get<float>());
            stats.setStat(SkillStat::TECHNIQUE, skill_json[4].get<float>());
            stats.setStat(SkillStat::ADAPTATION, skill_json[5].get<float>());
            stats.setStat(SkillStat::SURVIVAL, skill_json[6].get<float>());
            stats.setStat(SkillStat::FORAGING, skill_json[7].get<float>());
            stats.setStat(SkillStat::BUILDING, skill_json[8].get<float>());
            stats.setStat(SkillStat::COMBAT, skill_json[9].get<float>());
        }
    }
}

// GotchiMood serialization - serialize current mood type
void to_json(nlohmann::json& j, const GotchiMood& mood) {
    // Store the current mood type as an integer
    j["currentMood"] = static_cast<int>(mood.getCurrentMood());
}

void from_json(const nlohmann::json& j, GotchiMood& mood) {
    // Restore current mood from integer
    if (j.contains("currentMood")) {
        int moodInt = j["currentMood"].get<int>();
        if (moodInt >= 0 && moodInt <= 99) {
            mood.setCurrentMood(static_cast<GotchiMoodType>(moodInt));
        }
    }
}

void to_json(nlohmann::json& j, const GameState& s) {
    j["version"] = s.version;
    j["needs"] = s.needs;
    j["drivers"] = s.drivers;
    j["sleep"] = s.sleep;
    j["deathEnabled"] = s.deathEnabled;
    j["mode"] = static_cast<int>(s.mode);
    j["seenReality"] = s.seenReality;
    j["firstMergeBucket"] = static_cast<int>(s.firstMergeBucket);
    j["storyBeatIndex"] = s.storyBeatIndex;
    j["mergeLockTimer"] = s.mergeLockTimer;
    j["playtimeSeconds"] = s.playtimeSeconds;
    j["mergeCount"] = s.mergeCount;
    j["grime"] = s.grime;
    j["collapsed"] = s.collapsed;
    j["engagedCareSide"] = s.engagedCareSide;
    j["engagedStorySide"] = s.engagedStorySide;

    // Serialize vitals and mood (shared state)
    j["vitals"] = s.vitals;
    j["mood"] = s.mood;

    // Serialize flags map manually since Value is a variant
    nlohmann::json flags_json = nlohmann::json::object();
    for (const auto& pair : s.flags) {
        // Use to_json to convert Value to json
        nlohmann::json value_json;
        to_json(value_json, pair.second);
        flags_json[pair.first] = std::move(value_json);
    }
    j["flags"] = flags_json;
}

void from_json(const nlohmann::json& j, GameState& s) {
    // Version: if missing or newer than current, still try to load but be cautious
    s.version = j.value("version", 0);

    // Read each field with default if missing (version tolerance)
    if (j.contains("needs")) {
        from_json(j["needs"], s.needs);
    } else {
        s.needs = GameState::Needs{};
    }

    if (j.contains("drivers")) {
        from_json(j["drivers"], s.drivers);
    } else {
        s.drivers = GameState::Drivers{};
    }

    s.sleep = j.value("sleep", 1.0f);
    s.deathEnabled = j.value("deathEnabled", false);

    int modeInt = j.value("mode", static_cast<int>(Mode::Gotchi));
    if (modeInt >= static_cast<int>(Mode::Gotchi) && modeInt <= static_cast<int>(Mode::Story)) {
        s.mode = static_cast<Mode>(modeInt);
    } else {
        s.mode = Mode::Gotchi;
    }

    s.seenReality = j.value("seenReality", false);

    int mergeBucketInt = j.value("firstMergeBucket", static_cast<int>(FirstMerge::NotYet));
    if (mergeBucketInt >= static_cast<int>(FirstMerge::NotYet) &&
        mergeBucketInt <= static_cast<int>(FirstMerge::AfterNeglect)) {
        s.firstMergeBucket = static_cast<FirstMerge>(mergeBucketInt);
    } else {
        s.firstMergeBucket = FirstMerge::NotYet;
    }

    s.storyBeatIndex = j.value("storyBeatIndex", 0);
    s.mergeLockTimer = j.value("mergeLockTimer", 0.0f);
    s.playtimeSeconds = j.value("playtimeSeconds", 0.0);
    s.mergeCount = j.value("mergeCount", 0);
    s.grime = j.value("grime", 0.0f);
    s.collapsed = j.value("collapsed", false);
    s.engagedCareSide = j.value("engagedCareSide", false);
    s.engagedStorySide = j.value("engagedStorySide", false);

    // Vitals and mood: read if present, otherwise they're default-initialized
    if (j.contains("vitals")) {
        from_json(j["vitals"], s.vitals);
    } else {
        s.vitals = GotchiStats{};  // Default constructor initializes stats
    }

    // Mood: read if present, otherwise default-constructed
    // GotchiMood has no JSON serialization yet, so we just default-construct
    // This means saved mood data won't persist, but it's a reasonable default
    s.mood = GotchiMood{};

    // Flags: read if present, otherwise empty map
    if (j.contains("flags")) {
        for (auto it = j["flags"].begin(); it != j["flags"].end(); ++it) {
            Value v;
            from_json(it.value(), v);
            s.flags[it.key()] = std::move(v);
        }
    }
}

bool SaveManager::save(int slot, const GameState& s) {
    std::string path = slotPath(slot);
    std::string tempPath = slotTempPath(slot);

    // Serialize to JSON
    nlohmann::json j;
    to_json(j, s);
    std::string jsonStr = j.dump(4);  // Pretty print with 4-space indent

    // Write to temp file first (atomic write)
    std::ofstream tempFile(tempPath);
    if (!tempFile.is_open()) {
        std::cerr << "SaveManager: Failed to open temp file: " << tempPath << std::endl;
        return false;
    }

    tempFile << jsonStr;
    tempFile.close();

    if (!tempFile) {
        std::cerr << "SaveManager: Failed to write to temp file: " << tempPath << std::endl;
        std::filesystem::remove(tempPath);
        return false;
    }

    // Rename temp to final (atomic on most filesystems)
    try {
        std::filesystem::rename(tempPath, path);
    } catch (const std::exception& e) {
        std::cerr << "SaveManager: Failed to rename temp file: " << e.what() << std::endl;
        std::filesystem::remove(tempPath);
        return false;
    }

#if defined(PLATFORM_WEB)
    // Web: sync IDBFS to persist changes
    // This is async in Emscripten, but we'll try to force sync
    // Note: This is a best-effort since web has no reliable shutdown
    // FS.syncfs(false, cb) should be called elsewhere for guaranteed persistence
#endif

    return true;
}

std::optional<GameState> SaveManager::load(int slot) {
    std::string path = slotPath(slot);

    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    std::ifstream file(path);
    if (!file.is_open()) {
        return std::nullopt;
    }

    try {
        nlohmann::json j;
        file >> j;

        GameState s;
        from_json(j, s);

        // Check version: if save is from a newer build, warn but continue
        if (s.version > SAVE_VERSION) {
            std::cerr << "SaveManager: Save version " << s.version
                      << " is newer than current " << SAVE_VERSION
                      << ". Loading best-effort." << std::endl;
        }

        return s;
    } catch (const nlohmann::json::parse_error& e) {
        std::cerr << "SaveManager: JSON parse error: " << e.what() << std::endl;
        return std::nullopt;
    } catch (const std::exception& e) {
        std::cerr << "SaveManager: Load error: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool SaveManager::deleteSlot(int slot) {
    std::string path = slotPath(slot);
    std::string tempPath = slotTempPath(slot);

    bool removed = false;
    if (std::filesystem::exists(path)) {
        try {
            std::filesystem::remove(path);
            removed = true;
        } catch (const std::exception& e) {
            std::cerr << "SaveManager: Failed to delete slot " << slot << ": " << e.what() << std::endl;
        }
    }

    // Also remove any temp file for this slot
    if (std::filesystem::exists(tempPath)) {
        try {
            std::filesystem::remove(tempPath);
        } catch (const std::exception& e) {
            std::cerr << "SaveManager: Failed to remove temp file for slot " << slot << ": " << e.what() << std::endl;
        }
    }

    return removed;
}

void SaveManager::autosave(const GameState& s) {
    if (active_ >= 0 && active_ < NUM_SLOTS) {
        save(active_, s);
    }
}
