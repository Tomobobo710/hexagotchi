#include "SpriteTestScene.hpp"
#include "AssetPack.hpp"
#include "../effects/StarfieldEffect.hpp"
#include <dirent.h>
#include <algorithm>
#include <fstream>
#include <sstream>

// Key relative to assets/ (matches how tools/pack_assets.cpp keys resources
// inside assets.rres), not a filesystem path -- AssetPack reads packed
// resources by this key, never touching the assets/ directory directly.
static const char* GOTCHI_DIR = "gotchis/001";
static const float GOTCHI_FRAME_DURATION = 0.12f;
static const float GOTCHI_DISPLAY_SCALE = 4.0f;  // Source frames are 64x64; scale up for visibility

// Get all animation prefixes from the assets manifest file
static std::vector<std::string> getAvailableAnimations(const std::string& dir) {
    std::vector<std::string> animations;

    // Try multiple paths for the manifest file
    // Order matters - try most likely paths first
    std::vector<std::string> manifestPaths = {
        "assets_manifest.txt",              // In build/desktop (next to assets.rres)
        "../assets_manifest.txt",           // One level up (from build/desktop subdirs)
        "assets/assets_manifest.txt",       // Relative to project root (assets/)
        "build/desktop/assets_manifest.txt", // Alternative path relative to project root
    };

    std::ifstream manifest;
    std::string manifestPath;

    for (const auto& path : manifestPaths) {
        manifest.open(path);
        if (manifest.is_open()) {
            manifestPath = path;
            break;
        }
    }

    if (!manifest.is_open()) {
        // Fallback: try to read from filesystem directory
        std::string assetsPath = "assets/" + dir + "/";
        DIR* d = opendir(assetsPath.c_str());
        if (!d) {
            return animations;
        }

        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            std::string filename = entry->d_name;
            if (filename == "." || filename == "..") continue;

            size_t underscorePos = filename.find('_');
            if (underscorePos != std::string::npos) {
                std::string prefix = filename.substr(0, underscorePos);
                size_t suffixPos = prefix.rfind("_two");
                if (suffixPos != std::string::npos && suffixPos == prefix.length() - 4) {
                    prefix = prefix.substr(0, suffixPos);
                }
                suffixPos = prefix.rfind("_three");
                if (suffixPos != std::string::npos && suffixPos == prefix.length() - 6) {
                    prefix = prefix.substr(0, suffixPos);
                }

                if (std::find(animations.begin(), animations.end(), prefix) == animations.end()) {
                    animations.push_back(prefix);
                }
            }
        }
        closedir(d);
    } else {
        // Parse manifest file to extract animation prefixes
        std::string line;
        while (std::getline(manifest, line)) {
            std::string dirWithSlash = dir + "/";
            if (line.find(dirWithSlash) == 0) {
                std::string filename = line.substr(dir.length() + 1);
                size_t underscorePos = filename.find('_');
                if (underscorePos != std::string::npos) {
                    std::string prefix = filename.substr(0, underscorePos);
                    size_t suffixPos = prefix.rfind("_two");
                    if (suffixPos != std::string::npos && suffixPos == prefix.length() - 4) {
                        prefix = prefix.substr(0, suffixPos);
                    }
                    suffixPos = prefix.rfind("_three");
                    if (suffixPos != std::string::npos && suffixPos == prefix.length() - 6) {
                        prefix = prefix.substr(0, suffixPos);
                    }

                    if (std::find(animations.begin(), animations.end(), prefix) == animations.end()) {
                        animations.push_back(prefix);
                    }
                }
            }
        }
        manifest.close();
    }

    // Sort alphabetically
    std::sort(animations.begin(), animations.end());

    return animations;
}

SpriteTestScene::SpriteTestScene()
    : Scene(720.0f, 720.0f, {18, 18, 30, 255}) {
}

void SpriteTestScene::init() {
    getCamera()->setBoundary(0, 0, 720.0f, 720.0f);
    addEffect(new StarfieldEffect(getCamera()));

    // Load all available animations from the assets directory
    std::vector<std::string> availableAnims = getAvailableAnimations(GOTCHI_DIR);

    for (const std::string& action : availableAnims) {
        std::vector<Texture2D> frames = AssetPack::loadFrames(GOTCHI_DIR, action);
        if (!frames.empty()) {
            actionNames.push_back(action);
            animFrames[action] = frames;
        }
    }

    gotchi = new SceneActor({360.0f, 300.0f}, 64.0f, 64.0f);
    gotchi->setScale({GOTCHI_DISPLAY_SCALE, GOTCHI_DISPLAY_SCALE});
    gotchi->setClickable(true);
    gotchi->setOnClick([this]() {
        clickCount++;
        selectAction(selectedAction + 1);
    });
    addActor(gotchi);

    if (!actionNames.empty()) selectAction(0);
}

void SpriteTestScene::selectAction(int index) {
    if (actionNames.empty()) return;
    selectedAction = ((index % (int)actionNames.size()) + (int)actionNames.size()) % (int)actionNames.size();
    const std::string& name = actionNames[selectedAction];
    // All animations loop by default - user clicks to change to next animation
    gotchi->setAnimationFrames(animFrames[name], GOTCHI_FRAME_DURATION, true);
}

void SpriteTestScene::cleanup() {
    Scene::cleanup();
    for (auto& pair : animFrames) {
        AssetPack::unloadFrames(pair.second);
    }
    animFrames.clear();
}

void SpriteTestScene::update(float deltaTime) {
    Scene::update(deltaTime);
}

void SpriteTestScene::draw() {
    Scene::draw();

    DrawText("SPRITE TEST", 16, 8, 18, {180, 180, 255, 255});

    DrawText("Click character to cycle through animations", 16, 690, 12, {180, 180, 220, 255});

    char clickBuf[32];
    snprintf(clickBuf, sizeof(clickBuf), "Clicks: %d", clickCount);
    DrawText(clickBuf, 600, 40, 14, {180, 180, 220, 255});

    if (actionNames.empty()) {
        DrawText("No gotchi frames found in assets.rres (gotchis/001)", 16, 40, 14, {255, 120, 120, 255});
        return;
    }

    const std::string& name = actionNames[selectedAction];
    DrawText(("Action: " + name).c_str(), 16, 40, 16, {100, 200, 255, 255});

    char frameBuf[32];
    snprintf(frameBuf, sizeof(frameBuf), "Frame: %d / %d", gotchi->getFrame() + 1, (int)animFrames.at(name).size());
    DrawText(frameBuf, 16, 62, 14, {180, 180, 220, 255});

    // List all available actions, highlighting the active one.
    int ly = 100;
    DrawText("ACTIONS", 16, ly, 14, {100, 200, 255, 255});
    ly += 18;
    for (int i = 0; i < (int)actionNames.size(); i++) {
        Color c = (i == selectedAction) ? YELLOW : Color{180, 180, 220, 255};
        DrawText(actionNames[i].c_str(), 16, ly, 12, c);
        ly += 16;
    }
}
