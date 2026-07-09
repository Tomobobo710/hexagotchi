#ifndef SAVE_WIRING_H
#define SAVE_WIRING_H

#include "EventBus.h"
#include "SaveManager.h"
#include "EventType.h"
#include <functional>
#include <iostream>

// SaveWiring subscribes to checkpoint events and calls SaveManager::autosave()
// on StoryBeatCompleted, MergeBackCompleted, and scene transitions.
// GameState must be provided via setGameState() for autosave to work.
class SaveWiring {
public:
    SaveWiring(SaveManager& saveManager, EventBus& eventBus)
        : saveManager_(saveManager), eventBus_(eventBus), gameState_(nullptr) {
        // Subscribe to StoryBeatCompleted - checkpoint after a beat completes
        eventBus_.subscribe(EventType::StoryBeatCompleted,
            [this](const Event& e) {
                this->onCheckpoint("StoryBeatCompleted", e);
            });

        // Subscribe to MergeBackCompleted - when returning from merge to gotchi
        eventBus_.subscribe(EventType::MergeBackCompleted,
            [this](const Event& e) {
                this->onCheckpoint("MergeBackCompleted", e);
            });
    }

    // Set the GameState reference for autosave
    void setGameState(GameState* state) { gameState_ = state; }

    // Trigger autosave on scene transition
    // This is called from main.cpp when scene transitions complete
    void onSceneTransition() {
        onCheckpoint("SceneTransition", Event{EventType::StateChanged});
    }

    // Trigger autosave on checkpoint events
    void triggerAutosave(const char* source) {
        onCheckpoint(source, Event{EventType::StateChanged});
    }

private:
    SaveManager& saveManager_;
    EventBus& eventBus_;
    GameState* gameState_;

    void onCheckpoint(const char* source, const Event& e) {
        // Only autosave if an active slot is set AND we have GameState
        if (saveManager_.activeSlot() >= 0 && gameState_) {
            std::cout << "[SaveWiring] Autosave on " << source
                      << ", slot=" << saveManager_.activeSlot()
                      << " (storyBeatIndex=" << gameState_->storyBeatIndex
                      << ", mergeCount=" << gameState_->mergeCount << ")" << std::endl;
            saveManager_.autosave(*gameState_);
        }
    }
};

#endif // SAVE_WIRING_H
