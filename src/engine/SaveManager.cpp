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
