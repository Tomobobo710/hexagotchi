#ifndef SCENE_EFFECT_HPP
#define SCENE_EFFECT_HPP

class SceneEffect {
public:
    virtual ~SceneEffect() = default;
    virtual void init() {}
    virtual void update(float deltaTime) {}
    virtual void drawBackground() {}
    virtual void drawForeground() {}
    virtual void cleanup() {}
};

#endif
