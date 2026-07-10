#ifndef HEX_ITEM_OVERLAY_HPP
#define HEX_ITEM_OVERLAY_HPP

#include "raylib.h"
#include "HexTile.hpp"
#include <string>

// HexItemOverlay - an icon placed on a hex tile when dragging from the hotbar
// It uses world-space positioning so it moves with camera pan/zoom.
// The position is computed once at creation using the canonical hex conversion.

class HexItemOverlay {
public:
    // Item types supported
    enum class ItemType {
        FOOD,    // Hunger reduction
        BALL,    // Affection/warmth (pet/play family)
        WATER    // Hygiene reduction
    };

    // Construct with hex coordinates and pixel position - position is stored directly
    HexItemOverlay(HexCoords hex, ItemType itemType, Vector2 position, float hexSize);
    ~HexItemOverlay() = default;

    // Update - check if gotchi has arrived and fire the effect
    // Returns true if the overlay should be removed (consumed)
    bool update(float deltaTime, HexCoords gotchiCurrentHex);

    // Draw - render the icon centered on this hex's pixel position
    void draw(float hexSize) const;

    // Accessors
    HexCoords getHex() const { return hex_; }
    ItemType getItemType() const { return itemType_; }

    // Static helper: convert item type to CareAction code
    static int careActionCodeForItem(ItemType type);

private:
    HexCoords hex_;              // Hex coordinates (q, r)
    ItemType itemType_;

    // Computed world position (set at creation time from canonical conversion)
    Vector2 position_;

    // Visual properties (colors for placeholders)
    Color itemColor_ = WHITE;
    std::string itemName_ = "";

    // Animation
    float pulseTimer_ = 0.0f;
};

#endif // HEX_ITEM_OVERLAY_HPP
