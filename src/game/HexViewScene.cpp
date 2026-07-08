#include "HexViewScene.hpp"
#include "../engine/HexWorld.hpp"
#include "../engine/GameConstants.hpp"
#include "../engine/SceneInputHandler.hpp"
#include "PauseMenuOverlay.hpp"
#include <cmath>

HexViewScene::HexViewScene() : Scene(4800.0f, 900.0f, {12, 14, 28, 255}) {
    world = nullptr;
    cameraPanning = false;
}

HexViewScene::~HexViewScene() {
    if (world) {
        delete world;
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
    // Draw hex world tiles
    if (world) {
        for (HexTile* tile : world->getTiles()) {
            tile->draw();
        }
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
