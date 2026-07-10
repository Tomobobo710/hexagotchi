#include "HexViewScene.hpp"
#include "../engine/HexWorld.hpp"
#include "../engine/GameConstants.hpp"
#include "../engine/SceneInputHandler.hpp"
#include "../engine/HexPathFinder.hpp"
#include "../engine/Item.hpp"
#include "../engine/SceneManager.hpp"
#include "EventBus.h"
#include "PauseMenuOverlay.hpp"
#include "Hotbar.hpp"
#include <cmath>

// ============================================================================
// HexViewScene implementation
// ============================================================================

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
    gotchi = new Gotchi(gotchiPos, defaultStats_, defaultMood_);
    gotchi->setHexSize(hexSize_);
    gotchi->init();
    gotchi->loadAnimationFrames("gotchis/001");
    TraceLog(LOG_WARNING, "GOTCHI_ASSETS path=gotchis/001 idle=%d walk=%d move=%d",
             (int)gotchi->animIdleCount(), (int)gotchi->animWalkCount(), (int)gotchi->animMoveCount());
    gotchi->setAction("idle");
    gotchi->setWorld(world);  // Set world for item detection
    gotchi->setEventBus(eventBus_);  // Set event bus for CareAction emission
    addActor(gotchi);

    // Camera setup with smooth panning
    getCamera()->setBoundary(0, -200, 4800.0f, 900.0f);
    getCamera()->setLookaheadEnabled(false);

    // Position camera at world center (Y is at vertical center of valid band)
    float startX = (0.0f + 4800.0f) * 0.5f;
    float startY = (-200.0f + 900.0f) * 0.5f;
    getCamera()->setPosition(startX, startY);
    getCamera()->setZoom(0.8f);

    // Initialize the hotbar (screen-space UI)
    hotbar_ = std::make_unique<Hotbar>();
    hotbar_->init(GAME_W, GAME_H);
    hotbar_->setInputHandler(getInputHandler());

    // Add Back button
    addBackButton();
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

    // Draw items on the hex world (from direct placement)
    if (world) {
        for (const auto& item : world->getItems()) {
            if (item.consumed) continue;

            // Draw item marker at hex center
            Vector2 itemPos = HexCoords(item.hexQ, item.hexR).toPixel(hexSize_);
            Color itemColor;

            // Color based on item type
            switch (item.type) {
                case ItemType::FOOD:       itemColor = YELLOW; break;   // Orange/Yellow for food
                case ItemType::WATER:      itemColor = BLUE; break;     // Blue for water
                case ItemType::MEDICINE:   itemColor = RED; break;      // Red for medicine
                case ItemType::TOY:        itemColor = PINK; break;     // Pink for toys
                case ItemType::CLEANING:   itemColor = GREEN; break;    // Green for cleaning
                case ItemType::SLEEP:      itemColor = PURPLE; break;   // Purple for sleep
                case ItemType::ENERGY:     itemColor = ORANGE; break;   // Orange for energy
                case ItemType::HAPPINESS:  itemColor = {100, 200, 255, 255}; break;     // Cyan-like for happiness
                default:                   itemColor = WHITE; break;
            }

            // Draw item as a circle
            DrawCircleV(itemPos, 12.0f, itemColor);
            DrawCircleLines((int)itemPos.x, (int)itemPos.y, 12.0f, Color{255, 255, 255, 200});

            // Draw a small indicator of item value
            DrawText(std::to_string((int)item.value).c_str(),
                    (int)itemPos.x, (int)itemPos.y - 18, 10, WHITE);
        }
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

    // Draw hotbar at screen bottom
    if (hotbar_) {
        hotbar_->draw();
    }

    // Draw instructions
    DrawText("HEX VIEW", 14, 8, 18, Color{180, 180, 255, 255});
    DrawText("Right-click drag to pan  Mouse wheel to zoom", GAME_W - 290, 8, 12, Color{140, 140, 180, 255});

    // Hotbar instructions
    DrawText("Drag items from bottom palette onto hexes", 20, GAME_H - 75, 14, Color{180, 220, 255, 200});

    // Draw back button
    if (backButton_) {
        backButton_->draw();
    }
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

    // Check if we're dragging from the hotbar
    bool isDraggingFromHotbar = hotbar_ ? hotbar_->isMouseOverSlot() : false;

    // Update hotbar and check for drops
    if (hotbar_) {
        if (hotbar_->update(deltaTime, getCamera())) {
            // A valid drop occurred
            if (hotbar_->hasPendingDrop()) {
                HexItemOverlay::ItemType itemType = hotbar_->getDroppedItemType();
                Vector2 dropWorldPos = hotbar_->getDroppedWorldPosition();

                // Convert world position to hex using the same canonical method as click-to-move
                HexCoords targetHex = HexCoords::fromPixel(dropWorldPos, hexSize_);

                // Validate: only place on existing tile with non-ocean biome
                HexTile* tile = world->getTileAt(targetHex.q, targetHex.r);
                if (tile && tile->getTileType()) {
                    std::string biome = tile->getTileType()->getBiome();
                    if (biome != "ocean") {
                        // Map Hotbar::ItemType to Item::ItemType
                        ItemType itemTypeMap;
                        switch (itemType) {
                            case HexItemOverlay::ItemType::FOOD:   itemTypeMap = ItemType::FOOD; break;
                            case HexItemOverlay::ItemType::BALL:   itemTypeMap = ItemType::TOY; break;  // BALL -> TOY for affection
                            case HexItemOverlay::ItemType::WATER:  itemTypeMap = ItemType::WATER; break;
                            default:                                 itemTypeMap = ItemType::FOOD; break;
                        }

                        // Place item directly into HexWorld::items
                        Item newItem(itemTypeMap, 25.0f, targetHex.q, targetHex.r);
                        world->placeItem(newItem);

                        TraceLog(LOG_WARNING, "ITEM_DROPPED hex=(%d,%d) type=%d", targetHex.q, targetHex.r, (int)itemTypeMap);
                    }
                }

                hotbar_->clearPendingDrop();
            }
        }
    }

    // Check for left-click to select a hex destination (only if NOT dragging from hotbar)
    if (!isDraggingFromHotbar && gotchi && ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
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

    // Periodically remove consumed items (every 5 seconds)
    static float cleanupTimer = 0.0f;
    cleanupTimer += deltaTime;
    if (cleanupTimer >= 5.0f) {
        world->removeConsumedItems();
        cleanupTimer = 0.0f;
    }

    // Clear click marker when gotchi moves
    if (gotchi && hasClickMarker_) {
        HexCoords gotchiHex = gotchi->getCurrentHex();
        if (gotchiHex.q != clickMarkerHex_.q || gotchiHex.r != clickMarkerHex_.r) {
            hasClickMarker_ = false;
        }
    }

    // Update back button
    if (backButton_) {
        backButton_->update(ih, deltaTime);
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

// ============================================================================
// Back Button Implementation
// ============================================================================

void HexViewScene::addBackButton() {
    // Add Back button at top-left of screen
    // Takes input priority so clicking it does NOT also trigger a hex move
    float buttonWidth = 100.0f;
    float buttonHeight = 32.0f;
    float x = 20.0f;
    float y = 40.0f;

    backButton_ = std::unique_ptr<Button>(new Button({x, y}, buttonWidth, buttonHeight, "BACK"));
    backButton_->setAnchor("top-left");
    backButton_->setFontSize(14);
    backButton_->setBackgroundColor({60, 60, 100, 220});
    backButton_->setHoverColor({100, 100, 160, 240});
    backButton_->setBorderColor({150, 150, 200, 255});

    backButton_->setOnClick([this]() {
        onBackButtonClicked();
    });
}

void HexViewScene::onBackButtonClicked() {
    // Switch back to gotchi scene
    if (getSceneManager()) {
        SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
        mgr->switchScene("gotchi");
    }
}
