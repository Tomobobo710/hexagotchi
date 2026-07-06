#ifndef STARFIELD_EFFECT_HPP
#define STARFIELD_EFFECT_HPP

#include "SceneEffect.hpp"
#include "raylib.h"
#include <vector>

class SceneCamera;  // only referenced by pointer here; full def included in the .cpp

// Parallax starfield drawn as a background effect. It must be added to the
// scene BEFORE the moon so the moon (a nearer body) is drawn on top of and
// occludes the stars, rather than stars painting over the moon.
class StarfieldEffect : public SceneEffect {
public:
    StarfieldEffect(SceneCamera* cam);

    void init() override;
    void drawBackground() override;

private:
    SceneCamera* camera;
    std::vector<Vector2> stars;
    std::vector<Vector2> midStars;
};

#endif
