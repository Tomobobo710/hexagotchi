#include "HexItemOverlay.hpp"
#include "EventType.h"
#include "HexTile.hpp"

// ============================================================================
// HexItemOverlay implementation
// ============================================================================

HexItemOverlay::HexItemOverlay(HexCoords hex, ItemType itemType, Vector2 position, float hexSize)
    : hex_(hex), itemType_(itemType), position_(position) {
    // Set visual properties based on item type
    switch (itemType_) {
        case ItemType::FOOD:
            itemName_ = "food";
            itemColor_ = Color{160, 80, 40, 255};  // Orange-brown for food
            break;
        case ItemType::BALL:
            itemName_ = "ball";
            itemColor_ = Color{200, 60, 60, 255};  // Red for ball/warmth
            break;
        case ItemType::WATER:
            itemName_ = "water";
            itemColor_ = Color{60, 140, 220, 255};  // Blue for water
            break;
    }
}

bool HexItemOverlay::update(float deltaTime, HexCoords gotchiCurrentHex) {
    pulseTimer_ += deltaTime * 3.0f;  // Pulse animation speed

    // Check if gotchi has arrived at this hex (compare hex coords directly)
    if (gotchiCurrentHex.q == hex_.q && gotchiCurrentHex.r == hex_.r) {
        // Gotchi arrived - fire effect and signal for removal
        return true;
    }
    return false;
}

void HexItemOverlay::draw(float hexSize) const {
    // Use the stored position directly - no conversion needed
    Vector2 centerPos = position_;

    // Draw the icon - a simple circle with pulse effect
    float radius = hexSize * 0.35f;
    float pulseScale = 1.0f + sin(pulseTimer_) * 0.15f;  // 15% pulse
    float drawRadius = radius * pulseScale;

    // Draw the main icon circle
    DrawCircleV(centerPos, drawRadius, itemColor_);

    // Draw a white ring for visibility
    DrawCircleLines((int)centerPos.x, (int)centerPos.y, radius + 4.0f, WHITE);

    // Small indicator dot in center
    float dotScale = 0.4f;
    DrawCircleV(centerPos, drawRadius * dotScale, Color{255, 255, 255, 200});
}

int HexItemOverlay::careActionCodeForItem(ItemType type) {
    // Match the codes in GotchiSim.hpp:
    // CARE_ACTION_FEED = 0, CARE_ACTION_PET = 1, CARE_ACTION_WASH = 2, CARE_ACTION_WATER = 4

    switch (type) {
        case ItemType::FOOD:
            return 0;  // CARE_ACTION_FEED
        case ItemType::BALL:
            return 1;  // CARE_ACTION_PET (warmth action - affection)
        case ItemType::WATER:
            return 4;  // CARE_ACTION_WATER
        default:
            return -1;
    }
}
