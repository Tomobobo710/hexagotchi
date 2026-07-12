#include "Config.hpp"
#include "GameConstants.hpp"
#include "GameState.h"
#include "../game/TutorialController.hpp"

#include <string>
#include <unordered_map>
#include <sstream>
#include <fstream>
#include <cstdlib>

#if defined(PLATFORM_WEB)
#include <emscripten.h>
#endif

// Keys stored in the config. Kept identical across both backends so a value
// written on one is readable if the storage is ever shared.
namespace {
    const char* K_MUSIC   = "music_volume";     // int 0..10
    const char* K_SFX     = "sfx_volume";        // int 0..10
    const char* K_DIALOG  = "dialog_speed";      // "off" | "normal" | "fast"
    const char* K_TUTSEEN = "tutorial_seen";     // "0" | "1"

    // Desktop config path: next to the exe's working dir, same convention as
    // assets.rres. (Web ignores this.)
    const char* INI_PATH = "settings.ini";

    std::string dialogSpeedToStr(DialogSpeed s) {
        switch (s) {
            case DialogSpeed::Off:  return "off";
            case DialogSpeed::Fast: return "fast";
            default:                return "normal";
        }
    }
    DialogSpeed dialogSpeedFromStr(const std::string& s) {
        if (s == "off")  return DialogSpeed::Off;
        if (s == "fast") return DialogSpeed::Fast;
        return DialogSpeed::Normal;
    }

    // ---- Backend: read all key/values into a map; write a map back out -------

#if defined(PLATFORM_WEB)
    // localStorage. Values are short strings keyed by "hexa_<key>".
    EM_JS(char*, js_get, (const char* key), {
        try {
            var v = window.localStorage.getItem("hexa_" + UTF8ToString(key));
            if (v === null) return 0;
            var len = lengthBytesUTF8(v) + 1;
            var buf = _malloc(len);
            stringToUTF8(v, buf, len);
            return buf;
        } catch (e) { return 0; }
    });
    EM_JS(void, js_set, (const char* key, const char* val), {
        try { window.localStorage.setItem("hexa_" + UTF8ToString(key), UTF8ToString(val)); }
        catch (e) {}
    });

    bool backendGet(const char* key, std::string& out) {
        char* v = js_get(key);
        if (!v) return false;
        out = v;
        free(v);
        return true;
    }
    void backendReadAll(std::unordered_map<std::string, std::string>& kv) {
        std::string v;
        if (backendGet(K_MUSIC,   v)) kv[K_MUSIC]   = v;
        if (backendGet(K_SFX,     v)) kv[K_SFX]     = v;
        if (backendGet(K_DIALOG,  v)) kv[K_DIALOG]  = v;
        if (backendGet(K_TUTSEEN, v)) kv[K_TUTSEEN] = v;
    }
    void backendWriteAll(const std::unordered_map<std::string, std::string>& kv) {
        for (const auto& p : kv) js_set(p.first.c_str(), p.second.c_str());
    }
#else
    // settings.ini -- one "key=value" per line, '#' comments ignored.
    void backendReadAll(std::unordered_map<std::string, std::string>& kv) {
        std::ifstream f(INI_PATH);
        if (!f) return;
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            auto eq = line.find('=');
            if (eq == std::string::npos) continue;
            std::string key = line.substr(0, eq);
            std::string val = line.substr(eq + 1);
            // Trim trailing CR (Windows line endings) / spaces.
            while (!val.empty() && (val.back() == '\r' || val.back() == ' ')) val.pop_back();
            while (!key.empty() && key.back() == ' ') key.pop_back();
            kv[key] = val;
        }
    }
    void backendWriteAll(const std::unordered_map<std::string, std::string>& kv) {
        std::ofstream f(INI_PATH, std::ios::trunc);
        if (!f) return;
        f << "# Hexagotchi settings. Auto-generated; edit values, not keys.\n";
        // Stable order for readability.
        auto put = [&](const char* k) {
            auto it = kv.find(k);
            if (it != kv.end()) f << k << "=" << it->second << "\n";
        };
        put(K_MUSIC);
        put(K_SFX);
        put(K_DIALOG);
        put(K_TUTSEEN);
    }
#endif

    // Builds the key/value map from the CURRENT live global values.
    std::unordered_map<std::string, std::string> snapshot() {
        std::unordered_map<std::string, std::string> kv;
        kv[K_MUSIC]   = std::to_string(GetMusicVolume());
        kv[K_SFX]     = std::to_string(GetSfxVolume());
        kv[K_DIALOG]  = dialogSpeedToStr(GetDialogSpeed());
        kv[K_TUTSEEN] = globalGameState.getBool(TutorialController::SEEN_FLAG, false) ? "1" : "0";
        return kv;
    }
}

namespace Config {

void Load() {
    std::unordered_map<std::string, std::string> kv;
    backendReadAll(kv);

    auto geti = [&](const char* k, int def) -> int {
        auto it = kv.find(k);
        if (it == kv.end()) return def;
        try { return std::stoi(it->second); } catch (...) { return def; }
    };

    SetMusicVolume(geti(K_MUSIC, VOLUME_LEVEL_DEFAULT));
    SetSfxVolume(geti(K_SFX, VOLUME_LEVEL_DEFAULT));

    auto itd = kv.find(K_DIALOG);
    SetDialogSpeed(itd != kv.end() ? dialogSpeedFromStr(itd->second) : DialogSpeed::Normal);

    auto itt = kv.find(K_TUTSEEN);
    if (itt != kv.end() && itt->second == "1") {
        globalGameState.setFlag(TutorialController::SEEN_FLAG, true);
    }
}

void Save() {
    backendWriteAll(snapshot());
}

void ResetToDefaults() {
    // Restore default SETTINGS only. tutorial_seen is intentionally left as-is
    // -- resetting settings shouldn't force a returning player back through the
    // tutorial.
    SetMusicVolume(VOLUME_LEVEL_DEFAULT);
    SetSfxVolume(VOLUME_LEVEL_DEFAULT);
    SetDialogSpeed(DialogSpeed::Normal);
    Save();
}

} // namespace Config
