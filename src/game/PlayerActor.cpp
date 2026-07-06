#include "PlayerActor.hpp"

PlayerActor::PlayerActor(Vector2 pos, float gY)
    : SceneActor(pos, 28.0f, 36.0f), groundY(gY) {
    setTag("player");
    setLayer(ACTOR_LAYER_MIDGROUND);
    setColor({80, 160, 255, 255});
    setGravityEnabled(true);
}

void PlayerActor::setInputHandler(SceneInputHandler* handler) {
    inputHandler = handler;
}

void PlayerActor::update(float deltaTime) {
    // Trail
    trail.push_back({getCenter(), 0.18f});
    if (trail.size() > 12) trail.erase(trail.begin());
    for (auto& t : trail) t.life -= deltaTime;

    SceneActor::update(deltaTime);

    float speed = IsKeyDown(KEY_LEFT_SHIFT) ? 220.0f : 150.0f;

    if (inputHandler) {
        // Use input handler for movement actions
        if (inputHandler->isActionHeld(INPUT_ACTION_MOVE_LEFT))  velocity.x = -speed;
        else if (inputHandler->isActionHeld(INPUT_ACTION_MOVE_RIGHT)) velocity.x = speed;
        else velocity.x *= 0.85f;

        // Jump with coyote time
        bool playerOnGround = isOnGround() || onSurface;
        if (playerOnGround) coyoteTime = 0.12f;
        else coyoteTime -= deltaTime;

        if ((inputHandler->isActionPressed(INPUT_ACTION_JUMP) || inputHandler->isActionPressed(INPUT_ACTION_MOVE_UP)) && coyoteTime > 0) {
            velocity.y = -420.0f;
            coyoteTime = 0;
        }
        wasOnGround = playerOnGround;
    } else {
        // Fallback to hardcoded keys if no input handler
        if (IsKeyDown(KEY_A) || IsKeyDown(KEY_LEFT))  velocity.x = -speed;
        else if (IsKeyDown(KEY_D) || IsKeyDown(KEY_RIGHT)) velocity.x = speed;
        else velocity.x *= 0.85f;

        bool playerOnGround = isOnGround() || onSurface;
        if (playerOnGround) coyoteTime = 0.12f;
        else coyoteTime -= deltaTime;

        if ((IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) && coyoteTime > 0) {
            velocity.y = -420.0f;
            coyoteTime = 0;
        }
        wasOnGround = playerOnGround;
    }
}

bool PlayerActor::isOnGround() const {
    return position.y + height >= groundY - 1.0f;
}

void PlayerActor::draw() {
    // Draw trail
    for (int i = 0; i < (int)trail.size(); i++) {
        float t = (float)i / trail.size();
        unsigned char a = (unsigned char)(t * 80);
        float r = 6.0f * t;
        DrawCircle((int)trail[i].pos.x, (int)trail[i].pos.y, r, {80, 160, 255, a});
    }
    // Body
    DrawRectangleRounded({position.x, position.y, width, height}, 0.3f, 6, color);
    // Eyes
    float eyeY = position.y + height * 0.28f;
    float facing = velocity.x >= 0 ? 1.0f : -1.0f;
    DrawCircle((int)(position.x + width * 0.5f + facing * 6), (int)eyeY, 4, WHITE);
    DrawCircle((int)(position.x + width * 0.5f + facing * 7), (int)eyeY, 2, {20, 20, 60, 255});
}
