#ifndef PLAYER_ACTOR_HPP
#define PLAYER_ACTOR_HPP

#include "SceneActor.hpp"
#include "SceneInputHandler.hpp"
#include <vector>

// Simple trail point
struct TrailPoint { Vector2 pos; float life; };

class PlayerActor : public SceneActor {
public:
    std::vector<TrailPoint> trail;
    bool wasOnGround = true;
    float groundY = 500.0f;
    float coyoteTime = 0.0f;
    bool onSurface = false;  // set by scene each frame

    PlayerActor(Vector2 pos, float gY = 500.0f);

    void setInputHandler(SceneInputHandler* handler);
    void update(float deltaTime) override;
    bool isOnGround() const;
    void draw() override;

private:
    SceneInputHandler* inputHandler = nullptr;
};

#endif
