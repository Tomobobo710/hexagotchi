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
    Judy,
    Larry,
    Loraine,        // Larry's secretary
    Narrator,
    Phone,          // "Karen (text)" phone-insert lines in ApartmentScene
};

// Portrait art only ever comes in these three expressions.
enum class PortraitEmotion {
    Sad,
    Mid,
    Happy,
};

// Pose art has one more expression than portraits (Scared) -- kept as a
// separate enum from PortraitEmotion rather than reusing one shared type,
// since the two art sets are genuinely different categories with different
// sizes, not the same list at two points in time.
enum class PoseEmotion {
    Sad,
    Mid,
    Happy,
    Scared,
};

struct CharacterInfo {
    // Display name shown on the dialog box's name plate. Scene-specific
    // one-off overrides (e.g. "Larry (Tom's boss)" for a first-time intro
    // line) are passed separately to DialogBox::setCharacter() rather than
    // living here, since they're not this character's actual name.
    std::string displayName;

    // Dialogue speaker-name label color -- this is the character's
    // "identity color" (Karen=yellow, Ronzer=red).
    Color nameColor;
    // Placeholder silhouette-art fill color, used only by each scene's
    // hand-drawn shape stand-ins (drawWife/drawKaren/drawPokemon/etc).
    // Deliberately separate from nameColor: the shape art was tuned to look
    // right at its own color and isn't meant to follow the name label if
    // that gets changed for legibility/identity reasons.
    Color bodyColor;

    // Portrait background gradient (top/bottom), drawn behind the portrait
    // texture in the dialog box. One place per character to tune instead of
    // every scene's playLine() hand-picking its own gradient inline --
    // that's what caused Judy's portrait background not matching her name
    // color in the first place.
    Color portraitGradientTop;
    Color portraitGradientBottom;
    // Portrait asset keys (relative to assets/, matches AssetPack::loadTexture
    // keys), indexed by PortraitEmotion. Empty string means no art for that
    // emotion -- callers should fall back to Mid, or to no portrait at all if
    // Mid is also empty (e.g. Narrator, which has no portrait art).
    std::string portraitPath[3];

    // Full-body pose asset keys, indexed by PoseEmotion. Same empty-string/
    // fall-back-to-Mid convention as portraitPath. Characters without pose
    // art (Narrator, Phone) are left default-empty.
    std::string posePath[4];
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
    Texture2D loadPortrait(CharacterId id, PortraitEmotion emotion);

    // Same convention as loadPortrait() but for full-body pose art. Returns
    // a null Texture2D (id == 0) for characters with no pose art at all --
    // callers should fall back to their own procedural drawing in that case.
    Texture2D loadPose(CharacterId id, PoseEmotion emotion);
}

#endif // CHARACTER_REGISTRY_HPP
