#include "BossScene.hpp"
#include "PlayerActor.hpp"
#include "PauseMenuOverlay.hpp"
#include "GameConstants.hpp"
#include <cmath>

BossScene::BossScene() : Scene(2400.0f, 900.0f, {8, 4, 18, 255}) {
}

BossScene::~BossScene() {
}

void BossScene::init() {
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

    PlayerActor* player = new PlayerActor({300.0f, 440.0f}, groundY);
    // Snap the camera to the player immediately -- followActor() alone only
    // sets targetPosition, leaving the camera to lerp in from the
    // constructor's default (scene center) over the first several frames.
    getCamera()->followActor(player, 2.5f);
    getCamera()->setPosition(player->getPosition());
    addActor(player);

    SceneActor* ground = new SceneActor({0.0f, groundY}, 2400.0f, 40.0f);
    ground->setTag("ground");
    ground->setColor({30, 20, 50, 255});
    addActor(ground);

    SceneActor* boss = new SceneActor({1100.0f, groundY - 110.0f}, 90.0f, 110.0f);
    boss->setTag("boss");
    boss->setColor({200, 50, 80, 255});
    addActor(boss);

    float pillars[] = {500, 800, 1400, 1700};
    for (float px : pillars) {
        SceneActor* pillar = new SceneActor({px, groundY - 180.0f}, 40.0f, 180.0f);
        pillar->setTag("platform");
        pillar->setColor({40, 30, 60, 255});
        addActor(pillar);
    }

    getCamera()->setBoundary(0, -100, 2400.0f, 900.0f);
}

void BossScene::draw() {
    int cx = GAME_W / 2;
    int cy = GAME_H / 2;
    for (int i = 5; i >= 1; i--) {
        float r = 80.0f * i + sinf((float)GetTime() * 0.5f) * 10;
        unsigned char a = (unsigned char)(15 + i * 6);
        DrawCircleLines(cx, cy + 100, r, {180, 60, 100, a});
    }

    Scene::draw();

    // Draw pause menu overlay if paused
    if (paused && pauseMenu) {
        pauseMenu->draw();
    }

    Camera2D cam = getCamera()->getRaylibCamera();
    BeginMode2D(cam);

    SceneActor* boss = findActorByTag("boss");
    if (boss) {
        Vector2 bp = boss->getPosition();
        float t = (float)GetTime();
        for (int i = 3; i >= 1; i--) {
            DrawRectangleRounded(
                {bp.x - i * 4.0f, bp.y - i * 4.0f, 90.0f + i * 8.0f, 110.0f + i * 8.0f},
                0.2f, 6, {200, 50, 80, (unsigned char)(20 * i)}
            );
        }
        DrawRectangleRounded({bp.x, bp.y, 90, 110}, 0.15f, 6, {200, 50, 80, 255});
        float eyePulse = 1.0f + 0.2f * sinf(t * 3.0f);
        DrawCircle((int)(bp.x + 45), (int)(bp.y + 40), 18.0f * eyePulse, {255, 200, 50, 255});
        DrawCircle((int)(bp.x + 45), (int)(bp.y + 40), 10.0f * eyePulse, {40, 10, 10, 255});
        DrawRectangle((int)bp.x - 5, (int)bp.y - 20, 100, 10, {60, 20, 20, 255});
        DrawRectangle((int)bp.x - 5, (int)bp.y - 20, (int)(80 + 20 * sinf(t * 0.3f)), 10, {220, 50, 80, 255});
        DrawRectangleLines((int)bp.x - 5, (int)bp.y - 20, 100, 10, {255, 100, 120, 255});
    }

    for (SceneActor* a : getAllActors()) {
        if (a->getTag() == "platform") {
            Rectangle r = a->getBounds();
            DrawRectangle((int)r.x, (int)r.y, 4, (int)r.height, {100, 60, 140, 120});
        }
    }
    EndMode2D();
}

void BossScene::update(float deltaTime) {
    Scene::update(deltaTime);

    if (paused) {
        if (pauseMenu) pauseMenu->update(deltaTime);
        return;
    }

    bossPhase += deltaTime;

    PlayerActor* player = (PlayerActor*)findActorByTag("player");
    if (!player) return;

    SceneActor* ground = findActorByTag("ground");
    if (ground && player->isCollidingWith(ground) && player->getVelocity().y > 0) {
        player->setPosition({player->getPosition().x, ground->getPosition().y - player->getHeight()});
        player->setVelocity({player->getVelocity().x, 0});
    }

    for (SceneActor* a : getAllActors()) {
        if (a->getTag() == "platform" && player->isCollidingWith(a) && player->getVelocity().y > 0) {
            Vector2 ppos = player->getPosition();
            Vector2 apos = a->getPosition();
            if (ppos.y + player->getHeight() < apos.y + 12.0f) {
                player->setPosition({ppos.x, apos.y - player->getHeight()});
                player->setVelocity({player->getVelocity().x, 0});
            }
        }
    }

    getCamera()->followActor(player, 2.5f);
    getCamera()->update(deltaTime);

    bool onGround = player->isOnGround();
    float vy = player->getVelocity().y;
    if (!onGround && vy < -50) getCamera()->zoomTo(0.75f, 0.35f);
    else if (onGround) {
        if (!landedLastFrame) getCamera()->shake(5.0f, 0.2f);
        getCamera()->zoomTo(1.0f, 0.3f);
    }
    landedLastFrame = onGround;
}

void BossScene::togglePause() {
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

void BossScene::onExitSelected() {
    extern bool exitRequested;
    exitRequested = true;
}
