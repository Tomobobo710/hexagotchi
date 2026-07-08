#include "GameState.h"

// Implement flag accessors
bool GameState::getBool(const std::string& key, bool def) const {
    auto it = flags.find(key);
    if (it == flags.end()) return def;
    if (std::holds_alternative<bool>(it->second)) {
        return std::get<bool>(it->second);
    }
    return def;
}

int GameState::getInt(const std::string& key, int def) const {
    auto it = flags.find(key);
    if (it == flags.end()) return def;
    if (std::holds_alternative<int>(it->second)) {
        return std::get<int>(it->second);
    }
    return def;
}

float GameState::getFloat(const std::string& key, float def) const {
    auto it = flags.find(key);
    if (it == flags.end()) return def;
    if (std::holds_alternative<float>(it->second)) {
        return std::get<float>(it->second);
    }
    return def;
}

std::string GameState::getStr(const std::string& key, std::string def) const {
    auto it = flags.find(key);
    if (it == flags.end()) return def;
    if (std::holds_alternative<std::string>(it->second)) {
        return std::get<std::string>(it->second);
    }
    return def;
}
