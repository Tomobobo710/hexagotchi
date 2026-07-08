#include "TomSceneActor.hpp"
#include "AssetPack.hpp"

TomSceneActor::TomSceneActor(Vector2 position, float width, float height)
    : SceneActor(position, width, height) {
}

TomSceneActor::~TomSceneActor() {
    if (ownsTexture && texture.id != 0) {
        UnloadTexture(texture);
    }
}

void TomSceneActor::loadFromAssetKey(const std::string& key, bool keepSize) {
    if (ownsTexture && texture.id != 0) {
        UnloadTexture(texture);
        ownsTexture = false;
    }

    Texture2D tex = AssetPack::loadTexture(key);
    if (tex.id == 0) return;

    assetKey = key;
    setTexture(tex);
    ownsTexture = true;

    if (!keepSize) {
        setSize((float)tex.width, (float)tex.height);
    }
}

const std::string& TomSceneActor::getAssetKey() const {
    return assetKey;
}
