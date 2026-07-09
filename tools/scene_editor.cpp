// tools/scene_editor.cpp - standalone GUI tool for laying out Tom's
// world-scenes visually instead of hand-writing x/y coordinates.
//
// Not part of the game build and not loaded by the game at runtime -- this
// is purely a design aid. Workflow: pick an asset from the sidebar, click to
// place it as an actor, drag/scroll to position and scale it, Ctrl+S to
// export the layout as JSON. That JSON is read by a human (or handed to
// Claude) to hand-translate into real TomSceneActor construction calls in
// the actual scene .cpp -- the game never reads this file.
//
// Build: scene-editor.bat (wraps `make scene-editor`). Output lands in
// build/scene_editor/scene_editor.exe, alongside its own assets.rres copy --
// run it directly from there, or via Explorer.

#include "raylib.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <memory>

#define RRES_IMPLEMENTATION
#include "rres.h"
#define RRES_RAYLIB_IMPLEMENTATION
#include "rres-raylib.h"

// --- Config ----------------------------------------------------------------

static const int WINDOW_W = 3456;
static const int WINDOW_H = 1944;
static const int LEFT_SIDEBAR_W = 640;
static const int RIGHT_SIDEBAR_W = 480;
static const float WORLD_W = 1280.0f;
static const float WORLD_H = 720.0f;

// Own copy, sitting next to this exe (build/scene_editor/) -- copied there by
// the Makefile's scene-editor target, same convention as build/desktop/'s own
// assets.rres copy. Keeps this tool independent of the caller's cwd instead
// of assuming it's launched from the repo root.
static const char* MANIFEST_PATH = "assets_manifest.txt";
static const char* PACK_PATH = "assets.rres";

// --- Asset pack lookup (standalone copy of AssetPack::loadTexture, so this
// tool doesn't need to link the whole engine) --------------------------------

static Texture2D LoadPackedTexture(const std::string& key) {
    unsigned int id = rresComputeCRC32((const unsigned char*)key.c_str(), (int)key.size());
    rresResourceChunk chunk = rresLoadResourceChunk(PACK_PATH, id);
    if (chunk.data.raw == nullptr) return Texture2D{0};

    Image img = LoadImageFromResource(chunk);
    rresUnloadResourceChunk(chunk);
    if (img.data == nullptr) return Texture2D{0};

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_POINT);
    UnloadImage(img);
    return tex;
}

// --- Placed actor ------------------------------------------------------------

struct PlacedActor {
    std::string tag;
    std::string assetKey;
    Vector2 position;   // world-space, top-left
    float scale = 1.0f;
    int layer = 1;
    bool flipX = false;  // mirror the texture horizontally when drawn
    Texture2D texture = {0};
};

// --- Minimal JSON export/import (flat array of flat objects only -- no
// nesting needed for this schema) --------------------------------------------

static std::string JsonEscape(const std::string& s) {
    std::string out;
    for (char c : s) {
        if (c == '"' || c == '\\') out += '\\';
        out += c;
    }
    return out;
}

static void ExportLayout(const std::string& path, const std::vector<PlacedActor>& actors) {
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "[\n";
    for (size_t i = 0; i < actors.size(); i++) {
        const PlacedActor& a = actors[i];
        f << "  { \"tag\": \"" << JsonEscape(a.tag) << "\""
          << ", \"asset\": \"" << JsonEscape(a.assetKey) << "\""
          << ", \"x\": " << a.position.x
          << ", \"y\": " << a.position.y
          << ", \"scale\": " << a.scale
          << ", \"layer\": " << a.layer
          << ", \"flipX\": " << (a.flipX ? "true" : "false")
          << " }";
        if (i + 1 < actors.size()) f << ",";
        f << "\n";
    }
    f << "]\n";
}

// Tiny hand-rolled reader for the exact shape ExportLayout() writes -- not a
// general JSON parser. Reloading a previously-exported layout is a nice-to-
// have for continuing a session, not a hard requirement.
static bool ImportLayout(const std::string& path, std::vector<PlacedActor>& outActors) {
    std::ifstream f(path);
    if (!f.is_open()) return false;

    std::stringstream buf;
    buf << f.rdbuf();
    std::string text = buf.str();

    outActors.clear();
    size_t pos = 0;
    while (true) {
        size_t objStart = text.find('{', pos);
        if (objStart == std::string::npos) break;
        size_t objEnd = text.find('}', objStart);
        if (objEnd == std::string::npos) break;
        std::string obj = text.substr(objStart, objEnd - objStart);
        pos = objEnd + 1;

        PlacedActor a;
        auto extractString = [&](const char* key) -> std::string {
            std::string needle = std::string("\"") + key + "\"";
            size_t k = obj.find(needle);
            if (k == std::string::npos) return "";
            size_t colon = obj.find(':', k);
            size_t q1 = obj.find('"', colon);
            size_t q2 = obj.find('"', q1 + 1);
            if (q1 == std::string::npos || q2 == std::string::npos) return "";
            return obj.substr(q1 + 1, q2 - q1 - 1);
        };
        auto extractNumber = [&](const char* key) -> float {
            std::string needle = std::string("\"") + key + "\"";
            size_t k = obj.find(needle);
            if (k == std::string::npos) return 0.0f;
            size_t colon = obj.find(':', k);
            return (float)atof(obj.c_str() + colon + 1);
        };
        auto extractBool = [&](const char* key) -> bool {
            std::string needle = std::string("\"") + key + "\"";
            size_t k = obj.find(needle);
            if (k == std::string::npos) return false;
            size_t colon = obj.find(':', k);
            size_t t = obj.find("true", colon);
            size_t comma = obj.find(',', colon);
            return t != std::string::npos && (comma == std::string::npos || t < comma);
        };

        a.tag = extractString("tag");
        a.assetKey = extractString("asset");
        a.position.x = extractNumber("x");
        a.position.y = extractNumber("y");
        a.scale = extractNumber("scale");
        if (a.scale == 0.0f) a.scale = 1.0f;
        a.layer = (int)extractNumber("layer");
        a.flipX = extractBool("flipX");

        if (!a.assetKey.empty()) outActors.push_back(a);
    }
    return true;
}

// --- Asset manifest ----------------------------------------------------------

static std::vector<std::string> LoadManifest(const std::string& path) {
    std::vector<std::string> keys;
    std::ifstream f(path);
    if (!f.is_open()) return keys;
    std::string line;
    while (std::getline(f, line)) {
        while (!line.empty() && (line.back() == '\r' || line.back() == '\n')) line.pop_back();
        if (!line.empty()) keys.push_back(line);
    }
    return keys;
}

// --- Asset tree (folder-collapsed sidebar) ----------------------------------
//
// The manifest is a flat list of relative paths ("single_tiles/swamp/tile_00.png").
// Rather than show 648 flat rows for single_tiles alone, group by path segment
// at every level so each folder is one collapsible header row. Built once at
// startup from the manifest; only "expanded" state changes at runtime.

struct TreeNode {
    std::string name;               // this segment only, e.g. "swamp"
    int manifestIndex = -1;         // >= 0 for a leaf (asset) node, -1 for a folder
    bool expanded = false;
    std::vector<std::unique_ptr<TreeNode>> children;
};

static TreeNode* FindOrAddChild(TreeNode* parent, const std::string& name) {
    for (auto& c : parent->children) {
        if (c->name == name) return c.get();
    }
    parent->children.push_back(std::unique_ptr<TreeNode>(new TreeNode()));
    TreeNode* node = parent->children.back().get();
    node->name = name;
    return node;
}

static TreeNode BuildAssetTree(const std::vector<std::string>& manifest) {
    TreeNode root;
    for (int i = 0; i < (int)manifest.size(); i++) {
        const std::string& key = manifest[i];
        TreeNode* cur = &root;
        size_t start = 0;
        while (true) {
            size_t slash = key.find('/', start);
            if (slash == std::string::npos) {
                // Final segment -- the filename itself, a leaf.
                TreeNode* leaf = FindOrAddChild(cur, key.substr(start));
                leaf->manifestIndex = i;
                break;
            }
            std::string segment = key.substr(start, slash - start);
            cur = FindOrAddChild(cur, segment);
            start = slash + 1;
        }
    }
    return root;
}

// One row of the flattened, currently-visible sidebar tree -- rebuilt each
// frame from the (small, static) tree structure based on which folders are
// expanded. Cheap enough at this asset count to not bother caching.
struct VisibleRow {
    TreeNode* node;
    int depth;
};

static void FlattenVisible(TreeNode* node, int depth, std::vector<VisibleRow>& out) {
    for (auto& child : node->children) {
        out.push_back({child.get(), depth});
        if (child->manifestIndex < 0 && child->expanded) {
            FlattenVisible(child.get(), depth + 1, out);
        }
    }
}

// --- Stepper button (mouse-only value adjust, no keyboard needed) ----------
// Click nudges by `step` once; holding repeats after an initial delay, same
// feel as a scrollbar arrow. `heldId` identifies which stepper (if any) is
// currently being held, so only one auto-repeats at a time.

struct StepperState {
    int heldId = -1;
    float heldTimer = 0.0f;
};

static const float STEPPER_INITIAL_DELAY = 0.4f;
static const float STEPPER_REPEAT_RATE = 0.06f;

// Draws one [-] value [+] row and returns the delta to apply this frame
// (already sign-adjusted, 0 if nothing happened). `id` must be unique per
// stepper on screen so StepperState can track which one is held. Clicking
// the middle value box (rather than +/-) sets *clickedValueBox to true so
// the caller can enter click-to-type mode for that field instead.
static float DrawStepper(int id, StepperState& state, float x, float y, float w, float step,
                          const char* label, const std::string& valueText, float deltaTime,
                          bool isTyping, bool* clickedValueBox) {
    float result = 0.0f;
    float rowH = 64.0f;
    float btnW = 64.0f;

    DrawText(label, (int)x, (int)y - 30, 24, Color{180, 180, 200, 255});

    Rectangle minusBtn = {x, y, btnW, rowH};
    Rectangle valueBox = {x + btnW + 8, y, w - btnW * 2 - 16, rowH};
    Rectangle plusBtn = {x + w - btnW, y, btnW, rowH};

    Vector2 mouse = GetMousePosition();
    bool overMinus = CheckCollisionPointRec(mouse, minusBtn);
    bool overPlus = CheckCollisionPointRec(mouse, plusBtn);
    bool overValue = CheckCollisionPointRec(mouse, valueBox);

    DrawRectangleRec(minusBtn, overMinus ? Color{90, 90, 130, 255} : Color{55, 55, 65, 255});
    DrawRectangleRec(plusBtn, overPlus ? Color{90, 90, 130, 255} : Color{55, 55, 65, 255});
    DrawRectangleRec(valueBox, isTyping ? Color{50, 50, 80, 255} : Color{40, 40, 46, 255});
    DrawRectangleLinesEx(valueBox, 2.0f, isTyping ? YELLOW : Color{80, 80, 100, 255});

    int minusW = MeasureText("-", 32);
    int plusW = MeasureText("+", 32);
    DrawText("-", (int)(minusBtn.x + minusBtn.width / 2 - minusW / 2), (int)(minusBtn.y + 14), 32, RAYWHITE);
    DrawText("+", (int)(plusBtn.x + plusBtn.width / 2 - plusW / 2), (int)(plusBtn.y + 14), 32, RAYWHITE);
    DrawText(valueText.c_str(), (int)(valueBox.x + 16), (int)(valueBox.y + 16), 28, RAYWHITE);
    if (isTyping && ((int)(GetTime() * 2) % 2 == 0)) {
        int cursorX = (int)(valueBox.x + 16 + MeasureText(valueText.c_str(), 28));
        DrawText("|", cursorX, (int)(valueBox.y + 16), 28, RAYWHITE);
    }

    if (clickedValueBox) *clickedValueBox = overValue && IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    // Steppers are disabled while typing -- avoid a click on +/- both
    // nudging the value and fighting with an in-progress edit.
    if (!isTyping) {
        bool pressedMinus = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && overMinus;
        bool pressedPlus = IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && overPlus;
        if (pressedMinus) { result -= step; state.heldId = id * 2 + 0; state.heldTimer = 0.0f; }
        if (pressedPlus)  { result += step; state.heldId = id * 2 + 1; state.heldTimer = 0.0f; }

        if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            if (state.heldId == id * 2 + 0 || state.heldId == id * 2 + 1) state.heldId = -1;
        } else if (state.heldId == id * 2 + 0 && overMinus) {
            state.heldTimer += deltaTime;
            if (state.heldTimer > STEPPER_INITIAL_DELAY) {
                result -= step;
                state.heldTimer = STEPPER_INITIAL_DELAY - STEPPER_REPEAT_RATE;
            }
        } else if (state.heldId == id * 2 + 1 && overPlus) {
            state.heldTimer += deltaTime;
            if (state.heldTimer > STEPPER_INITIAL_DELAY) {
                result += step;
                state.heldTimer = STEPPER_INITIAL_DELAY - STEPPER_REPEAT_RATE;
            }
        }
    }

    return result;
}

// --- Main --------------------------------------------------------------------

int main(int argc, char** argv) {
    InitWindow(WINDOW_W, WINDOW_H, "Tom Scene Editor");
    SetTargetFPS(60);

    std::vector<std::string> manifest = LoadManifest(MANIFEST_PATH);
    if (manifest.empty()) {
        TraceLog(LOG_WARNING, "No manifest at %s -- run `make` or tools/pack_assets.sh first", MANIFEST_PATH);
    }
    TreeNode assetTree = BuildAssetTree(manifest);

    std::vector<PlacedActor> actors;
    std::string outPath = (argc > 1) ? argv[1] : "layout.json";
    ImportLayout(outPath, actors);
    // Re-resolve textures for anything loaded from a saved layout.
    for (auto& a : actors) a.texture = LoadPackedTexture(a.assetKey);

    // Camera: pans/zooms over the WORLD_W x WORLD_H canvas, drawn between the
    // two sidebars. Rolled by hand instead of reusing SceneCamera -- that
    // class bakes its screen offset to GAME_W/GAME_H (720x720), which doesn't
    // fit this tool's sidebar+canvas window layout.
    Camera2D cam = {0};
    cam.zoom = 1.0f;
    cam.target = {WORLD_W / 2.0f, WORLD_H / 2.0f};

    int selectedAssetManifestIndex = -1;
    int selectedActorIndex = -1;
    int sidebarScroll = 0;
    bool draggingActor = false;
    Vector2 dragOffset = {0, 0};
    int nextTagId = 1;

    StepperState stepperState;

    // Click-to-type state for the property panel: -1 = nothing focused,
    // otherwise 0=x, 1=y, 2=scale, 3=layer. typingBuffer holds the in-progress
    // text until Enter/Tab/click-away commits it back into the actor.
    int typingField = -1;
    std::string typingBuffer;
    int lastSelectedActorForTyping = -1;

    // Cache thumbnails lazily as they scroll into view would be more work
    // than this tool needs -- load everything up front, manifests are small.
    std::vector<Texture2D> assetThumbs(manifest.size());
    for (size_t i = 0; i < manifest.size(); i++) assetThumbs[i] = LoadPackedTexture(manifest[i]);

    while (!WindowShouldClose()) {
        int screenW = GetScreenWidth();
        int screenH = GetScreenHeight();
        int canvasX0 = LEFT_SIDEBAR_W;
        int canvasX1 = screenW - RIGHT_SIDEBAR_W;
        cam.offset = {canvasX0 + (canvasX1 - canvasX0) / 2.0f, screenH / 2.0f};

        Vector2 mouseScreen = GetMousePosition();
        bool overLeftSidebar = mouseScreen.x < LEFT_SIDEBAR_W;
        bool overRightSidebar = mouseScreen.x >= canvasX1;
        bool overCanvas = !overLeftSidebar && !overRightSidebar;

        // --- Rebuild the visible row list for this frame ---
        std::vector<VisibleRow> visibleRows;
        FlattenVisible(&assetTree, 0, visibleRows);

        // --- Left sidebar: asset tree interaction ---
        if (overLeftSidebar) {
            sidebarScroll -= (int)(GetMouseWheelMove() * 80.0f);
            if (sidebarScroll < 0) sidebarScroll = 0;

            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                int row = (int)((mouseScreen.y + sidebarScroll - 90) / 88);
                if (row >= 0 && row < (int)visibleRows.size()) {
                    TreeNode* node = visibleRows[row].node;
                    if (node->manifestIndex >= 0) {
                        selectedAssetManifestIndex = node->manifestIndex;
                    } else {
                        node->expanded = !node->expanded;
                    }
                }
            }
        } else if (overCanvas) {
            // --- Canvas pan/zoom (right mouse drag pans, wheel zooms to cursor) ---
            if (IsMouseButtonDown(MOUSE_BUTTON_RIGHT)) {
                Vector2 delta = GetMouseDelta();
                cam.target.x -= delta.x / cam.zoom;
                cam.target.y -= delta.y / cam.zoom;
            }
            float wheel = GetMouseWheelMove();
            if (wheel != 0.0f) {
                Vector2 beforeWorld = GetScreenToWorld2D(mouseScreen, cam);
                cam.zoom *= (wheel > 0) ? 1.1f : (1.0f / 1.1f);
                if (cam.zoom < 0.1f) cam.zoom = 0.1f;
                if (cam.zoom > 4.0f) cam.zoom = 4.0f;
                Vector2 afterWorld = GetScreenToWorld2D(mouseScreen, cam);
                cam.target.x += (beforeWorld.x - afterWorld.x);
                cam.target.y += (beforeWorld.y - afterWorld.y);
            }

            Vector2 mouseWorld = GetScreenToWorld2D(mouseScreen, cam);

            // --- Place a new actor from the selected asset ---
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && selectedAssetManifestIndex >= 0) {
                PlacedActor a;
                a.assetKey = manifest[selectedAssetManifestIndex];
                a.texture = assetThumbs[selectedAssetManifestIndex];
                a.position = mouseWorld;
                char buf[64];
                snprintf(buf, sizeof(buf), "actor_%d", nextTagId++);
                a.tag = buf;
                actors.push_back(a);
                selectedActorIndex = (int)actors.size() - 1;
                selectedAssetManifestIndex = -1; // one placement per click, avoid accidental spam
            }
            // --- Select/drag an existing actor ---
            else if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                selectedActorIndex = -1;
                for (int i = (int)actors.size() - 1; i >= 0; i--) {
                    PlacedActor& a = actors[i];
                    float w = a.texture.width * a.scale;
                    float h = a.texture.height * a.scale;
                    Rectangle bounds = {a.position.x, a.position.y, w, h};
                    if (CheckCollisionPointRec(mouseWorld, bounds)) {
                        selectedActorIndex = i;
                        draggingActor = true;
                        dragOffset = {mouseWorld.x - a.position.x, mouseWorld.y - a.position.y};
                        break;
                    }
                }
            }

            if (draggingActor && selectedActorIndex >= 0) {
                if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                    actors[selectedActorIndex].position = {mouseWorld.x - dragOffset.x, mouseWorld.y - dragOffset.y};
                } else {
                    draggingActor = false;
                }
            }
        }

        // Right-sidebar stepper buttons and the delete button are drawn (and
        // their clicks handled) down in the draw section below -- DrawStepper
        // both renders a row and returns this frame's value delta, so there's
        // no separate update-then-draw pass needed for the property panel.

        // --- Save/load ---
        bool ctrl = IsKeyDown(KEY_LEFT_CONTROL) || IsKeyDown(KEY_RIGHT_CONTROL);
        if (ctrl && IsKeyPressed(KEY_S)) {
            ExportLayout(outPath, actors);
            TraceLog(LOG_INFO, "Exported %d actors to %s", (int)actors.size(), outPath.c_str());
        }

        // --- Draw ---
        BeginDrawing();
        ClearBackground(Color{30, 30, 34, 255});

        BeginMode2D(cam);
        // World bounds outline
        DrawRectangleLinesEx({0, 0, WORLD_W, WORLD_H}, 2.0f / cam.zoom, Color{100, 100, 140, 255});

        // Draw actors sorted by layer (stable_sort by index so within-layer
        // order matches placement order, same tie-break as Scene's own sort).
        std::vector<int> order(actors.size());
        for (size_t i = 0; i < actors.size(); i++) order[i] = (int)i;
        std::stable_sort(order.begin(), order.end(), [&](int a, int b) {
            return actors[a].layer < actors[b].layer;
        });

        for (int idx : order) {
            PlacedActor& a = actors[idx];
            if (a.texture.id == 0) continue;
            Rectangle src = {0, 0, a.flipX ? -(float)a.texture.width : (float)a.texture.width, (float)a.texture.height};
            Rectangle dest = {a.position.x, a.position.y, a.texture.width * a.scale, a.texture.height * a.scale};
            DrawTexturePro(a.texture, src, dest, {0, 0}, 0.0f, WHITE);
            if (idx == selectedActorIndex) {
                DrawRectangleLinesEx(dest, 2.0f / cam.zoom, YELLOW);
            }
        }
        EndMode2D();

        // --- Left sidebar: folder-collapsed asset tree ---
        DrawRectangle(0, 0, LEFT_SIDEBAR_W, screenH, Color{20, 20, 24, 255});
        DrawText("ASSETS", 24, 24, 40, RAYWHITE);
        BeginScissorMode(0, 90, LEFT_SIDEBAR_W, screenH - 90);
        for (size_t i = 0; i < visibleRows.size(); i++) {
            TreeNode* node = visibleRows[i].node;
            int depth = visibleRows[i].depth;
            int y = 90 + (int)i * 88 - sidebarScroll;
            if (y < -88 || y > screenH) continue;

            int indent = depth * 48;
            bool isFolder = (node->manifestIndex < 0);
            bool sel = (!isFolder && node->manifestIndex == selectedAssetManifestIndex);
            if (sel) DrawRectangle(0, y, LEFT_SIDEBAR_W, 86, Color{60, 60, 90, 255});

            if (isFolder) {
                const char* arrow = node->expanded ? "v" : ">";
                DrawText(arrow, 24 + indent, y + 28, 32, Color{160, 160, 200, 255});
                DrawText(TextFormat("%s (%d)", node->name.c_str(), (int)node->children.size()),
                          24 + indent + 40, y + 28, 28, Color{200, 200, 220, 255});
            } else {
                if (assetThumbs[node->manifestIndex].id != 0) {
                    Rectangle src = {0, 0, (float)assetThumbs[node->manifestIndex].width, (float)assetThumbs[node->manifestIndex].height};
                    Rectangle dest = {(float)(10 + indent), (float)y + 3, 80, 80};
                    DrawTexturePro(assetThumbs[node->manifestIndex], src, dest, {0, 0}, 0.0f, WHITE);
                }
                DrawText(node->name.c_str(), 100 + indent, y + 34, 24, RAYWHITE);
            }
        }
        EndScissorMode();

        // --- Left sidebar HUD ---
        DrawRectangle(0, screenH - 160, LEFT_SIDEBAR_W, 160, Color{20, 20, 24, 220});
        DrawText(TextFormat("Actors: %d", (int)actors.size()), 24, screenH - 140, 32, RAYWHITE);
        DrawText("LMB: expand folder / place / drag", 24, screenH - 96, 26, GRAY);
        DrawText("RMB drag / wheel: pan/zoom canvas", 24, screenH - 64, 26, GRAY);
        DrawText("Ctrl+S: save layout (only keyboard use)", 24, screenH - 32, 26, GRAY);

        // --- Right sidebar: property panel ---
        DrawRectangle(canvasX1, 0, RIGHT_SIDEBAR_W, screenH, Color{20, 20, 24, 255});
        DrawText("PROPERTIES", canvasX1 + 24, 24, 36, RAYWHITE);

        if (selectedActorIndex >= 0 && selectedActorIndex < (int)actors.size()) {
            PlacedActor& a = actors[selectedActorIndex];
            DrawText(a.tag.c_str(), canvasX1 + 24, 90, 30, YELLOW);
            DrawText(a.assetKey.c_str(), canvasX1 + 24, 130, 20, GRAY);

            if (selectedActorIndex != lastSelectedActorForTyping) {
                lastSelectedActorForTyping = selectedActorIndex;
                typingField = -1;
            }

            int fieldW = RIGHT_SIDEBAR_W - 48;
            int rowH = 100;
            int firstFieldY = 220;
            float dt = GetFrameTime();

            // Committing a typed value writes it straight into the actor and
            // clears typingField -- shared by all four fields below.
            auto commitTyping = [&]() {
                if (typingField < 0) return;
                float v = (float)atof(typingBuffer.c_str());
                if (typingField == 0) a.position.x = v;
                else if (typingField == 1) a.position.y = v;
                else if (typingField == 2) a.scale = (v > 0.0f) ? v : a.scale;
                else if (typingField == 3) a.layer = (int)v;
                typingField = -1;
            };

            const char* labels[4] = {"X", "Y", "Scale", "Layer"};
            float steps[4] = {10.0f, 10.0f, 0.1f, 1.0f};
            float deltas[4] = {0, 0, 0, 0};
            for (int i = 0; i < 4; i++) {
                std::string displayText = (typingField == i) ? typingBuffer :
                    (i == 0 ? TextFormat("%.0f", a.position.x) :
                     i == 1 ? TextFormat("%.0f", a.position.y) :
                     i == 2 ? TextFormat("%.2f", a.scale) :
                              TextFormat("%d", a.layer));
                bool clickedBox = false;
                deltas[i] = DrawStepper(i, stepperState, (float)(canvasX1 + 24), (float)(firstFieldY + i * rowH),
                                         (float)fieldW, steps[i], labels[i], displayText, dt,
                                         typingField == i, &clickedBox);
                if (clickedBox) {
                    commitTyping(); // commit whatever was being typed elsewhere first
                    typingField = i;
                    typingBuffer = (i == 0 ? TextFormat("%.0f", a.position.x) :
                                    i == 1 ? TextFormat("%.0f", a.position.y) :
                                    i == 2 ? TextFormat("%.2f", a.scale) :
                                             TextFormat("%d", a.layer));
                }
            }

            if (typingField >= 0) {
                int ch = GetCharPressed();
                while (ch > 0) {
                    if ((ch >= '0' && ch <= '9') || ch == '.' || ch == '-') typingBuffer += (char)ch;
                    ch = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && !typingBuffer.empty()) typingBuffer.pop_back();
                if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_TAB)) commitTyping();
                // Clicking anywhere outside the panel commits too.
                if (!overRightSidebar && (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) || IsMouseButtonPressed(MOUSE_BUTTON_RIGHT))) {
                    commitTyping();
                }
            } else {
                a.position.x += deltas[0];
                a.position.y += deltas[1];
                a.scale = fmaxf(0.1f, a.scale + deltas[2]);
                a.layer += (int)deltas[3];
            }

            // Flip X checkbox -- mirrors the actor's texture horizontally.
            Rectangle flipBox = {(float)(canvasX1 + 24), (float)(firstFieldY + 4 * rowH + 20), 48.0f, 48.0f};
            bool overFlip = CheckCollisionPointRec(mouseScreen, flipBox);
            DrawRectangleRec(flipBox, overFlip ? Color{90, 90, 130, 255} : Color{55, 55, 65, 255});
            DrawRectangleLinesEx(flipBox, 2.0f, Color{80, 80, 100, 255});
            if (a.flipX) {
                DrawLineEx({flipBox.x + 8, flipBox.y + 8}, {flipBox.x + 40, flipBox.y + 40}, 3.0f, RAYWHITE);
                DrawLineEx({flipBox.x + 40, flipBox.y + 8}, {flipBox.x + 8, flipBox.y + 40}, 3.0f, RAYWHITE);
            }
            DrawText("Flip X", (int)(flipBox.x + 60), (int)(flipBox.y + 12), 26, RAYWHITE);
            if (overFlip && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) a.flipX = !a.flipX;

            // Delete button -- mouse-only, no keyboard shortcut needed.
            Rectangle deleteBtn = {(float)(canvasX1 + 24), (float)(firstFieldY + 4 * rowH + 90), (float)fieldW, 70.0f};
            bool overDelete = CheckCollisionPointRec(mouseScreen, deleteBtn);
            DrawRectangleRec(deleteBtn, overDelete ? Color{140, 50, 50, 255} : Color{90, 40, 40, 255});
            DrawRectangleLinesEx(deleteBtn, 2.0f, Color{180, 80, 80, 255});
            int delTextW = MeasureText("Delete Actor", 28);
            DrawText("Delete Actor", (int)(deleteBtn.x + deleteBtn.width / 2 - delTextW / 2), (int)(deleteBtn.y + 20), 28, RAYWHITE);
            if (overDelete && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                actors.erase(actors.begin() + selectedActorIndex);
                selectedActorIndex = -1;
            }
        } else {
            DrawText("No actor selected", canvasX1 + 24, 90, 26, GRAY);
        }

        EndDrawing();
    }

    for (auto& tex : assetThumbs) if (tex.id != 0) UnloadTexture(tex);
    CloseWindow();
    return 0;
}
