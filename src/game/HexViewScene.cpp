#include "HexViewScene.hpp"
#include "../engine/HexWorld.hpp"
#include "../engine/GameConstants.hpp"
#include "../engine/SceneInputHandler.hpp"
#include "../engine/HexPathFinder.hpp"
#include "../engine/Item.hpp"
#include "../engine/SceneManager.hpp"
#include "../engine/GameState.h"
#include "EventBus.h"
#include "PauseMenuOverlay.hpp"
#include "Hotbar.hpp"
#include <cmath>
#include <string>

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

    // Use shared vitals and mood from GameState if available, otherwise fallback to defaults
    GotchiStats& stats = gameState_ ? gameState_->vitals : defaultStats_;
    GotchiMood& mood = gameState_ ? gameState_->mood : defaultMood_;
    gotchi = new Gotchi(gotchiPos, stats, mood, gameState_);
    gotchi->setHexSize(hexSize_);
    gotchi->init();
    gotchi->loadAnimationFrames("gotchis/001");
    TraceLog(LOG_WARNING, "GOTCHI_ASSETS path=gotchis/001 idle=%d walk=%d move=%d",
             (int)gotchi->animIdleCount(), (int)gotchi->animWalkCount(), (int)gotchi->animMoveCount());
    gotchi->setAction("idle");
    gotchi->setWorld(world);  // Set world for item detection
    gotchi->setEventBus(eventBus_);  // Set event bus for CareAction emission
    addActor(gotchi);

    // Camera setup with smooth panning - follow the gotchi
    // Adjusted boundary to properly contain the 32x18 hex world
    getCamera()->setBoundary(0, -400, 4200.0f, 1450.0f);
    getCamera()->setLookaheadEnabled(false);
    getCamera()->followActor(gotchi);
    // Position camera directly over the gotchi
    getCamera()->setPosition(gotchi->getPosition());
    getCamera()->setZoom(2.0f);

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

    // Draw vitals at top right
    drawVitals();

    // Draw instructions
    DrawText("HEX VIEW", 14, 8, 18, Color{180, 180, 255, 255});
    DrawText("Right-click drag to pan  Mouse wheel to zoom", GAME_W - 290, 8, 12, Color{140, 140, 180, 255});

    // Hotbar instructions
    DrawText("Drag items from bottom palette onto hexes", 20, GAME_H - 75, 14, Color{180, 220, 255, 200});

    // Draw back button
    if (backButton_) {
        backButton_->draw();
    }

    // Draw the tutorial dialog on top of everything while it's this scene's turn
    if (tutorialController_ && tutorialController_->isActive() && tutorialController_->currentScene() == "hexboard") {
        tutorialController_->draw();
    }
}

void HexViewScene::update(float deltaTime) {
    // GotchiSim sets state_.collapsed when a vital need bottoms out (only
    // once deathEnabled). That's our one death condition -- route it into
    // Gotchi::isDead() so the death trigger below fires.
    if (gotchi && gameState_ && gameState_->collapsed && !gotchi->isDead()) {
        gotchi->setDead(true);
    }

    // Sleep-collapse gate: same trigger as GotchiScene::applySleepCollapseGate()
    // (the visible Sleep bar, SLEEP_DEBT, hits 100) -- duplicated here
    // because the player can be exploring the hexboard when it happens, not
    // just sitting on the home screen. Locks buttons via
    // applyTutorialLocks()-equivalent logic below and holds the gotchi in
    // "wobble" until the player merges (never auto-merges).
    bool justCollapsed = false;
    bool sleepMaxed = gotchi && gotchi->getStats().getStat(SecondaryStat::SLEEP_DEBT) >= 100.0f;
    if (gameState_ && !gameState_->sleepCollapsed && sleepMaxed && gameState_->mode == Mode::Gotchi) {
        gameState_->sleepCollapsed = true;
        justCollapsed = true;
    }
    bool collapseFreeze = gameState_ && gameState_->sleepCollapsed;

    // Merge only exists as a button on GotchiScene -- send the player back
    // there immediately so the collapse doesn't strand them on the hexboard
    // with no way to actually merge.
    if (justCollapsed && getSceneManager()) {
        static_cast<SceneManager*>(getSceneManager())->switchScene("gotchi");
    }

    // Freeze vitals while the tutorial is actively teaching, or once
    // collapsed -- must happen before Scene::update() since that's what
    // drives gotchi->update() (added via addActor()). Written directly to
    // GameState (what GotchiSim actually reads) since Gotchi::setStatsFrozen()
    // forwards through Gotchi::gameState_, which nothing wires up -- see
    // GotchiScene's matching fix for details.
    if (gameState_) {
        gameState_->statsFrozen = (tutorialController_ && tutorialController_->isActive()) || collapseFreeze;
        gameState_->onHexboard = true;
    }

    Scene::update(deltaTime);

    // Gotchi update is handled by Scene::update() since it was added via addActor()
    // Keeping only the explicit draw() call in HexViewScene::draw()

    // Death's merge -> DeathScene handoff is driven externally by
    // DeathSequencer (see main.cpp), which switches away from whichever
    // scene is current the moment GotchiSim emits PetCollapsed -- no
    // per-scene handling needed here.

    if (paused) {
        if (pauseMenu) pauseMenu->update(deltaTime);
        return;
    }

    // Lock the back button while the tutorial hasn't taught hex movement yet,
    // so the player can't bail out of the hexboard step before trying it.
    if (backButton_) {
        bool backLocked = tutorialController_ && tutorialController_->isActive()
                           && !tutorialController_->isActionUnlocked("walk");
        backButton_->setEnabled(!backLocked);
    }

    // Update back button first (before click handlers)
    if (backButton_) {
        SceneInputHandler* ih = getInputHandler();
        backButton_->update(ih, deltaTime);
    }

    // Advance/reveal the tutorial dialog while it belongs to this scene; once
    // an advance crosses back into a "gotchi" step, follow it there immediately.
    if (tutorialController_ && tutorialController_->isActive() && tutorialController_->currentScene() == "hexboard") {
        tutorialController_->update(deltaTime);
        if (tutorialController_->currentScene() == "gotchi" && getSceneManager()) {
            static_cast<SceneManager*>(getSceneManager())->switchScene("gotchi");
            return;
        }
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
    // and only if we're not clicking on the back button
    if (!isDraggingFromHotbar && gotchi && ih->isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
        // Don't process hex click if mouse is over the back button
        if (!(backButton_ && backButton_->isHovered())) {
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
                        if (tutorialController_) tutorialController_->reportAction("walk");
                    }
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

    // Update biome in GameState when gotchi completes path movement
    if (gotchi && gameState_ && !gotchi->isFollowingPath()) {
        HexCoords currentHex = gotchi->getCurrentHex();
        // Only update if position changed
        if (currentHex.q != gameState_->gotchiHexQ || currentHex.r != gameState_->gotchiHexR) {
            gameState_->gotchiBiome = world->getBiomeAt(gotchi->getPosition().x, gotchi->getPosition().y);
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
    // Save gotchi's current hex position and biome to GameState before switching
    if (gameState_ && gotchi) {
        HexCoords currentHex = gotchi->getCurrentHex();
        gameState_->gotchiHexQ = currentHex.q;
        gameState_->gotchiHexR = currentHex.r;
        // Get biome from world at current position
        gameState_->gotchiBiome = world->getBiomeAt(gotchi->getPosition().x, gotchi->getPosition().y);
    }

    // Switch back to gotchi scene
    if (getSceneManager()) {
        SceneManager* mgr = static_cast<SceneManager*>(getSceneManager());
        mgr->switchScene("gotchi");
    }
}

// ============================================================================
// Vitals Display Implementation
// ============================================================================

void HexViewScene::drawVitals() const {
    if (!gotchi) return;

    const GotchiStats& stats = gotchi->getStats();

    // Vitals to display: Hunger, Thirst, Hygiene, Sleep
    struct Vital {
        std::string name;
        float value;
        Color color;
    };

    // Get normalized values (0-1 range) and create vital records
    // Lower is better for Hunger, Thirst, Hygiene (need to invert for display)
    // Higher sleep debt means more sleep needed (higher = worse)
    float hunger = stats.getHunger();      // 0=full, 100=starving
    float thirst = stats.getThirst();      // 0=hydrated, 100=dehydrated
    float hygiene = stats.getHygiene();    // 0=clean, 100=dirty (higher is worse)
    float sleep = stats.getSleep();        // 0=rested, 100=exhausted

    std::vector<Vital> vitals = {
        {"Hunger", hunger, Color{255, 160, 0, 255}},      // Orange
        {"Thirst", thirst, Color{0, 160, 255, 255}},      // Blue
        {"Hygiene", hygiene, Color{0, 200, 100, 255}},    // Green
        {"Sleep", sleep, Color{160, 0, 255, 255}}         // Purple
    };

    // Position at top right
    float x = GAME_W - 140.0f;  // Right side
    float y = 40.0f;            // Near top
    float barWidth = 120.0f;
    float barHeight = 8.0f;
    float spacing = 16.0f;

    // Draw title
    DrawText("VITALS", static_cast<int>(x - 35), static_cast<int>(y - 20), 12, WHITE);

    // Draw each vital bar
    for (size_t i = 0; i < vitals.size(); i++) {
        float vitalY = y + i * spacing;
        const Vital& vital = vitals[i];

        // Convert 0-100 to 0-1 for bar
        float normalized = vital.value / 100.0f;

        // Background (empty bar)
        DrawRectangle(static_cast<int>(x), static_cast<int>(vitalY),
                      static_cast<int>(barWidth), static_cast<int>(barHeight),
                      Color{40, 40, 40, 200});

        // Foreground (filled bar) - red for high values (bad)
        // Use color that indicates severity (green = good, red = bad)
        // For these vitals, higher = worse, so red = bad
        Color barColor = Color{200, 50, 50, 255};  // Reddish for "needs attention"
        if (vital.value < 30) {
            barColor = Color{50, 200, 50, 255};    // Green for healthy
        } else if (vital.value < 60) {
            barColor = Color{200, 200, 50, 255};   // Yellow for moderate
        }

        DrawRectangle(static_cast<int>(x), static_cast<int>(vitalY),
                      static_cast<int>(barWidth * normalized), static_cast<int>(barHeight),
                      barColor);

        // Label with percentage
        std::string label = vital.name + " " + std::to_string(static_cast<int>(vital.value)) + "%";
        DrawText(label.c_str(), static_cast<int>(x - 75), static_cast<int>(vitalY - 1),
                 10, Color{200, 200, 200, 255});
    }
}
