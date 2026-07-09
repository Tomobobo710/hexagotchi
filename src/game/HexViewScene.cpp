#include "HexViewScene.hpp"
#include "../engine/HexWorld.hpp"
#include "../engine/GameConstants.hpp"
#include "../engine/SceneInputHandler.hpp"
#include "../engine/HexPathFinder.hpp"
#include "PauseMenuOverlay.hpp"
#include <cmath>

HexViewScene::HexViewScene() : Scene(4800.0f, 900.0f, {12, 14, 28, 255}) {
    world = nullptr;
    gotchi = nullptr;
    cameraPanning = false;
}

HexViewScene::~HexViewScene() {
    if (world) {
        delete world;
    }
    if (gotchi) {
        delete gotchi;
    }
}

void HexViewScene::init() {
    TraceLog(LOG_WARNING, "=== HEXVIEW init() called, generating world ===");
    // Set up callbacks for the pause menu
    pauseMenu = std::unique_ptr<PauseMenuOverlay>(new PauseMenuOverlay(*this));
    pauseMenu->onResume = [this]() {
        paused = false;
        SceneInputHandler* ih = getInputHandler();
        if (ih) {
            ih->clearAllInputs();
        }
        cameraPanning = false;
    };

    pauseMenu->onClose = [this]() {
    };

    pauseMenu->onExitSelected = [this]() {
        onExitSelected();
    };

    // Initialize the hex world map
    HexWorldConfig config;
    config.width = 32;   // 32 hexes wide
    config.height = 18;  // 18 hexes tall
    config.hexSize = 64.0f;

    world = new HexWorld(config);
    world->generate();
    hexSize_ = config.hexSize;

    // Initialize the Gotchi actor
    // Place it at the center hex (q=16, r=9) by converting to pixel coordinates
    HexCoords startHex(16, 9);
    Vector2 gotchiPos = startHex.toPixel(hexSize_);
    gotchi = new Gotchi(gotchiPos);
    gotchi->setHexSize(hexSize_);
    gotchi->init();
    gotchi->loadAnimationFrames("gotchis/001");
    TraceLog(LOG_WARNING, "GOTCHI_ASSETS path=gotchis/001 idle=%d walk=%d move=%d",
             (int)gotchi->animIdleCount(), (int)gotchi->animWalkCount(), (int)gotchi->animMoveCount());
    gotchi->setAction("idle");
    addActor(gotchi);

    // Camera setup with smooth panning
    getCamera()->setBoundary(0, -200, 4800.0f, 900.0f);
    getCamera()->setLookaheadEnabled(false);

    // Position camera at world center (Y is at vertical center of valid band)
    float startX = (0.0f + 4800.0f) * 0.5f;
    float startY = (-200.0f + 900.0f) * 0.5f;
    getCamera()->setPosition(startX, startY);
    getCamera()->setZoom(0.8f);
}

void HexViewScene::draw() {
    Camera2D cam = getCamera()->getRaylibCamera();

    TraceLog(LOG_WARNING, "HEXCAM target=(%.0f,%.0f) offset=(%.0f,%.0f) zoom=%.2f",
        cam.target.x, cam.target.y, cam.offset.x, cam.offset.y, cam.zoom);

    Scene::draw();

    // Draw pause menu overlay if paused
    if (paused && pauseMenu) {
        pauseMenu->draw();
    }

    BeginMode2D(cam);
    // Draw hex world tiles first (background)
    if (world) {
        for (HexTile* tile : world->getTiles()) {
            tile->draw();
        }
    }

    // Draw Gotchi actor on top of the hex tiles
    if (gotchi) {
        gotchi->draw();
    }

    // Visual probe: draw click marker (red dot = world point, green ring = hex center)
    if (hasClickMarker_) {
        // Raw world point the click resolved to (red dot)
        DrawCircleV(clickMarkerWorld_, 6.0f, RED);
        // Center of the hex it mapped to (green ring) — should sit under the red dot
        Vector2 hc = clickMarkerHex_.toPixel(hexSize_);
        DrawCircleLines((int)hc.x, (int)hc.y, hexSize_ * 0.5f, GREEN);
    }

    EndMode2D();

    // Draw panning hint when panning
    if (cameraPanning) {
        DrawText("PANNING (drag to move)", 20, GAME_H - 40, 20, Color{255, 255, 255, 200});
    }

    // Draw instructions
    DrawText("HEX VIEW", 14, 8, 18, Color{180, 180, 255, 255});
    DrawText("Right-click drag to pan  Mouse wheel to zoom", GAME_W - 290, 8, 12, Color{140, 140, 180, 255});
}

void HexViewScene::update(float deltaTime) {
    Scene::update(deltaTime);

    // Gotchi update is handled by Scene::update() since it was added via addActor()
    // Keeping only the explicit draw() call in HexViewScene::draw()

    if (paused) {
        if (pauseMenu) pauseMenu->update(deltaTime);
        return;
    }

    // Camera panning logic
    SceneInputHandler* ih = getInputHandler();
    SceneCamera* cam = getCamera();
    Vector2 mouse = ih->getMousePosition();

    // Right-drag panning with inertia
    if (ih->isMouseButtonHeld(MOUSE_BUTTON_RIGHT)) {
        cam->panUpdate(mouse);
        cameraPanning = true;
    } else {
        cam->panEnd();
        cameraPanning = false;
    }

    // Zoom with mouse wheel - using camera's smooth zoom-to-cursor
    float wheelMove = ih->getMouseWheel();
    if (wheelMove != 0.0f) {
        float ZOOM_STEP = 0.15f;
        cam->zoomAtScreen(mouse, wheelMove * ZOOM_STEP);
    }

    // Check for left-click to select a hex destination
    if (gotchi && ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        Vector2 mouseWorldPos = ih->getMouseWorldPosition();

        // Convert world position to hex coordinates using canonical method
        HexCoords target = HexCoords::fromPixel(mouseWorldPos, hexSize_);
        int targetQ = target.q;
        int targetR = target.r;

        // Get current Gotchi hex position
        HexCoords gotchiHex = gotchi->getCurrentHex();

        // Check if the clicked tile is walkable
        HexTile* clickedTile = world->getTileAt(targetQ, targetR);
        TraceLog(LOG_WARNING,
            "CLICK fired world=(%.0f,%.0f) target=(%d,%d) tile=%s gotchi=(%d,%d)",
            mouseWorldPos.x, mouseWorldPos.y, targetQ, targetR,
            clickedTile ? "OK" : "NULL", gotchiHex.q, gotchiHex.r);

        // Visual probe: store the click marker for rendering
        hasClickMarker_   = true;
        clickMarkerHex_   = target;
        clickMarkerWorld_ = mouseWorldPos;

        if (clickedTile && clickedTile->getTileType()) {
            std::string biome = clickedTile->getTileType()->getBiome();
            if (biome != "ocean") {
                // Find path and set Gotchi moving
                HexPathFinder pathfinder(world);
                std::vector<HexCoords> path = pathfinder.findPath(gotchiHex.q, gotchiHex.r, targetQ, targetR);
                TraceLog(LOG_WARNING, "CLICK pathN=%d biome=%s", (int)path.size(), biome.c_str());

                if (!path.empty()) {
                    gotchi->setPath(path);
                }
            }
        }
    }
}

void HexViewScene::togglePause() {
    if (!paused) {
        paused = true;
        if (pauseMenu) {
            pauseMenu->open();
        }
    } else {
        paused = false;
        if (pauseMenu) {
            pauseMenu->close();
            SceneInputHandler* ih = getInputHandler();
            if (ih) {
                ih->clearAllInputs();
            }
        }
    }
}

void HexViewScene::onExitSelected() {
    extern bool exitRequested;
    exitRequested = true;
}
