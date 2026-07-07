#include "AssetPack.hpp"

#define RRES_IMPLEMENTATION
#include "rres.h"
#define RRES_RAYLIB_IMPLEMENTATION
#include "rres-raylib.h"

#include <cstdio>

namespace AssetPack {

static std::string packFile = "assets.rres";

void setPackFile(const std::string& path) {
    packFile = path;
}

Texture2D loadTexture(const std::string& key) {
    unsigned int id = rresComputeCRC32((const unsigned char*)key.c_str(), (int)key.size());
    rresResourceChunk chunk = rresLoadResourceChunk(packFile.c_str(), id);

    if (chunk.data.raw == nullptr) {
        TraceLog(LOG_WARNING, "ASSETPACK miss: key='%s' id=%08X", key.c_str(), id);
        return Texture2D{0};
    }

    Image img = LoadImageFromResource(chunk);
    rresUnloadResourceChunk(chunk);

    if (img.data == nullptr) {
        return Texture2D{0};
    }

    Texture2D tex = LoadTextureFromImage(img);
    UnloadImage(img);
    return tex;
}

std::vector<Texture2D> loadFrames(const std::string& dir, const std::string& prefix, int digits) {
    std::vector<Texture2D> frames;
    char numbuf[16];
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%0%dd", digits);

    for (int i = 0; ; i++) {
        snprintf(numbuf, sizeof(numbuf), fmt, i);
        std::string key = dir + "/" + prefix + "_" + numbuf + ".png";

        Texture2D tex = loadTexture(key);
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
