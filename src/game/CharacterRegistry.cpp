#include "CharacterRegistry.hpp"
#include "AssetPack.hpp"

namespace {

// Emotion order matches the Emotion enum: [0]=Sad, [1]=Mid, [2]=Happy.
// Tom's files are named "gotchiportrait<emotion>" (a holdover from before
// the tomagotchi-stat-screen portraits and these scene portraits shared a
// folder) while every other character is "<name><emotion>" directly --
// intentional, not a bug to "fix" by renaming the files.
const CharacterInfo kTom = {
    {139, 172, 15, 255}, {139, 172, 15, 255},
    {"portraits/tom/gotchiportraitsad.png", "portraits/tom/gotchiportraitmid.png", "portraits/tom/gotchiportraithappy.png"},
    {"poses/tom/tomposesad.png", "poses/tom/tomposemid.png", "poses/tom/tomposehappy.png"},
};

// Karen: name label yellow. Body-art color kept at its original pink/red --
// the placeholder shape wasn't drawn to look right in yellow.
const CharacterInfo kKaren = {
    {230, 200, 40, 255}, {200, 60, 90, 255},
    {"portraits/karen/karensad.png", "portraits/karen/karenmid.png", "portraits/karen/karenhappy.png"},
    // No sad pose art -- loadPose() falls back to Mid for Emotion::Sad.
    {"", "poses/karen/karenposemid.png", "poses/karen/karenposehappy.png"},
};

// Ronzer: name label red. Body-art color kept at its original yellow -- the
// placeholder shape wasn't drawn to look right in red.
const CharacterInfo kRonzer = {
    {200, 60, 60, 255}, {250, 200, 40, 255},
    {"portraits/ronzer/ronzersad.png", "portraits/ronzer/ronzermid.png", "portraits/ronzer/ronzerhappy.png"},
    {"", "poses/ronzer/ronzerposemid.png", "poses/ronzer/ronzerposehappy.png"},
};

const CharacterInfo kJimmy = {
    {41, 128, 185, 255}, {41, 128, 185, 255},   // matches JS JIMMY color scheme (blue kid)
    {"portraits/jimmy/jimmysad.png", "portraits/jimmy/jimmymid.png", "portraits/jimmy/jimmyhappy.png"},
    {"poses/jimmy/jimmyposesad.png", "poses/jimmy/jimmyposemid.png", "poses/jimmy/jimmyposehappy.png"},
};

// No portrait art yet -- portraitPath[] left default-empty (see
// CharacterInfo's comment on empty meaning "no art for this emotion").
const CharacterInfo kTherapist = {{22, 160, 133, 255}, {22, 160, 133, 255}, {"", "", ""}, {"", "", ""}};
const CharacterInfo kBoss      = {{142, 68, 173, 255}, {142, 68, 173, 255}, {"", "", ""}, {"", "", ""}};
const CharacterInfo kNarrator  = {{150, 150, 170, 255}, {150, 150, 170, 255}, {"", "", ""}, {"", "", ""}};
const CharacterInfo kPhone     = {{230, 160, 60, 255}, {230, 160, 60, 255}, {"", "", ""}, {"", "", ""}};

} // namespace

const CharacterInfo& CharacterRegistry::get(CharacterId id) {
    switch (id) {
        case CharacterId::Tom:       return kTom;
        case CharacterId::Karen:     return kKaren;
        case CharacterId::Ronzer:    return kRonzer;
        case CharacterId::Jimmy:     return kJimmy;
        case CharacterId::Therapist: return kTherapist;
        case CharacterId::Boss:      return kBoss;
        case CharacterId::Narrator:  return kNarrator;
        case CharacterId::Phone:     return kPhone;
    }
    return kNarrator; // unreachable -- silences -Wreturn-type on some compilers
}

Texture2D CharacterRegistry::loadPortrait(CharacterId id, Emotion emotion) {
    const CharacterInfo& info = get(id);
    const std::string& path = info.portraitPath[(int)emotion];
    if (!path.empty()) return AssetPack::loadTexture(path);

    const std::string& midPath = info.portraitPath[(int)Emotion::Mid];
    if (!midPath.empty()) return AssetPack::loadTexture(midPath);

    return Texture2D{0};
}

Texture2D CharacterRegistry::loadPose(CharacterId id, Emotion emotion) {
    const CharacterInfo& info = get(id);
    const std::string& path = info.posePath[(int)emotion];
    if (!path.empty()) return AssetPack::loadTexture(path);

    const std::string& midPath = info.posePath[(int)Emotion::Mid];
    if (!midPath.empty()) return AssetPack::loadTexture(midPath);

    return Texture2D{0};
}
