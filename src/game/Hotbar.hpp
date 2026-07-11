#ifndef HOTBAR_HPP
#define HOTBAR_HPP

#include "raylib.h"
#include "HexItemOverlay.hpp"
#include <string>

// Hotbar - a screen-space palette of care items at the bottom of the screen
// Drag from slots to place items on the hex board.

class SceneCamera;
class SceneInputHandler;

class Hotbar {
public:
    Hotbar();
    ~Hotbar() = default;

    // Initialize - sets up slots and their positions
    void init(float screenWidth, float screenHeight);

    // Set input handler (required for mouse/input queries)
    void setInputHandler(SceneInputHandler* ih) { inputHandler_ = ih; }

    // Update - handles drag state and slot detection
    // Returns true if a valid drop occurred (item placed on board)
    // On drop, reports: item type and world position where it was released
    bool update(float deltaTime, SceneCamera* camera);

    // Get the dropped item if one was just placed (cleared after retrieval)
    bool hasPendingDrop() const;
    HexItemOverlay::ItemType getDroppedItemType() const;
    Vector2 getDroppedWorldPosition() const;  // World position of drop release
    void clearPendingDrop();

    // Draw - renders the hotbar at screen bottom
    void draw() const;

    // Check if mouse is over any slot (for input gating)
    bool isMouseOverSlot() const;

    // Screen-space bounds of the whole hotbar band, for positioning other
    // UI (e.g. a hint label) relative to it.
    Rectangle getBounds() const { return hotbarBounds_; }

private:
    // Slot definition
    struct Slot {
        int index;                    // Slot number (0, 1, 2)
        HexItemOverlay::ItemType type; // Item type for this slot
        Rectangle bounds;             // Screen-space position/size
        Color color;                  // Visual color for placeholder

        std::string label;
        int careActionCode;           // For event emission
    };

    // Items supported (2-3 per design)
    static const int MAX_SLOTS = 3;

    Slot slots_[MAX_SLOTS];
    int numSlots_ = 0;

    // Drag state
    bool dragging_ = false;
    HexItemOverlay::ItemType draggedItemType_ = HexItemOverlay::ItemType::FOOD;
    Vector2 dragOffset_;            // Mouse offset from slot center
    Vector2 dragPosition_;          // Current mouse position during drag

    // Drop state
    bool pendingDrop_ = false;
    HexItemOverlay::ItemType droppedType_ = HexItemOverlay::ItemType::FOOD;
    Vector2 droppedWorldPos_;       // World position where item was released

    // Screen layout
    float screenWidth_;
    float screenHeight_;

    // Hotbar area (bottom band)
    Rectangle hotbarBounds_;

    // Input handler (owned by scene, passed for queries)
    SceneInputHandler* inputHandler_ = nullptr;

    // Helper methods
    void updateSlotLayout();
    bool isMouseOverSlot(int slotIndex) const;
    Vector2 getDragIconPosition() const;
};

#endif // HOTBAR_HPP
