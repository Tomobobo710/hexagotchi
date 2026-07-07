#include "HexViewScene.hpp"
#include "../engine/HexWorld.hpp"
#include "../engine/GameConstants.hpp"
#include "PauseMenuOverlay.hpp"

HexViewScene::HexViewScene() : Scene(4800.0f, 900.0f, {12, 14, 28, 255}) {
    world = nullptr;
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
    };

    pauseMenu->onClose = [this]() {
    };

    pauseMenu->onExitSelected = [this]() {
        onExitSelected();
    };

    // Initialize the hex world map
    // Initialize the hex world map
    HexWorldConfig config;
    config.width = 32;   // 32 hexes wide
    config.height = 18;  // 18 hexes tall
    config.hexSize = 64.0f;

    world = new HexWorld(config);
    world->generate();

    getCamera()->setBoundary(0, -200, 4800.0f, 900.0f);
    getCamera()->setLookaheadEnabled(false);  // Disable lookahead since we're not following anything
    getCamera()->setZoom(0.8f);

    // Position camera at origin to see the hex tiles at (0,0) etc.
    // World is 32 hexes wide (3072px) so center at ~1500,1000
    getCamera()->setPosition(1536.0f, 960.0f);
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

    // Draw instructions
    DrawText("HEX VIEW", 14, 8, 18, Color{180, 180, 255, 255});
    DrawText("1: World  2: Boss  3: Input  4: Sprite  5: World  ESC: Exit", GAME_W - 290, 8, 12, Color{140, 140, 180, 255});
}

void HexViewScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (paused) {
        if (pauseMenu) pauseMenu->update(deltaTime);
        return;
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
