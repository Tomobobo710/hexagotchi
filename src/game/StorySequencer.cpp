#include "StorySequencer.hpp"
#include "EventBus.h"
#include "EventType.h"
#include "GameState.h"
#include "SceneManager.hpp"
#include "DialogBox.hpp"
#include <iostream>

// Debug logging flag — set to true to see event flow in console
// Clean up before merge (remove or gate behind #ifdef)
static const bool DEBUG_LOG = true;

// The authored story order — edit this list to change story sequence
// Scene names must match those registered in main.cpp.
//
// Beat 0 is special: its dialog selection is keyed to firstMergeBucket
// (Immediate / AfterABit / AfterNeglect) in GameState.
//
// The last beat should have isClimax = true, which:
//   1. Sets state.deathEnabled = true
//   2. Emits DeathEnabled before StoryBeatCompleted
//   3. Causes MergeController to hold in story mode (ending box takes over)
static const std::vector<StoryBeat> BEATS = {
    // Beat 0: Opening scene — variant selected by firstMergeBucket
    { "pizza_parlor", 0, false },

    // Beat 1: Apartment scene (Tom alone, morning)
    { "apartment", 0, false },

    // Beat 2: Therapist office (session)
    { "therapist_office", 0, false },

    // Beat 3: Office scene (performance review)
    { "office", 0, false },

    // Beat 4: School pickup (Karen needling)
    { "school", 0, false },

    // Beat 5: THE CLIMAX — sets deathEnabled
    { "boss", 0, true },
};

StorySequencer::StorySequencer(EventBus& bus, GameState& state, SceneManager& scenes, DialogBox& dialog)
    : bus_(bus), state_(state), scenes_(scenes), dialog_(dialog) {

    // Subscribe to StoryBeatStarted
    bus_.subscribe(EventType::StoryBeatStarted, [this](const Event& e) {
        this->onStoryBeatStarted(e);
    });
}

StorySequencer::~StorySequencer() {
    // Unsubscribe (token not stored; EventBus doesn't expose unsub by type)
    // Since this is single-threaded and destruction happens at shutdown,
    // we rely on EventBus not calling destroyed objects.
}

void StorySequencer::update(float dt) {
    // Event-driven; nothing to do here
    (void)dt;
}

void StorySequencer::onStoryBeatStarted(const Event& e) {
    int beatIndex = e.ia;

    // Guard: ignore if already in a beat (shouldn't happen per MergeController lock)
    if (beatInProgress_) {
        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] Ignoring StoryBeatStarted{" << beatIndex
                      << "}: beat already in progress" << std::endl;
        }
        return;
    }

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] StoryBeatStarted{" << beatIndex << "}" << std::endl;
    }

    // Check bounds: if index is past the end of our beats, do nothing
    if (beatIndex >= (int)BEATS.size()) {
        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] Beat index " << beatIndex
                      << " >= BEATS.size() (" << BEATS.size()
                      << "), ignoring (end of content)" << std::endl;
        }
        return;
    }

    startBeat(beatIndex);
}

void StorySequencer::startBeat(int beatIndex) {
    const StoryBeat& beat = BEATS[beatIndex];

    beatInProgress_ = true;
    activeBeatIndex_ = beatIndex;

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Starting beat " << beatIndex
                  << ": scene=" << beat.sceneName
                  << ", eventIndex=" << beat.eventIndex << std::endl;
    }

    // Switch to the story scene with a door/step-through transition
    // Use SLIDE_RIGHT as a "stepping through a door" effect (design §6)
    scenes_.switchScene(beat.sceneName, TransitionEffect::SLIDE_RIGHT, 0.5f);

    // Play the beat's dialog
    playBeatDialog(beat);
}

void StorySequencer::playBeatDialog(const StoryBeat& beat) {
    // Get the current scene (should be the one we just switched to)
    Scene* scene = scenes_.getCurrentScene();
    if (!scene) {
        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] No current scene found!" << std::endl;
        }
        return;
    }

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Scene switched, calling triggerStoryEvent(" << beat.eventIndex << ")" << std::endl;
    }

    // Call triggerStoryEvent on the scene
    scene->triggerStoryEvent(beat.eventIndex);

    // Set up dialog completion callback
    dialog_.setOnFinished([this]() {
        this->onDialogFinished();
    });

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Dialog callback set, waiting for completion..." << std::endl;
    }
}

void StorySequencer::onDialogFinished() {
    if (activeBeatIndex_ < 0) {
        return;
    }

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Dialog finished for beat " << activeBeatIndex_ << std::endl;
    }

    const StoryBeat& beat = BEATS[activeBeatIndex_];

    // If this is the climax beat, set deathEnabled and emit DeathEnabled
    if (beat.isClimax) {
        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] Climax beat detected, setting deathEnabled=true" << std::endl;
        }

        state_.deathEnabled = true;

        // Emit DeathEnabled BEFORE StoryBeatCompleted so MergeController sees it
        Event deathEvent = Event::deathEnabled();
        bus_.emit(deathEvent);

        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] Emitted DeathEnabled event" << std::endl;
        }
    }

    // Emit StoryBeatCompleted
    emitBeatCompleted(activeBeatIndex_);

    // Reset state
    activeBeatIndex_ = -1;
    beatInProgress_ = false;

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Beat completed" << std::endl;
    }
}

void StorySequencer::emitBeatCompleted(int beatIndex) {
    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Emitting StoryBeatCompleted{" << beatIndex << "}" << std::endl;
    }

    Event completed = Event::storyBeatCompleted(beatIndex);
    bus_.emit(completed);
}
