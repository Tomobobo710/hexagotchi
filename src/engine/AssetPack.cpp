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

    // Chunks are RAWD-type: the original PNG file bytes stored verbatim (see
    // tools/pack_assets.cpp). Pull the bytes out and decode the PNG on demand --
    // this keeps only in-use textures decoded in memory, instead of the whole
    // pack's worth of raw RGBA (which OOM'd the web build's heap).
    unsigned int dataSize = 0;
    unsigned char* pngBytes = (unsigned char*)LoadDataFromResource(chunk, &dataSize);
    rresUnloadResourceChunk(chunk);

    if (pngBytes == nullptr || dataSize == 0) {
        TraceLog(LOG_WARNING, "ASSETPACK decode fail (no data): key='%s'", key.c_str());
        if (pngBytes) UnloadFileData(pngBytes);
        return Texture2D{0};
    }

    Image img = LoadImageFromMemory(".png", pngBytes, (int)dataSize);
    UnloadFileData(pngBytes);

    if (img.data == nullptr) {
        TraceLog(LOG_WARNING, "ASSETPACK decode fail (bad png): key='%s'", key.c_str());
        return Texture2D{0};
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

std::vector<Texture2D> loadFrames(const std::string& dir, const std::string& prefix, int digits) {
    std::vector<Texture2D> frames;
    char numbuf[16];
    char fmt[16];
    snprintf(fmt, sizeof(fmt), "%%0%dd", digits);

    // Try to find the first valid frame index
    // Some asset sequences start at index 0, others at index 1
    int startIdx = 0;

    // Check if index 0 exists
    snprintf(numbuf, sizeof(numbuf), fmt, startIdx);
    std::string key0 = dir + "/" + prefix + "_" + numbuf + ".png";
    Texture2D tex0 = loadTexture(key0);

    if (tex0.id == 0) {
        // Index 0 not found, try index 1 as start
        startIdx = 1;
        snprintf(numbuf, sizeof(numbuf), fmt, startIdx);
        std::string key1 = dir + "/" + prefix + "_" + numbuf + ".png";
        Texture2D tex1 = loadTexture(key1);

        if (tex1.id == 0) {
            // Neither index 0 nor 1 exists, return empty
            return frames;
        }

        // Add index 1 and continue
        frames.push_back(tex1);
    } else {
        // Index 0 exists, add it
        frames.push_back(tex0);
    }

    // Continue from the next index after startIdx
    for (int i = startIdx + 1; ; i++) {
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
