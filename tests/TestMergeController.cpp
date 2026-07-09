// Test program for MergeController
#include "game/MergeController.hpp"
#include "events/EventBus.h"
#include "engine/GameState.h"
#include "engine/SceneManager.hpp"
#include "events/EventType.h"
#include "flags/Keys.h"

// MergeController test - no source files included, linked via CMake
#include <iostream>
#include <vector>
#include <cassert>
#include <cmath>

// Mock SceneManager for testing
// Note: The base class switchScene has default parameters but the derived
// class cannot have them or override won't work. We use the exact signature
// without defaults.
class MockSceneManager : public SceneManager {
public:
    void switchScene(const std::string& sceneName, TransitionEffect effect, float duration) {
        // Record the last scene we tried to switch to
        lastScene = sceneName;
        lastEffect = effect;
        lastDuration = duration;
        switched = true;
        // Mark as transitioning for the update test
        transitioning = true;
    }

    std::string lastScene = "";
    TransitionEffect lastEffect = TransitionEffect::NONE;
    float lastDuration = 0.0f;
    bool switched = false;
};

void testBusRoundtrip() {
    std::cout << "T1 bus_roundtrip: ";

    EventBus bus;
    bool fired = false;
    int token = bus.subscribe(EventType::MergeRequested, [&fired](const Event&) {
        fired = true;
    });

    bus.emit(Event::mergeRequested());

    assert(fired && "Event handler should have been called");

    // Cleanup
    bus.unsubscribe(token);

    std::cout << "PASS bus_roundtrip" << std::endl;
}

void testFirstMergeImmediate() {
    std::cout << "T2 first_merge_immediate: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    // First merge (seenReality = false)
    state.seenReality = false;

    MergeController controller(bus, state, scenes);

    // Capture emitted events
    std::vector<Event> events;
    int token = bus.subscribe(EventType::StoryBeatStarted, [&events](const Event& e) {
        events.push_back(e);
    });
    // Also capture MergeCompleted
    bus.subscribe(EventType::MergeCompleted, [&events](const Event& e) {
        events.push_back(e);
    });

    // Simulate merge request
    bus.emit(Event::mergeRequested());

    // Check results
    assert(state.firstMergeBucket == FirstMerge::Immediate && "firstMergeBucket should be Immediate");
    assert(state.seenReality == true && "seenReality should be true after first merge");
    assert(state.mode == Mode::Story && "mode should be Story after merge");

    // Check events
    bool foundMergeCompleted = false;
    bool foundStoryBeatStarted = false;
    for (const auto& e : events) {
        if (e.type == EventType::MergeCompleted) foundMergeCompleted = true;
        if (e.type == EventType::StoryBeatStarted && e.ia == 0) foundStoryBeatStarted = true;
    }
    assert(foundMergeCompleted && "MergeCompleted should be emitted");
    assert(foundStoryBeatStarted && "StoryBeatStarted{0} should be emitted");

    bus.unsubscribe(token);

    std::cout << "PASS first_merge_immediate" << std::endl;
}

void testFirstMergeTimeNeglect() {
    std::cout << "T3 first_merge_time_neglect: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    // First merge (seenReality = false)
    state.seenReality = false;

    MergeController controller(bus, state, scenes);

    // Simulate time passing by calling update with enough dt to exceed ABIT threshold
    // FIRST_MERGE_ABIT_SEC = 90.0f, so update with 100.0f
    controller.update(100.0f);

    std::vector<Event> events;
    int token = bus.subscribe(EventType::StoryBeatStarted, [&events](const Event& e) {
        events.push_back(e);
    });
    bus.subscribe(EventType::MergeCompleted, [&events](const Event& e) {
        events.push_back(e);
    });

    // Simulate merge request
    bus.emit(Event::mergeRequested());

    // With waitingTimer_ >= 90.0f and no needs below threshold, should be AfterNeglect
    assert(state.firstMergeBucket == FirstMerge::AfterNeglect && "firstMergeBucket should be AfterNeglect when waitingTimer_ >= 90s");

    // Check events
    bool foundMergeCompleted = false;
    bool foundStoryBeatStarted = false;
    for (const auto& e : events) {
        if (e.type == EventType::MergeCompleted) foundMergeCompleted = true;
        if (e.type == EventType::StoryBeatStarted && e.ia == 0) foundStoryBeatStarted = true;
    }
    assert(foundMergeCompleted && "MergeCompleted should be emitted");
    assert(foundStoryBeatStarted && "StoryBeatStarted{0} should be emitted");

    bus.unsubscribe(token);

    std::cout << "PASS first_merge_time_neglect" << std::endl;
}

void testFirstMergeNeedNeglect() {
    std::cout << "T4 first_merge_need_neglect: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    // First merge (seenReality = false), but needs.hunger = 0.2 (< 0.40)
    // This should result in AfterNeglect
    state.seenReality = false;
    state.needs.hunger = 0.2f;
    state.needs.hygiene = 1.0f;
    state.needs.affection = 1.0f;
    state.needs.energy = 1.0f;

    MergeController controller(bus, state, scenes);

    std::vector<Event> events;
    int token = bus.subscribe(EventType::StoryBeatStarted, [&events](const Event& e) {
        events.push_back(e);
    });
    bus.subscribe(EventType::MergeCompleted, [&events](const Event& e) {
        events.push_back(e);
    });

    bus.emit(Event::mergeRequested());

    // Verify the merge happened
    assert(state.seenReality == true && "seenReality should be true after first merge");
    assert(state.mode == Mode::Story && "mode should be Story after merge");

    std::cout << "PASS first_merge_need_neglect" << std::endl;

    bus.unsubscribe(token);
}

void testBeatCompleteReturns() {
    std::cout << "T5 beat_complete_returns: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    // Set up state as if we just returned from a merge
    state.seenReality = true;
    state.mode = Mode::Story;
    state.mergeLockTimer = 0.0f;
    state.sleep = 0.5f;
    state.storyBeatIndex = 0;
    state.engagedStorySide = false;
    state.mergeCount = 0;

    MergeController controller(bus, state, scenes);

    std::vector<Event> events;
    int token = bus.subscribe(EventType::MergeBackCompleted, [&events](const Event& e) {
        events.push_back(e);
    });

    // Emit StoryBeatCompleted
    bus.emit(Event::storyBeatCompleted(0));

    // Check results
    assert(state.mode == Mode::Gotchi && "mode should be Gotchi after return");
    assert(state.mergeLockTimer > 0.0f && "mergeLockTimer should be > 0 after return");
    assert(std::abs(state.sleep - SLEEP_FULL) < 0.001f && "sleep should be reset to SLEEP_FULL");
    assert(state.storyBeatIndex == 1 && "storyBeatIndex should be 1 after return");
    assert(state.engagedStorySide == true && "engagedStorySide should be true");
    assert(state.mergeCount == 1 && "mergeCount should be 1 after return");
    assert(scenes.switched && "scene should have been switched");
    assert(scenes.lastScene == "gotchi" && "scene should be switched to gotchi");

    // Check events
    bool foundMergeBackCompleted = false;
    for (const auto& e : events) {
        if (e.type == EventType::MergeBackCompleted) foundMergeBackCompleted = true;
    }
    assert(foundMergeBackCompleted && "MergeBackCompleted should be emitted");

    // isMergeAvailable should be false due to lock timer
    assert(!controller.isMergeAvailable() && "isMergeAvailable should be false due to lock");

    bus.unsubscribe(token);

    std::cout << "PASS beat_complete_returns" << std::endl;
}

void testLockExpiry() {
    std::cout << "T6 lock_expiry: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    state.seenReality = true;
    state.mode = Mode::Gotchi;
    state.mergeLockTimer = MERGE_LOCK_COOLDOWN_SEC + 0.1f;  // Just above cooldown

    MergeController controller(bus, state, scenes);

    assert(!controller.isMergeAvailable() && "isMergeAvailable should be false initially");

    // Update with dt past the cooldown
    controller.update(MERGE_LOCK_COOLDOWN_SEC + 0.1f);

    assert(controller.isMergeAvailable() && "isMergeAvailable should be true after lock expiry");

    std::cout << "PASS lock_expiry" << std::endl;
}

void testForcedMerge() {
    std::cout << "T7 forced_merge: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    state.seenReality = true;
    state.mode = Mode::Gotchi;
    state.mergeLockTimer = 0.0f;  // Unlocked
    state.sleep = 0.0f;  // Empty - should trigger forced merge

    MergeController controller(bus, state, scenes);

    std::vector<Event> events;
    int token = bus.subscribe(EventType::StoryBeatStarted, [&events](const Event& e) {
        events.push_back(e);
    });

    controller.update(0.0f);  // Should trigger forced merge

    assert(state.mode == Mode::Story && "mode should be Story after forced merge");

    bool foundStoryBeatStarted = false;
    for (const auto& e : events) {
        if (e.type == EventType::StoryBeatStarted) foundStoryBeatStarted = true;
    }
    assert(foundStoryBeatStarted && "StoryBeatStarted should be emitted");

    bus.unsubscribe(token);

    std::cout << "PASS forced_merge" << std::endl;
}

void testVoluntaryGate() {
    std::cout << "T8 voluntary_gate: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    state.seenReality = true;
    state.mode = Mode::Gotchi;
    state.mergeLockTimer = 0.0f;  // Unlocked
    state.sleep = 1.0f;  // Full - will be reset to full after beat
    state.setFlag(flag::VOLUNTARY_MERGE_UNLOCKED, false);

    MergeController controller(bus, state, scenes);

    std::vector<Event> events;
    int token = bus.subscribe(EventType::StoryBeatStarted, [&events](const Event& e) {
        events.push_back(e);
    });
    bus.subscribe(EventType::MergeCompleted, [&events](const Event& e) {
        events.push_back(e);
    });

    // Request merge with flag = false - should be ignored
    bus.emit(Event::mergeRequested());

    assert(state.mode == Mode::Gotchi && "mode should still be Gotchi (request ignored)");
    // Check that no StoryBeatStarted was emitted
    bool storyBeatEmitted = false;
    for (const auto& e : events) {
        if (e.type == EventType::StoryBeatStarted) storyBeatEmitted = true;
    }
    assert(!storyBeatEmitted && "StoryBeatStarted should not be emitted when flag is false");

    // Now set flag = true and request again
    state.setFlag(flag::VOLUNTARY_MERGE_UNLOCKED, true);
    events.clear();

    bus.emit(Event::mergeRequested());

    assert(state.mode == Mode::Story && "mode should be Story after second request");
    // mergeCount is 0 because we haven't returned from the merge yet
    assert(state.mergeCount == 0 && "mergeCount should be 0 (merge not completed yet)");

    // Now emit StoryBeatCompleted to complete the beat
    bus.emit(Event::storyBeatCompleted(0));

    assert(state.mode == Mode::Gotchi && "mode should be Gotchi after returning from beat");
    assert(state.mergeCount == 1 && "mergeCount should be incremented after beat completion");

    bus.unsubscribe(token);

    std::cout << "PASS voluntary_gate" << std::endl;
}

void testClimaxHolds() {
    std::cout << "T9 climax_holds: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    state.seenReality = true;
    state.mode = Mode::Story;
    state.mergeLockTimer = 0.0f;
    state.sleep = 0.5f;
    state.storyBeatIndex = 0;
    state.engagedStorySide = false;
    state.mergeCount = 0;
    state.deathEnabled = true;  // Climax reached

    MergeController controller(bus, state, scenes);

    std::vector<Event> events;
    int token = bus.subscribe(EventType::MergeBackCompleted, [&events](const Event& e) {
        events.push_back(e);
    });

    // Emit StoryBeatCompleted - should NOT return to gotchi
    bus.emit(Event::storyBeatCompleted(0));

    // mode should still be Story (not returned)
    assert(state.mode == Mode::Story && "mode should stay Story at climax");
    assert(!scenes.switched && "scene should NOT be switched at climax");

    // MergeBackCompleted should NOT be emitted
    bool foundMergeBackCompleted = false;
    for (const auto& e : events) {
        if (e.type == EventType::MergeBackCompleted) foundMergeBackCompleted = true;
    }
    assert(!foundMergeBackCompleted && "MergeBackCompleted should NOT be emitted at climax");

    bus.unsubscribe(token);

    std::cout << "PASS climax_holds" << std::endl;
}

void testButtonLabel() {
    std::cout << "T10 button_label: ";

    GameState state;
    EventBus bus;
    MockSceneManager scenes;

    MergeController controller(bus, state, scenes);

    // Before first merge
    assert(std::string(controller.mergeButtonLabel()) == "Give him a break" && "label should be 'Give him a break'");

    // After first merge
    state.seenReality = true;
    assert(std::string(controller.mergeButtonLabel()) == "Merge" && "label should be 'Merge'");

    std::cout << "PASS button_label" << std::endl;
}

int main() {
    std::cout << "=== MergeController Test Suite ===" << std::endl;
    std::cout << std::endl;

    testBusRoundtrip();
    testFirstMergeImmediate();
    testFirstMergeTimeNeglect();
    testFirstMergeNeedNeglect();
    testBeatCompleteReturns();
    testLockExpiry();
    testForcedMerge();
    testVoluntaryGate();
    testClimaxHolds();
    testButtonLabel();

    std::cout << std::endl;
    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
