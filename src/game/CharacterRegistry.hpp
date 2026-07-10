#ifndef CHARACTER_REGISTRY_HPP
#define CHARACTER_REGISTRY_HPP

#include "raylib.h"
#include <string>

// One place per recurring character: name-label color and portrait
// texture paths per emotion. Scenes previously each declared their own
// TOM_COLOR/KAREN_COLOR/etc static Color constants (with the same character
// sometimes ending up under different names in different files -- WIFE_COLOR
// vs KAREN_COLOR for the same person) and, in PizzaParlorScene, a private
// portraits[actor][emotion] Texture2D array. This is the single source of
// truth both should pull from instead, so a color/portrait fix in one place
// applies everywhere that character appears.
enum class CharacterId {
    Tom,
    Karen,
    Ronzer,
    Jimmy,
    Therapist,
    Larry,
    Narrator,
    Phone,          // "Karen (text)" phone-insert lines in ApartmentScene
};

// Only the emotions we actually have portrait art for today. Add to this
// (and to CharacterRegistry.cpp's portrait path tables) as more art lands --
// characters that don't have a given emotion's art yet just fall back to
// Mid in getPortraitPath()/loadPortrait().
enum class Emotion {
    Sad,
    Mid,
    Happy,
};

struct CharacterInfo {
    // Dialogue speaker-name label color (and portrait background gradient) --
    // this is the character's "identity color" (Karen=yellow, Ronzer=red).
    Color nameColor;
    // Placeholder silhouette-art fill color, used only by each scene's
    // hand-drawn shape stand-ins (drawWife/drawKaren/drawPokemon/etc).
    // Deliberately separate from nameColor: the shape art was tuned to look
    // right at its own color and isn't meant to follow the name label if
    // that gets changed for legibility/identity reasons.
    Color bodyColor;
    // Portrait asset keys (relative to assets/, matches AssetPack::loadTexture
    // keys), indexed by Emotion. Empty string means no art for that emotion --
    // callers should fall back to Mid, or to no portrait at all if Mid is
    // also empty (e.g. Narrator/Larry/Therapist, which have no portrait art).
    std::string portraitPath[3];

    // Full-body pose asset keys, same convention/fallback rules as
    // portraitPath. Separate art from portraits (poses/ vs portraits/) --
    // characters without pose art (Therapist, Larry, Narrator, Phone) are
    // left default-empty and scenes fall back to their procedural silhouette
    // drawing for those.
    std::string posePath[3];
};

namespace CharacterRegistry {
    // Returns this character's registered color/portrait info. Always
    // returns a valid reference -- unregistered ids simply aren't reachable
    // since CharacterId is a closed enum.
    const CharacterInfo& get(CharacterId id);

    // Convenience: loads (via AssetPack) the texture for this character's
    // emotion, falling back to Mid then to a null Texture2D (id == 0) if
    // neither is registered. Caller owns the result and must UnloadTexture()
    // it, same convention as every other AssetPack-backed texture load.
    Texture2D loadPortrait(CharacterId id, Emotion emotion);

    // Same convention as loadPortrait() but for full-body pose art. Returns
    // a null Texture2D (id == 0) for characters with no pose art at all --
    // callers should fall back to their own procedural drawing in that case.
    Texture2D loadPose(CharacterId id, Emotion emotion);
}

#endif // CHARACTER_REGISTRY_HPP
