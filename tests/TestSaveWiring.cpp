// Test program for SaveManager and SaveWiring
#include "engine/SaveManager.h"
#include "engine/GameState.h"
#include "events/EventBus.h"
#include "engine/SaveWiring.h"
#include "flags/Keys.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>
// Removed - not needed for assert-based tests

void initTestSaveDir() {
    // Use a test-specific directory
    std::string testDir = "./build/saves_test";
    // Clear any existing saves
    if (std::filesystem::exists(testDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(testDir)) {
            std::filesystem::remove(entry.path());
        }
    }
    SaveManager::setSaveDir(testDir);
}

void testStartupLoad() {
    std::cout << "Test: Startup load..." << std::endl;

    initTestSaveDir();

    // Create and save a GameState with specific values
    GameState state;
    state.storyBeatIndex = 3;
    state.mergeCount = 5;
    state.mode = Mode::Story;
    state.setFlag("test_flag", true);

    SaveManager sm;
    sm.save(0, state);
    sm.setActiveSlot(0);

    // Simulate startup load
    GameState loadedState;
    auto loadedOpt = sm.load(0);
    assert(loadedOpt.has_value());
    loadedState = loadedOpt.value();

    assert(loadedState.storyBeatIndex == 3);
    assert(loadedState.mergeCount == 5);
    assert(loadedState.mode == Mode::Story);
    assert(loadedState.getBool("test_flag", false) == true);

    std::cout << "  PASSED: Startup load works!" << std::endl;
}

void testSaveLoadCycle() {
    std::cout << "Test: Save/load cycle..." << std::endl;

    initTestSaveDir();

    SaveManager sm;
    sm.setActiveSlot(1);

    // Save initial state
    GameState state1;
    state1.storyBeatIndex = 1;
    state1.mergeCount = 2;
    state1.setFlag("phase", 1);
    assert(sm.save(1, state1));

    // Modify and save again
    GameState state2;
    state2.storyBeatIndex = 2;
    state2.mergeCount = 4;
    state2.setFlag("phase", 2);
    assert(sm.save(1, state2));

    // Load and verify latest state
    auto loaded = sm.load(1);
    assert(loaded.has_value());
    assert(loaded->storyBeatIndex == 2);
    assert(loaded->mergeCount == 4);
    assert(loaded->getInt("phase") == 2);

    std::cout << "  PASSED: Save/load cycle works!" << std::endl;
}

void testAutosave() {
    std::cout << "Test: Autosave..." << std::endl;

    initTestSaveDir();

    SaveManager sm;
    sm.setActiveSlot(2);

    GameState state;
    state.storyBeatIndex = 10;
    state.mergeCount = 20;
    sm.autosave(state);

    // Verify autosave wrote to active slot
    auto loaded = sm.load(2);
    assert(loaded.has_value());
    assert(loaded->storyBeatIndex == 10);
    assert(loaded->mergeCount == 20);

    std::cout << "  PASSED: Autosave works!" << std::endl;
}

void testSaveWiringEvents() {
    std::cout << "Test: SaveWiring event subscription..." << std::endl;

    initTestSaveDir();

    SaveManager sm;
    sm.setActiveSlot(0);

    EventBus bus;
    GameState state;
    state.storyBeatIndex = 5;
    state.mergeCount = 10;

    SaveWiring wiring(sm, bus);
    wiring.setGameState(&state);

    // Emit StoryBeatCompleted event - should trigger autosave
    Event completed = Event::storyBeatCompleted(3);
    bus.emit(completed);

    // Verify autosave was triggered and data was saved
    auto loaded = sm.load(0);
    assert(loaded.has_value());
    assert(loaded->storyBeatIndex == 5);
    assert(loaded->mergeCount == 10);

    // Now test MergeBackCompleted
    state.storyBeatIndex = 7;
    state.mergeCount = 15;
    Event backCompleted = Event::mergeBackCompleted();
    bus.emit(backCompleted);

    // Verify autosave was triggered again
    loaded = sm.load(0);
    assert(loaded.has_value());
    assert(loaded->storyBeatIndex == 7);
    assert(loaded->mergeCount == 15);

    std::cout << "  PASSED: SaveWiring event subscription works!" << std::endl;
}

void testSaveWiringSceneTransition() {
    std::cout << "Test: SaveWiring scene transition autosave..." << std::endl;

    initTestSaveDir();

    SaveManager sm;
    sm.setActiveSlot(1);

    GameState state;
    state.storyBeatIndex = 7;
    state.mergeCount = 15;
    state.setFlag("scene_test", "transition");

    EventBus bus;
    SaveWiring wiring(sm, bus);
    wiring.setGameState(&state);

    // Trigger autosave via scene transition method
    wiring.onSceneTransition();

    // Verify autosave wrote to active slot
    auto loaded = sm.load(1);
    assert(loaded.has_value());
    assert(loaded->storyBeatIndex == 7);
    assert(loaded->mergeCount == 15);
    assert(loaded->getStr("scene_test") == "transition");

    std::cout << "  PASSED: SaveWiring scene transition autosave works!" << std::endl;
}

int main() {
    std::cout << "=== SaveManager Test Suite ===" << std::endl;

    testStartupLoad();
    testSaveLoadCycle();
    testAutosave();
    testSaveWiringEvents();
    testSaveWiringSceneTransition();

    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}
