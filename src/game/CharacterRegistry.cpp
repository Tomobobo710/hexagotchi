#include "CharacterRegistry.hpp"
#include "AssetPack.hpp"

namespace {

// Portrait order matches PortraitEmotion: [0]=Sad, [1]=Mid, [2]=Happy.
// Pose order matches PoseEmotion: [0]=Sad, [1]=Mid, [2]=Happy, [3]=Scared.
const CharacterInfo kTom = {
    "Tom",
    {139, 172, 15, 255}, {139, 172, 15, 255},
    {40, 160, 60, 255}, {15, 60, 25, 255},
    {"portraits/tom/tomportraitsad.png", "portraits/tom/tomportraitmid.png", "portraits/tom/tomportraithappy.png"},
    {"poses/tom/tomposesad.png", "poses/tom/tomposemid.png", "poses/tom/tomposehappy.png", "poses/tom/tomposescared.png"},
};

// Karen: name label yellow. Body-art color kept at its original pink/red --
// the placeholder shape wasn't drawn to look right in yellow.
const CharacterInfo kKaren = {
    "Karen",
    {230, 200, 40, 255}, {200, 60, 90, 255},
    {230, 200, 40, 255}, {120, 100, 10, 255},
    {"portraits/karen/karensad.png", "portraits/karen/karenmid.png", "portraits/karen/karenhappy.png"},
    {"poses/karen/karenposesad.png", "poses/karen/karenposemid.png", "poses/karen/karenposehappy.png", "poses/karen/karenposescared.png"},
};

// Ronzer: name label red. Body-art color kept at its original yellow -- the
// placeholder shape wasn't drawn to look right in red.
const CharacterInfo kRonzer = {
    "Ronzer",
    {200, 60, 60, 255}, {250, 200, 40, 255},
    {200, 40, 40, 255}, {90, 10, 10, 255},
    {"portraits/ronzer/ronzersad.png", "portraits/ronzer/ronzermid.png", "portraits/ronzer/ronzerhappy.png"},
    {"", "poses/ronzer/ronzerposemid.png", "poses/ronzer/ronzerposehappy.png", "poses/ronzer/ronzerposescared.png"},
};

const CharacterInfo kJimmy = {
    "Jimmy",
    {41, 128, 185, 255}, {41, 128, 185, 255},   // matches JS JIMMY color scheme (blue kid)
    {41, 128, 185, 255}, {15, 55, 90, 255},
    {"portraits/jimmy/jimmysad.png", "portraits/jimmy/jimmymid.png", "portraits/jimmy/jimmyhappy.png"},
    {"poses/jimmy/jimmyposesad.png", "poses/jimmy/jimmyposemid.png", "poses/jimmy/jimmyposehappy.png", "poses/jimmy/jimmyposescared.png"},
};

const CharacterInfo kJudy = {
    "Judy",
    {255, 255, 255, 255}, {22, 160, 133, 255},
    {200, 200, 200, 255}, {70, 70, 70, 255},
    {"portraits/judy/judysad.png", "portraits/judy/judymid.png", "portraits/judy/judyhappy.png"},
    {"poses/judy/judyposesad.png", "poses/judy/judyposemid.png", "poses/judy/judyposehappy.png", "poses/judy/judyposescared.png"},
};

const CharacterInfo kLarry = {
    "Larry",
    {170, 170, 170, 255}, {142, 68, 173, 255},
    {110, 110, 110, 255}, {10, 10, 10, 255},   // portrait gradient: grey -> black
    {"portraits/larry/larrysad.png", "portraits/larry/larrymid.png", "portraits/larry/larryhappy.png"},
    {"poses/larry/larryposesad.png", "poses/larry/larryposemid.png", "poses/larry/larryposehappy.png", "poses/larry/larryposescared.png"},
};
// Loraine: Larry's secretary. Name label (and portrait gradient) orange --
// her whole dialog-box identity is orange.
const CharacterInfo kLoraine = {
    "Loraine",
    {235, 140, 30, 255}, {235, 140, 30, 255},
    {235, 140, 30, 255}, {110, 60, 10, 255},
    {"portraits/loraine/lorainesad.png", "portraits/loraine/lorainemid.png", "portraits/loraine/lorainehappy.png"},
    {"poses/loraine/loraineposesad.png", "poses/loraine/loraineposemid.png", "poses/loraine/loraineposehappy.png", "poses/loraine/loraineposescared.png"},
};

// Mark: Tom's apartment maintenance guy. Pink identity. Only one pose
// (markpose.png, no emotion suffix) and one portrait (markmid.png) exist --
// both mapped to Mid; loadPose/loadPortrait fall back to Mid for the rest.
// He's a deadpan straight man, so a single neutral expression fits.
const CharacterInfo kMark = {
    "Mark",
    {235, 90, 160, 255}, {235, 90, 160, 255},
    {235, 90, 160, 255}, {110, 40, 75, 255},
    {"", "portraits/mark/markmid.png", ""},
    {"", "poses/mark/markpose.png", "", ""},
};

const CharacterInfo kNarrator = {
    "Narrator",
    {150, 150, 170, 255}, {150, 150, 170, 255},
    {150, 150, 170, 255}, {70, 70, 85, 255},
    {"", "portraits/narrator/narratormid.png", ""}, {"", "", "", ""},
};

// Only "mid" art exists for the phone-text insert portrait -- loadPortrait()
// falls back to Mid for Sad/Happy automatically.
const CharacterInfo kPhone = {
    "Karen (text)",
    {230, 160, 60, 255}, {230, 160, 60, 255},
    {160, 110, 30, 255}, {70, 45, 10, 255},
    {"", "portraits/phone/phonemid.png", ""},
    {"", "", "", ""},
};

} // namespace

const CharacterInfo& CharacterRegistry::get(CharacterId id) {
    switch (id) {
        case CharacterId::Tom:       return kTom;
        case CharacterId::Karen:     return kKaren;
        case CharacterId::Ronzer:    return kRonzer;
        case CharacterId::Jimmy:     return kJimmy;
        case CharacterId::Judy:      return kJudy;
        case CharacterId::Larry:     return kLarry;
        case CharacterId::Loraine:   return kLoraine;
        case CharacterId::Mark:      return kMark;
        case CharacterId::Narrator:  return kNarrator;
        case CharacterId::Phone:     return kPhone;
    }
    return kNarrator; // unreachable -- silences -Wreturn-type on some compilers
}

Texture2D CharacterRegistry::loadPortrait(CharacterId id, PortraitEmotion emotion) {
    const CharacterInfo& info = get(id);
    const std::string& path = info.portraitPath[(int)emotion];
    if (!path.empty()) return AssetPack::loadTexture(path);

    const std::string& midPath = info.portraitPath[(int)PortraitEmotion::Mid];
    if (!midPath.empty()) return AssetPack::loadTexture(midPath);

    return Texture2D{0};
}

Texture2D CharacterRegistry::loadPose(CharacterId id, PoseEmotion emotion) {
    const CharacterInfo& info = get(id);
    const std::string& path = info.posePath[(int)emotion];
    if (!path.empty()) return AssetPack::loadTexture(path);

    const std::string& midPath = info.posePath[(int)PoseEmotion::Mid];
    if (!midPath.empty()) return AssetPack::loadTexture(midPath);

    return Texture2D{0};
}
