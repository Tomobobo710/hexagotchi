#include "Hotbar.hpp"
#include "SceneCamera.hpp"
#include "SceneInputHandler.hpp"

// ============================================================================
// Hotbar implementation
// ============================================================================

Hotbar::Hotbar() {
    // Initialize slots with default types (food, ball/warmth, water)
    numSlots_ = MAX_SLOTS;

    slots_[0].index = 0;
    slots_[0].type = HexItemOverlay::ItemType::FOOD;
    slots_[0].label = "Food";
    slots_[0].careActionCode = 0;  // CARE_ACTION_FEED
    slots_[0].color = Color{160, 80, 40, 255};  // Orange-brown

    slots_[1].index = 1;
    slots_[1].type = HexItemOverlay::ItemType::BALL;
    slots_[1].label = "Ball";
    slots_[1].careActionCode = 1;  // CARE_ACTION_PET (warmth)
    slots_[1].color = Color{200, 60, 60, 255};  // Red

    slots_[2].index = 2;
    slots_[2].type = HexItemOverlay::ItemType::WATER;
    slots_[2].label = "Water";
    slots_[2].careActionCode = 4;  // CARE_ACTION_WATER
    slots_[2].color = Color{60, 140, 220, 255};  // Blue
}

void Hotbar::init(float screenWidth, float screenHeight) {
    screenWidth_ = screenWidth;
    screenHeight_ = screenHeight;

    updateSlotLayout();
}

void Hotbar::updateSlotLayout() {
    // Calculate hotbar dimensions. Slot labels draw INSIDE the slot's lower
    // edge rather than below it -- at 720px game height the old
    // below-slot label baseline landed a few px past the bottom of the
    // screen entirely.
    const float SLOT_SIZE = 60.0f;
    const float SLOT_SPACING = 14.0f;
    const float BAND_MARGIN = 14.0f;  // padding around slots inside the band
    const float BOTTOM_MARGIN = 14.0f; // gap from the very bottom of the screen

    float totalWidth = numSlots_ * (SLOT_SIZE + SLOT_SPACING) - SLOT_SPACING;
    float startX = (screenWidth_ - totalWidth) / 2.0f;

    hotbarBounds_ = {
        startX - BAND_MARGIN,
        screenHeight_ - BOTTOM_MARGIN - SLOT_SIZE - BAND_MARGIN * 2.0f,
        totalWidth + BAND_MARGIN * 2.0f,
        SLOT_SIZE + BAND_MARGIN * 2.0f
    };

    float startY = hotbarBounds_.y + BAND_MARGIN;

    // Position each slot
    for (int i = 0; i < numSlots_; i++) {
        slots_[i].bounds.x = startX + i * (SLOT_SIZE + SLOT_SPACING);
        slots_[i].bounds.y = startY;
        slots_[i].bounds.width = SLOT_SIZE;
        slots_[i].bounds.height = SLOT_SIZE;
    }
}

bool Hotbar::isMouseOverSlot() const {
    if (!inputHandler_) return false;

    Vector2 mousePos = inputHandler_->getMousePosition();

    for (int i = 0; i < numSlots_; i++) {
        if (CheckCollisionPointRec(mousePos, slots_[i].bounds)) {
            return true;
        }
    }
    return false;
}

bool Hotbar::isMouseOverSlot(int slotIndex) const {
    if (!inputHandler_) return false;

    Vector2 mousePos = inputHandler_->getMousePosition();
    return CheckCollisionPointRec(mousePos, slots_[slotIndex].bounds);
}

Vector2 Hotbar::getDragIconPosition() const {
    // The drag icon follows the cursor with an offset
    if (!inputHandler_) return {0, 0};

    Vector2 mousePos = inputHandler_->getMousePosition();

    // Offset slightly up so the cursor isn't covered
    return {mousePos.x - 16.0f, mousePos.y - 40.0f};
}

bool Hotbar::update(float deltaTime, SceneCamera* camera) {
    pendingDrop_ = false;
    droppedType_ = HexItemOverlay::ItemType::FOOD;
    droppedWorldPos_ = {0, 0};

    if (!inputHandler_) return false;

    Vector2 mousePos = inputHandler_->getMousePosition();

    // Check for slot presses
    bool anySlotPressed = false;
    int pressedSlotIndex = -1;

    for (int i = 0; i < numSlots_; i++) {
        if (CheckCollisionPointRec(mousePos, slots_[i].bounds)) {
            if (inputHandler_->isMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                // Start drag from this slot
                dragging_ = true;
                draggedItemType_ = slots_[i].type;
                dragOffset_ = {16.0f, 24.0f};  // Icon center offset
                anySlotPressed = true;
                pressedSlotIndex = i;
                break;
            }
        }
    }

    // Handle drag release
    if (dragging_ && inputHandler_->isMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
        // Convert screen mouse position to world position
        Vector2 worldPos = camera ? camera->screenToWorld(mousePos) : mousePos;

        // Report item type and world position - let the scene handle hex conversion
        pendingDrop_ = true;
        droppedType_ = draggedItemType_;
        droppedWorldPos_ = worldPos;

        dragging_ = false;
    }

    return pendingDrop_;
}

void Hotbar::draw() const {
    // Draw hotbar background band -- rounded pill, matching the rest of the
    // scene's HUD styling.
    DrawRectangleRounded(hotbarBounds_, 0.25f, 8, Color{25, 25, 40, 210});
    DrawRectangleRoundedLinesEx(hotbarBounds_, 0.25f, 8, 2.0f, Color{130, 130, 190, 220});

    // Draw slots
    for (int i = 0; i < numSlots_; i++) {
        const Slot& slot = slots_[i];

        // Slot background
        DrawRectangleRounded(slot.bounds, 0.2f, 6, Color{55, 55, 85, 235});
        DrawRectangleRoundedLinesEx(slot.bounds, 0.2f, 6, 2.0f, Color{130, 130, 180, 255});

        // Slot icon (placeholder circle), nudged up to leave room for the
        // label strip along the slot's bottom edge instead of below it.
        Vector2 center = {
            slot.bounds.x + slot.bounds.width / 2.0f,
            slot.bounds.y + slot.bounds.height * 0.40f
        };
        float radius = slot.bounds.width * 0.26f;

        DrawCircleV(center, radius, slot.color);
        DrawCircleLines(static_cast<int>(center.x), static_cast<int>(center.y),
                         radius, Color{255, 255, 255, 90});

        // Slot label as a small strip along the slot's bottom edge, fully
        // inside the slot bounds -- guarantees it never runs off-screen
        // regardless of game height.
        int fontSize = 11;
        const char* label = slot.label.c_str();
        float textWidth = MeasureText(label, fontSize);
        float labelY = slot.bounds.y + slot.bounds.height - fontSize - 4.0f;
        DrawText(label,
                 static_cast<int>(center.x - textWidth / 2.0f),
                 static_cast<int>(labelY),
                 fontSize, Color{225, 225, 240, 255});
    }

    // Draw dragged icon if dragging
    if (dragging_) {
        Vector2 pos = getDragIconPosition();
        float radius = 20.0f;

        // Find color for dragged item type
        Color dragColor = WHITE;
        switch (draggedItemType_) {
            case HexItemOverlay::ItemType::FOOD:
                dragColor = Color{160, 80, 40, 255};
                break;
            case HexItemOverlay::ItemType::BALL:
                dragColor = Color{200, 60, 60, 255};
                break;
            case HexItemOverlay::ItemType::WATER:
                dragColor = Color{60, 140, 220, 255};
                break;
        }

        DrawCircleV(pos, radius, dragColor);
        DrawCircleLines((int)pos.x, (int)pos.y, radius + 2.0f, WHITE);
    }
}

bool Hotbar::hasPendingDrop() const {
    return pendingDrop_;
}

HexItemOverlay::ItemType Hotbar::getDroppedItemType() const {
    return droppedType_;
}

Vector2 Hotbar::getDroppedWorldPosition() const {
    return droppedWorldPos_;
}

void Hotbar::clearPendingDrop() {
    pendingDrop_ = false;
    droppedWorldPos_ = {0, 0};
}
