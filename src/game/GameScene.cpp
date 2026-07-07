#include "GameScene.hpp"
#include "PlayerActor.hpp"
#include "PauseMenuOverlay.hpp"
#include "ControlsOverlay.hpp"
#include "../effects/MoonEffect.hpp"
#include "../effects/StarfieldEffect.hpp"
#include "../engine/GameConstants.hpp"
#include "../engine/SceneInputHandler.hpp"
#include <vector>
#include <cmath>

GameScene::GameScene() : Scene(4800.0f, 900.0f, {12, 14, 28, 255}) {
    // Set up callbacks for the pause menu
    pauseMenu = std::make_unique<PauseMenuOverlay>(*this);
    pauseMenu->onResume = [this]() {
        // Resume game - close pause menu and resume physics
        paused = false;
        if (controlsOverlay) {
            controlsOverlay.reset();
        }
        SceneInputHandler* ih = getInputHandler();
        if (ih) {
            ih->clearAllInputs();
        }
    };

    pauseMenu->onClose = [this]() {
        // Close - handled by onPause which clears inputs
        // Kept empty to avoid double-cleanup
    };

    pauseMenu->onControlsSelected = [this]() {
        onControlsSelected();
    };

    pauseMenu->onExitSelected = [this]() {
        onExitSelected();
    };
}

void GameScene::init() {
    PlayerActor* player = new PlayerActor({300.0f, 480.0f}, groundY);
    player->setInputHandler(getInputHandler());
    addActor(player);

    SceneActor* ground = new SceneActor({0.0f, groundY}, 4800.0f, 40.0f);
    ground->setTag("ground");
    ground->setColor({40, 44, 80, 255});
    addActor(ground);

    float platforms[][3] = {
        {600, 440, 160},  {900, 370, 120},  {1150, 300, 140},
        {1450, 380, 160}, {1700, 290, 120}, {2000, 360, 180},
        {2300, 430, 140}, {2600, 300, 160}, {2900, 380, 120},
        {3200, 260, 140}, {3500, 340, 160}, {3800, 420, 180},
        {4100, 300, 140}, {4400, 370, 120},
    };
    for (auto& p : platforms) {
        SceneActor* plat = new SceneActor({p[0], p[1]}, p[2], 16.0f);
        plat->setTag("platform");
        plat->setColor({60, 80, 140, 255});
        addActor(plat);
    }

    float enemyPositions[] = {800, 1300, 1800, 2400, 3000, 3600, 4200};
    for (float ex : enemyPositions) {
        SceneActor* enemy = new SceneActor({ex, groundY - 28.0f}, 28.0f, 28.0f);
        enemy->setTag("enemy");
        enemy->setColor({220, 60, 80, 255});
        addActor(enemy);
    }

    float gems[][2] = {
        {650, 400}, {950, 330}, {1200, 260}, {1500, 340},
        {1750, 250}, {2050, 320}, {2650, 260}, {3250, 220},
    };
    for (auto& g : gems) {
        SceneActor* gem = new SceneActor({g[0], g[1]}, 14.0f, 14.0f);
        gem->setTag("gem");
        gem->setColor({255, 220, 60, 255});
        addActor(gem);
    }

    getCamera()->setBoundary(0, -200, 4800.0f, 900.0f);
    getCamera()->setLookaheadEnabled(true);
    // Order matters: starfield first (farthest back), then the moon on top
    // of it. Effects render in the order they are added.
    addEffect(new StarfieldEffect(getCamera()));
    addEffect(new MoonEffect());
}

void GameScene::draw() {
    Camera2D cam = getCamera()->getRaylibCamera();

    Scene::draw();

    BeginMode2D(cam);
    for (SceneActor* a : getAllActors()) {
        if (a->getTag() == "gem") {
            Vector2 p = a->getPosition();
            float t = (float)GetTime();
            float pulse = 1.0f + 0.15f * sinf(t * 4.0f + p.x * 0.01f);
            DrawCircle((int)(p.x + 7), (int)(p.y + 7), 10.0f * pulse, {255, 220, 60, 40});
            DrawRectangleRounded({p.x, p.y, 14, 14}, 0.4f, 4, {255, 220, 60, 255});
        }
        if (a->getTag() == "enemy") {
            Vector2 p = a->getPosition();
            float t = (float)GetTime();
            float bob = sinf(t * 2.0f + p.x * 0.005f) * 2.0f;
            DrawRectangleRounded({p.x, p.y + bob, 28, 28}, 0.3f, 6, {220, 60, 80, 255});
            DrawCircle((int)(p.x + 8),  (int)(p.y + 10 + bob), 4, WHITE);
            DrawCircle((int)(p.x + 20), (int)(p.y + 10 + bob), 4, WHITE);
            DrawCircle((int)(p.x + 9),  (int)(p.y + 10 + bob), 2, {20, 0, 0, 255});
            DrawCircle((int)(p.x + 21), (int)(p.y + 10 + bob), 2, {20, 0, 0, 255});
        }
        if (a->getTag() == "platform") {
            Rectangle r = a->getBounds();
            DrawRectangle((int)r.x, (int)r.y, (int)r.width, 3, {120, 150, 220, 200});
        }
    }
    EndMode2D();

    // Draw controls overlay if active
    if (controlsOverlay) {
        controlsOverlay->draw();
    }

    // Draw pause menu overlay if paused and no controls overlay
    if (paused && !controlsOverlay && pauseMenu) {
        pauseMenu->draw();
    }
}

void GameScene::update(float deltaTime) {
    // Always update input handler first (needed for pause menu and controls overlay input)
    inputHandler.update();

    // Handle pause menu and controls overlay input (before Scene::update which would skip due to paused)
    if (controlsOverlay) {
        // Controls overlay takes priority - it handles its own input
        controlsOverlay->update(deltaTime);
        return;
    }

    if (paused && pauseMenu) {
        // Pause menu input handling
        pauseMenu->update(deltaTime, &inputHandler);
        return;
    }

    // Normal game update
    Scene::update(deltaTime);
    PlayerActor* player = (PlayerActor*)findActorByTag("player");
    if (!player) return;

    player->onSurface = false;

    SceneActor* ground = findActorByTag("ground");
    if (ground && player->isCollidingWith(ground) && player->getVelocity().y >= 0) {
        player->setPosition({player->getPosition().x, ground->getPosition().y - player->getHeight()});
        player->setVelocity({player->getVelocity().x, 0});
        player->onSurface = true;
    }

    for (SceneActor* a : getAllActors()) {
        if (a->getTag() == "platform" && player->isCollidingWith(a) && player->getVelocity().y >= 0) {
            Vector2 ppos = player->getPosition();
            Vector2 apos = a->getPosition();
            if (ppos.y + player->getHeight() < apos.y + 12.0f) {
                player->setPosition({ppos.x, apos.y - player->getHeight()});
                player->setVelocity({player->getVelocity().x, 0});
                player->onSurface = true;
            }
        }
    }

    std::vector<SceneActor*> toRemove;
    for (SceneActor* a : getAllActors()) {
        if (a->getTag() == "gem" && player->isCollidingWith(a))
            toRemove.push_back(a);
    }
    for (SceneActor* a : toRemove) removeActor(a);

    getCamera()->followActor(player, 4.0f);
    getCamera()->update(deltaTime);

    bool onGround = player->isOnGround() || player->onSurface;
    float vy = player->getVelocity().y;
    float targetZoom = 1.0f - (vy / 420.0f) * 0.25f;
    targetZoom = targetZoom < 0.72f ? 0.72f : targetZoom > 1.0f ? 1.0f : targetZoom;
    getCamera()->zoomTo(targetZoom, 0.1f);

    if (onGround && !landedLastFrame) getCamera()->shake(4.0f, 0.15f);
    landedLastFrame = onGround;
}

void GameScene::togglePause() {
    if (!paused) {
        // Opening pause menu
        paused = true;
        if (controlsOverlay) {
            controlsOverlay.reset();
        }
        if (pauseMenu) {
            pauseMenu->open();
        }
    } else {
        // Closing pause menu - clear inputs and resume game
        paused = false;
        if (controlsOverlay) {
            controlsOverlay.reset();
        }
        if (pauseMenu) {
            pauseMenu->close();

            // Clear all key states in input handler to prevent stuck inputs
            SceneInputHandler* ih = getInputHandler();
            if (ih) {
                ih->clearAllInputs();
            }
        }
    }
}

void GameScene::onControlsSelected() {
    // This callback is invoked when "Controls" is selected in pause menu
    // Create and show controls overlay within this scene (not a separate scene)
    if (!controlsOverlay && getInputHandler()) {
        controlsOverlay = std::make_unique<ControlsOverlay>(getInputHandler());
        controlsOverlay->open();
        // When closing controls, return to paused state with menu visible
        controlsOverlay->onClose = [this]() {
            if (controlsOverlay) {
                controlsOverlay.reset();
            }
            // Return to pause state so the menu shows
            paused = true;
            if (pauseMenu) {
                pauseMenu->open();
            }
        };
    }
}

void GameScene::onExitSelected() {
    // This callback is invoked when "Exit Game" is selected in pause menu
    // Set the global exit request flag to close the game
    extern bool exitRequested;  // Extern declaration from main.cpp
    exitRequested = true;
}
