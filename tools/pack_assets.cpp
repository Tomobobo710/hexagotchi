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

#define RRES_IMPLEMENTATION
#include "rres.h"

#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <streambuf>

// Cross-platform directory walking:
// - POSIX (Linux/macOS/Cygwin): use dirent.h
// - Windows MSVC: use _findfirst/_findnext from io.h
#if defined(_WIN32) && !defined(__CYGWIN__)
#include <io.h>
#include <sys/stat.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif

// True for the file types we pack. Bytes are stored verbatim as RAWD chunks
// regardless of type -- PNGs decode on load via LoadImageFromMemory, audio via
// LoadWaveFromMemory. Keep this in sync with AssetPack's loaders.
static bool IsPackableAsset(const std::string& name) {
    auto endsWith = [&](const char* ext) {
        size_t n = strlen(ext);
        return name.size() > n && name.substr(name.size() - n) == ext;
    };
    return endsWith(".png") || endsWith(".mp3");
}

static std::vector<std::string> FindPngsRecursive(const std::string& root) {
    std::vector<std::string> found;
    std::vector<std::string> dirs = {root};

    while (!dirs.empty()) {
        std::string dir = dirs.back();
        dirs.pop_back();

#if defined(_WIN32) && !defined(__CYGWIN__)
        // Windows implementation using _findfirst/_findnext (avoids windows.h conflicts)
        std::string searchPattern = dir + "\\*";
        _finddata_t ffd;
        intptr_t hFind = _findfirst(searchPattern.c_str(), &ffd);
        if (hFind == -1) continue;

        do {
            std::string name = ffd.name;
            if (name == "." || name == "..") continue;
            std::string full = dir + "\\" + name;

            // Check if it's a directory
            if (ffd.attrib & _A_SUBDIR) {
                dirs.push_back(full);
            } else if (IsPackableAsset(name)) {
                found.push_back(full);
            }
        } while (_findnext(hFind, &ffd) == 0);

        _findclose(hFind);
#else
        // POSIX implementation using opendir/readdir
        DIR* d = opendir(dir.c_str());
        if (!d) continue;
        struct dirent* entry;
        while ((entry = readdir(d)) != nullptr) {
            std::string name = entry->d_name;
            if (name == "." || name == "..") continue;
            std::string full = dir + "/" + name;
            struct stat st;
            if (stat(full.c_str(), &st) != 0) continue;
            if (S_ISDIR(st.st_mode)) {
                dirs.push_back(full);
            } else if (IsPackableAsset(name)) {
                found.push_back(full);
            }
        }
        closedir(d);
#endif
    }

    // Sort the found files for deterministic output
    std::sort(found.begin(), found.end());

    return found;
}

static std::string ToRelativeKey(const std::string& fullPath, const std::string& root) {
    std::string rel = fullPath.substr(root.size());
    while (!rel.empty() && (rel[0] == '/' || rel[0] == '\\')) rel.erase(0, 1);
#if defined(_WIN32) && !defined(__CYGWIN__)
    // On Windows, normalize backslashes to forward slashes for consistency
    for (char& c : rel) if (c == '\\') c = '/';
#endif
    return rel;
}

// Builds one continuous RAWD chunk data buffer: propCount(=1), props[0]=byte
// count, then the raw file bytes. We store the original *.png bytes verbatim --
// PNG is already compressed, so this keeps the pack ~PNG-sized (~10MB) instead
// of exploding to raw RGBA (~46MB). The loader decodes each PNG on demand via
// LoadImageFromMemory, exactly like a normal LoadTexture, so nothing sits
// decoded in memory except the textures actually in use. (The old approach
// decoded to R8G8B8A8 at pack time and stored raw pixels, which both bloated
// the file AND forced the whole pack's worth of pixels into the wasm heap on
// web -> Aborted(OOM).) rres data type: RRES_DATA_RAW / FourCC "RAWD".
static unsigned char* BuildRawChunkBuffer(const unsigned char* fileData, unsigned int fileSize, unsigned int* outBufSize) {
    unsigned int propCount = 1;
    unsigned int bufSize = (propCount + 1) * sizeof(unsigned int) + fileSize;
    unsigned char* buffer = (unsigned char*)RRES_CALLOC(bufSize, 1);

    unsigned int props[1] = { fileSize };

    memcpy(buffer, &propCount, sizeof(unsigned int));
    memcpy(buffer + sizeof(unsigned int), props, propCount * sizeof(unsigned int));
    memcpy(buffer + (propCount + 1) * sizeof(unsigned int), fileData, fileSize);

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
        // Store the raw *.png file bytes verbatim (no decode). PNG is already
        // compressed; decoding to RGBA here is what bloated the pack and OOM'd
        // web. The loader decodes on demand via LoadImageFromMemory.
        // Use standard C++ file I/O instead of raylib to avoid X11 dependencies
        std::ifstream file(fullPath, std::ios::binary | std::ios::ate);
        if (!file) {
            fprintf(stderr, "Skipping (failed to load): %s\n", fullPath.c_str());
            continue;
        }
        std::streamsize fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        if (fileSize <= 0) {
            fprintf(stderr, "Skipping (empty file): %s\n", fullPath.c_str());
            continue;
        }
        std::vector<unsigned char> fileData(fileSize);
        if (!file.read(reinterpret_cast<char*>(fileData.data()), fileSize)) {
            fprintf(stderr, "Skipping (read failed): %s\n", fullPath.c_str());
            continue;
        }

        std::string key = ToRelativeKey(fullPath, assetsDir);

        unsigned int bufSize = 0;
        unsigned char* buffer = BuildRawChunkBuffer(fileData.data(), (unsigned int)fileSize, &bufSize);

        rresResourceChunkInfo chunkInfo = {0};
        chunkInfo.type[0] = 'R'; chunkInfo.type[1] = 'A'; chunkInfo.type[2] = 'W'; chunkInfo.type[3] = 'D';
        chunkInfo.id = rresComputeCRC32((unsigned char*)key.c_str(), (int)key.size());
        chunkInfo.compType = RRES_COMP_NONE;   // PNG already compressed; leave as-is
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

        printf("  packed: %s (id=%u) %d bytes (png)\n", key.c_str(), chunkInfo.id, (int)fileSize);
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
