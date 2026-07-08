#ifndef TOM_SCENE_ACTOR_HPP
#define TOM_SCENE_ACTOR_HPP

#include "SceneActor.hpp"
#include <string>

// SceneActor subclass for Tom's world-scenes and the scene editor (tools/
// scene_editor.cpp). Kept separate from the base engine so this can evolve
// freely (asset-key tracking, editor round-tripping) without touching
// SceneActor, which Bazola's tomagotchi/hexboard side also depends on.
//
// Adds a texture-by-key convenience on top of SceneActor::setTexture() --
// `assetKey` is the same relative-path key AssetPack::loadTexture() and the
// scene editor's asset manifest both use (e.g. "props/lamp.png"), so an actor
// placed in the editor and one constructed in real scene code both point at
// the same underlying image the same way.
class TomSceneActor : public SceneActor {
public:
    TomSceneActor(Vector2 position, float width, float height);
    ~TomSceneActor() override;

    // Loads assetKey via AssetPack and sets it as this actor's texture, sizing
    // width/height to the loaded texture's pixel dimensions unless keepSize
    // is true. Actor owns the texture from here on (unloaded in destructor).
    void loadFromAssetKey(const std::string& key, bool keepSize = false);

    const std::string& getAssetKey() const;

private:
    std::string assetKey;
    bool ownsTexture = false;
};

#endif // TOM_SCENE_ACTOR_HPP
