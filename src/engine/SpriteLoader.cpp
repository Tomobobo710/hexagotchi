#include "SpriteLoader.hpp"
#include <cstdio>

namespace SpriteLoader {

std::vector<Texture2D> loadFrames(const std::string& dir, const std::string& prefix, int digits) {
    std::vector<Texture2D> frames;
    char numbuf[16];
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%0%dd", digits);

    for (int i = 0; ; i++) {
        snprintf(numbuf, sizeof(numbuf), fmt, i);
        std::string path = dir + "/" + prefix + "_" + numbuf + ".png";
        if (!FileExists(path.c_str())) break;

        Texture2D tex = LoadTexture(path.c_str());
        if (tex.id == 0) break;
        frames.push_back(tex);
    }

    return frames;
}

void unloadFrames(std::vector<Texture2D>& frames) {
    for (auto& tex : frames) {
        if (tex.id != 0) UnloadTexture(tex);
    }
    frames.clear();
}

}
