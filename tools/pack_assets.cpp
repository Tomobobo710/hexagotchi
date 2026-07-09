// tools/pack_assets.cpp - builds assets.rres from the assets/ directory.
//
// Bypasses rres's official rrespacker GUI tool (a separate itch.io download)
// by writing directly to the open rres 1.0 file format using rres.h's public
// structs, following the exact layout from rres/examples/rres_create_file.c.
// The result loads normally through rres-raylib.h's LoadImageFromResource() --
// this is a from-scratch writer for the *format*, not a reimplementation of
// their tool.
//
// Every *.png under assets/ becomes one IMGE resource chunk, keyed by its
// path relative to assets/ (e.g. "gotchis/001/idle_00.png"), so lookups in
// AssetPack use the same relative paths SpriteLoader/etc. already use.
//
// Build/run: see tools/pack_assets.sh

#include "raylib.h"
#define RRES_IMPLEMENTATION
#include "rres.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>

#include <dirent.h>
#include <sys/stat.h>

// POSIX directory walk (works under Cygwin/Linux/macOS). Deliberately avoids
// <windows.h> here -- pulling it into this Cygwin-targeted build previously
// caused the linker to expect a WinMain entry point instead of plain main().
static std::vector<std::string> FindPngsRecursive(const std::string& root) {
    std::vector<std::string> found;
    std::vector<std::string> dirs = {root};

    while (!dirs.empty()) {
        std::string dir = dirs.back();
        dirs.pop_back();

        DIR* d = opendir(dir.c_str());
        if (!d) continue;
        struct dirent* entry;
        std::vector<std::string> entries;
        while ((entry = readdir(d)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            std::string full = dir + "/" + name;
            struct stat st;
            if (stat(full.c_str(), &st) != 0) continue;
            if (S_ISDIR(st.st_mode)) {
                dirs.push_back(full);
            } else if (name.size() > 4 && name.substr(name.size() - 4) == ".png") {
                found.push_back(full);
            }
        }
        closedir(d);
    }

    // Sort the found files for deterministic output
    std::sort(found.begin(), found.end());

    return found;
}

static std::string ToRelativeKey(const std::string& fullPath, const std::string& root) {
    std::string rel = fullPath.substr(root.size());
    while (!rel.empty() && (rel[0] == '/' || rel[0] == '\\')) rel.erase(0, 1);
    for (char& c : rel) if (c == '\\') c = '/';
    return rel;
}

// Builds one continuous IMGE chunk data buffer: propCount, props[], raw pixels.
// Mirrors rres_create_file.c's LoadDataBuffer() exactly.
static unsigned char* BuildImageChunkBuffer(const Image& img, unsigned int rawSize, unsigned int* outBufSize) {
    unsigned int propCount = 4;
    unsigned int bufSize = (propCount + 1) * sizeof(unsigned int) + rawSize;
    unsigned char* buffer = (unsigned char*)RRES_CALLOC(bufSize, 1);

    unsigned int props[4] = {
        (unsigned int)img.width,
        (unsigned int)img.height,
        (unsigned int)img.format,
        (unsigned int)img.mipmaps
    };

    memcpy(buffer, &propCount, sizeof(unsigned int));
    memcpy(buffer + sizeof(unsigned int), props, propCount * sizeof(unsigned int));
    memcpy(buffer + (propCount + 1) * sizeof(unsigned int), img.data, rawSize);

    *outBufSize = bufSize;
    return buffer;
}

int main(int argc, char** argv) {
    std::string assetsDir = (argc > 1) ? argv[1] : "assets";
    std::string outPath = (argc > 2) ? argv[2] : "assets.rres";

    std::vector<std::string> pngs = FindPngsRecursive(assetsDir);
    if (pngs.empty()) {
        fprintf(stderr, "No PNGs found under %s\n", assetsDir.c_str());
        return 1;
    }

    FILE* rresFile = fopen(outPath.c_str(), "wb");
    if (!rresFile) {
        fprintf(stderr, "Could not open %s for writing\n", outPath.c_str());
        return 1;
    }

    rresFileHeader header = {0};
    header.id[0] = 'r'; header.id[1] = 'r'; header.id[2] = 'e'; header.id[3] = 's';
    header.version = 100;
    header.chunkCount = (unsigned short)pngs.size();
    header.cdOffset = 0;
    header.reserved = 0;
    fwrite(&header, sizeof(rresFileHeader), 1, rresFile);

    int packed = 0;
    for (const std::string& fullPath : pngs) {
        Image img = LoadImage(fullPath.c_str());
        if (img.data == nullptr) {
            fprintf(stderr, "Skipping (failed to load): %s\n", fullPath.c_str());
            continue;
        }

        // Convert to RGBA format (8 bits per channel) for compatibility with AssetPack
        ImageFormat(&img, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);

        unsigned int rawSize = (unsigned int)GetPixelDataSize(img.width, img.height, img.format);
        std::string key = ToRelativeKey(fullPath, assetsDir);

        unsigned int bufSize = 0;
        unsigned char* buffer = BuildImageChunkBuffer(img, rawSize, &bufSize);

        rresResourceChunkInfo chunkInfo = {0};
        chunkInfo.type[0] = 'I'; chunkInfo.type[1] = 'M'; chunkInfo.type[2] = 'G'; chunkInfo.type[3] = 'E';
        chunkInfo.id = rresComputeCRC32((unsigned char*)key.c_str(), (int)key.size());
        chunkInfo.compType = RRES_COMP_NONE;
        chunkInfo.cipherType = RRES_CIPHER_NONE;
        chunkInfo.flags = 0;
        chunkInfo.baseSize = bufSize;
        chunkInfo.packedSize = bufSize;
        chunkInfo.nextOffset = 0;
        chunkInfo.reserved = 0;
        chunkInfo.crc32 = rresComputeCRC32(buffer, bufSize);

        fwrite(&chunkInfo, sizeof(rresResourceChunkInfo), 1, rresFile);
        fwrite(buffer, 1, bufSize, rresFile);

        RRES_FREE(buffer);
        UnloadImage(img);

        printf("  packed: %s (id=%u)\n", key.c_str(), chunkInfo.id);
        packed++;
    }

    fclose(rresFile);
    printf("Wrote %s: %d resources\n", outPath.c_str(), packed);

    // Plain-text manifest alongside the .rres -- rres itself has no directory
    // listing (lookups are by CRC32 of the key, one-way), so anything that
    // needs to enumerate "every packed asset" (e.g. tools/scene_editor.cpp's
    // asset browser) reads this instead of the binary pack.
    std::string manifestPath = outPath;
    size_t dot = manifestPath.find_last_of('.');
    if (dot != std::string::npos) manifestPath = manifestPath.substr(0, dot);
    manifestPath += "_manifest.txt";

    FILE* manifestFile = fopen(manifestPath.c_str(), "w");
    if (manifestFile) {
        for (const std::string& fullPath : pngs) {
            fprintf(manifestFile, "%s\n", ToRelativeKey(fullPath, assetsDir).c_str());
        }
        fclose(manifestFile);
        printf("Wrote %s: %d entries\n", manifestPath.c_str(), (int)pngs.size());
    }

    return 0;
}
