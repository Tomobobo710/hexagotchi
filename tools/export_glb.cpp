// Exports a hand-built Mesh (position/normal/vertex-color, no UVs/textures)
// to a binary glTF (.glb) file, for round-tripping through Blender:
//   1. Run this tool -- writes build/export/<name>.glb
//   2. Open/edit in Blender (vertex-color paint, reshape, whatever)
//   3. Export back out of Blender as .glb
//   4. Hand the result back to Claude, who reads its vertex/normal/color
//      arrays and rewrites them as PushTri/PushQuad calls in the relevant
//      BuildXMesh() function -- there is no runtime glTF loader in the game,
//      this is purely an offline geometry-editing round trip.
//
// Vertex colors only (COLOR_0), matching how every hand-built mesh in this
// project actually gets its color -- no UVs/materials/textures are written,
// since none of our meshes use them for anything Blender needs to see.
//
// Standalone tool, not linked into the game. Build:
//   g++ -std=c++17 -I src/effects -I src/engine -I raylib/src tools/export_glb.cpp -o build/export_glb.exe <raylib link flags>
// (see build-alt.sh / Makefile's LDFLAGS for the actual link line used here)

#include "raylib.h"
#include "VehicleMesh.hpp"
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

// Raw string concatenation for the JSON chunk -- this tool only ever
// exports one mesh at a time with a fixed, known structure, so a tiny JSON
// templating helper is simpler and more debuggable than pulling in a real
// JSON library for a one-shot offline tool.
static std::string BuildGltfJson(int vertexCount, size_t posByteLen, size_t posByteOffset,
                                  size_t normByteLen, size_t normByteOffset,
                                  size_t colorByteLen, size_t colorByteOffset,
                                  float minPos[3], float maxPos[3], size_t totalBufferLen) {
    char buf[4096];
    snprintf(buf, sizeof(buf), R"({
  "asset": { "version": "2.0", "generator": "hexagotchi export_glb" },
  "scene": 0,
  "scenes": [ { "nodes": [0] } ],
  "nodes": [ { "mesh": 0, "name": "ExportedMesh" } ],
  "meshes": [ {
    "primitives": [ {
      "attributes": { "POSITION": 0, "NORMAL": 1, "COLOR_0": 2 },
      "mode": 4
    } ]
  } ],
  "buffers": [ { "byteLength": %zu } ],
  "bufferViews": [
    { "buffer": 0, "byteOffset": %zu, "byteLength": %zu, "target": 34962 },
    { "buffer": 0, "byteOffset": %zu, "byteLength": %zu, "target": 34962 },
    { "buffer": 0, "byteOffset": %zu, "byteLength": %zu, "target": 34962 }
  ],
  "accessors": [
    { "bufferView": 0, "componentType": 5126, "count": %d, "type": "VEC3",
      "min": [%f, %f, %f], "max": [%f, %f, %f] },
    { "bufferView": 1, "componentType": 5126, "count": %d, "type": "VEC3" },
    { "bufferView": 2, "componentType": 5121, "normalized": true, "count": %d, "type": "VEC4" }
  ]
})",
        totalBufferLen,
        posByteOffset, posByteLen,
        normByteOffset, normByteLen,
        colorByteOffset, colorByteLen,
        vertexCount, minPos[0], minPos[1], minPos[2], maxPos[0], maxPos[1], maxPos[2],
        vertexCount,
        vertexCount);
    return std::string(buf);
}

static size_t AlignUp4(size_t n) { return (n + 3) & ~size_t(3); }

static void WriteGlb(const std::string& path, const Mesh& mesh) {
    int vcount = mesh.vertexCount;

    // POSITION + NORMAL as-is (already float32 vec3). COLOR_0 as normalized
    // unsigned byte vec4 (glTF's compact vertex-color format, and exactly
    // what raylib's Mesh.colors already stores -- straight copy, no
    // conversion needed).
    size_t posLen = (size_t)vcount * 3 * sizeof(float);
    size_t normLen = (size_t)vcount * 3 * sizeof(float);
    size_t colorLen = (size_t)vcount * 4 * sizeof(unsigned char);

    size_t posOffset = 0;
    size_t normOffset = AlignUp4(posOffset + posLen);
    size_t colorOffset = AlignUp4(normOffset + normLen);
    size_t totalLen = AlignUp4(colorOffset + colorLen);

    float minPos[3] = {1e30f, 1e30f, 1e30f};
    float maxPos[3] = {-1e30f, -1e30f, -1e30f};
    for (int i = 0; i < vcount; i++) {
        for (int c = 0; c < 3; c++) {
            float v = mesh.vertices[i * 3 + c];
            if (v < minPos[c]) minPos[c] = v;
            if (v > maxPos[c]) maxPos[c] = v;
        }
    }

    std::string json = BuildGltfJson(vcount, posLen, posOffset, normLen, normOffset,
                                      colorLen, colorOffset, minPos, maxPos, totalLen);
    // JSON chunk itself must also be 4-byte aligned, padded with spaces
    // (glTF spec requirement) rather than nulls, since it's parsed as text.
    size_t jsonPadded = AlignUp4(json.size());
    json.append(jsonPadded - json.size(), ' ');

    std::vector<uint8_t> bin(totalLen, 0);
    memcpy(bin.data() + posOffset, mesh.vertices, posLen);
    memcpy(bin.data() + normOffset, mesh.normals, normLen);
    memcpy(bin.data() + colorOffset, mesh.colors, colorLen);

    FILE* f = fopen(path.c_str(), "wb");
    if (!f) { fprintf(stderr, "failed to open %s for writing\n", path.c_str()); return; }

    uint32_t magic = 0x46546C67; // "glTF"
    uint32_t version = 2;
    uint32_t totalFileLen = 12 + (8 + (uint32_t)json.size()) + (8 + (uint32_t)bin.size());
    fwrite(&magic, 4, 1, f);
    fwrite(&version, 4, 1, f);
    fwrite(&totalFileLen, 4, 1, f);

    uint32_t jsonChunkLen = (uint32_t)json.size();
    uint32_t jsonChunkType = 0x4E4F534A; // "JSON"
    fwrite(&jsonChunkLen, 4, 1, f);
    fwrite(&jsonChunkType, 4, 1, f);
    fwrite(json.data(), 1, json.size(), f);

    uint32_t binChunkLen = (uint32_t)bin.size();
    uint32_t binChunkType = 0x004E4942; // "BIN\0"
    fwrite(&binChunkLen, 4, 1, f);
    fwrite(&binChunkType, 4, 1, f);
    fwrite(bin.data(), 1, bin.size(), f);

    fclose(f);
    printf("wrote %s (%d verts, %zu bytes)\n", path.c_str(), vcount, totalFileLen);
}

int main(int argc, char** argv) {
    // BuildRingFrameMesh/BuildMembraneMesh call UploadMesh(), which needs a
    // live GL context -- there's no headless/context-free mesh path in
    // raylib, so open a real (if pointless-looking) window just to get one.
    SetTraceLogLevel(LOG_WARNING); // quiet raylib's usual per-frame INFO spam
    InitWindow(64, 64, "export_glb (temporary)");

    Mesh car = BuildVehicleMesh(false, Color{180, 40, 40, 255});
    WriteGlb("build/export/vehicle_car.glb", car);

    Mesh truck = BuildVehicleMesh(true, Color{200, 160, 40, 255});
    WriteGlb("build/export/vehicle_truck.glb", truck);

    CloseWindow();
    return 0;
}
