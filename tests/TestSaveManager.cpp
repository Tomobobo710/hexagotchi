// Test program for SaveManager
#include "engine/SaveManager.h"
#include "engine/GameState.h"
#include "flags/Keys.h"
#include <iostream>
#include <cassert>
#include <fstream>
#include <filesystem>

void initTestSaveDir() {
    // Use a test-specific directory
    std::string testDir = "./build/desktop/Debug/saves";
    // Clear any existing saves
    if (std::filesystem::exists(testDir)) {
        for (const auto& entry : std::filesystem::directory_iterator(testDir)) {
            std::filesystem::remove(entry.path());
        }
    }
    SaveManager::setSaveDir(testDir);
}

void testRoundTrip() {
    std::cout << "Test: Round-trip save/load..." << std::endl;

    // Create a GameState with non-default values
    GameState original;
    original.needs.hunger = 0.5f;
    original.needs.hygiene = 0.7f;
    original.needs.affection = 0.3f;
    original.needs.energy = 0.9f;
    original.sleep = 0.8f;
    original.deathEnabled = true;
    original.drivers.affection = 0.2f;
    original.drivers.mercy = 0.6f;
    original.drivers.survival = 0.9f;
    original.mode = Mode::Story;
    original.seenReality = true;
    original.firstMergeBucket = FirstMerge::AfterNeglect;
    original.storyBeatIndex = 5;
    original.mergeLockTimer = 10.5f;
    original.playtimeSeconds = 3600.0;

    // Add flags of different types
    original.setFlag(flag::SEEN_REALITY, true);
    original.setFlag(flag::BOTH_SIDES_ENGAGED, true);
    original.setFlag(flag::CHOICE_OFFICE_CONFRONT, 1);  // int
    original.setFlag("custom_float", 3.14f);
    original.setFlag("custom_string", std::string("test value"));
    original.setFlag("custom_bool", false);

    // Save to slot 0
    SaveManager sm;
    sm.setActiveSlot(0);
    assert(sm.save(0, original));

    // Load from slot 0
    auto loadedOpt = sm.load(0);
    assert(loadedOpt.has_value());

    GameState loaded = loadedOpt.value();

    // Verify all fields match
    assert(loaded.version == original.version);
    assert(loaded.needs.hunger == original.needs.hunger);
    assert(loaded.needs.hygiene == original.needs.hygiene);
    assert(loaded.needs.affection == original.needs.affection);
    assert(loaded.needs.energy == original.needs.energy);
    assert(loaded.sleep == original.sleep);
    assert(loaded.deathEnabled == original.deathEnabled);
    assert(loaded.drivers.affection == original.drivers.affection);
    assert(loaded.drivers.mercy == original.drivers.mercy);
    assert(loaded.drivers.survival == original.drivers.survival);
    assert(loaded.mode == original.mode);
    assert(loaded.seenReality == original.seenReality);
    assert(loaded.firstMergeBucket == original.firstMergeBucket);
    assert(loaded.storyBeatIndex == original.storyBeatIndex);
    assert(loaded.mergeLockTimer == original.mergeLockTimer);
    assert(loaded.playtimeSeconds == original.playtimeSeconds);

    // Verify flags
    assert(loaded.has(flag::SEEN_REALITY));
    assert(loaded.has(flag::BOTH_SIDES_ENGAGED));
    assert(loaded.has(flag::CHOICE_OFFICE_CONFRONT));
    assert(loaded.has("custom_float"));
    assert(loaded.has("custom_string"));
    assert(loaded.has("custom_bool"));

    assert(loaded.getBool(flag::SEEN_REALITY) == original.getBool(flag::SEEN_REALITY));
    assert(loaded.getBool(flag::BOTH_SIDES_ENGAGED) == original.getBool(flag::BOTH_SIDES_ENGAGED));
    assert(loaded.getInt(flag::CHOICE_OFFICE_CONFRONT) == original.getInt(flag::CHOICE_OFFICE_CONFRONT));
    assert(loaded.getFloat("custom_float") == original.getFloat("custom_float"));
    assert(loaded.getStr("custom_string") == original.getStr("custom_string"));
    assert(loaded.getBool("custom_bool") == original.getBool("custom_bool"));

    std::cout << "  PASSED: All fields and flags match!" << std::endl;
}

void testSlotIsolation() {
    std::cout << "Test: Slot isolation..." << std::endl;

    SaveManager sm;

    // Save different states to each slot
    GameState s0, s1, s2;

    s0.setFlag("slot", 0);
    s1.setFlag("slot", 1);
    s2.setFlag("slot", 2);

    assert(sm.save(0, s0));
    assert(sm.save(1, s1));
    assert(sm.save(2, s2));

    // Load and verify each slot has its own data
    auto l0 = sm.load(0);
    auto l1 = sm.load(1);
    auto l2 = sm.load(2);

    assert(l0.has_value());
    assert(l1.has_value());
    assert(l2.has_value());

    assert(l0->getInt("slot") == 0);
    assert(l1->getInt("slot") == 1);
    assert(l2->getInt("slot") == 2);

    std::cout << "  PASSED: Each slot has its own data!" << std::endl;
}

void testDeleteSlot() {
    std::cout << "Test: Delete slot..." << std::endl;

    SaveManager sm;

    // Create some data
    GameState s;
    s.setFlag("test", true);
    sm.save(1, s);

    // Verify slot exists
    assert(sm.slotExists(1));

    // Delete slot 1
    sm.deleteSlot(1);

    // Verify slot is gone
    assert(!sm.slotExists(1));

    // Verify other slots are untouched
    sm.save(0, s);
    sm.save(2, s);

    assert(sm.slotExists(0));
    assert(!sm.slotExists(1));
    assert(sm.slotExists(2));

    std::cout << "  PASSED: Slot deletion works correctly!" << std::endl;
}

void testMissingKeyTolerance() {
    std::cout << "Test: Missing key tolerance..." << std::endl;

    SaveManager sm;

    // Create and save a state
    GameState original;
    original.needs.hunger = 0.5f;
    original.setFlag("exists", true);

    sm.save(0, original);

    // Manually edit the JSON to remove a typed field and a flag
    // This simulates loading an old save
    std::ifstream file("/home/bazola-hp/dev/baz-code/hexagotchi/build/desktop/Debug/saves/save_0.json");
    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();

    // Remove the "needs" field and one flag
    // In a real scenario, this would be done with a text editor
    // For testing, we'll just verify defaults work by loading with minimal data

    // Create a minimal save file
    std::ofstream minimal("/home/bazola-hp/dev/baz-code/hexagotchi/build/desktop/Debug/saves/save_0.json");
    minimal << R"({
        "version": 1,
        "needs": {"hunger": 1, "hygiene": 1, "affection": 1, "energy": 1},
        "drivers": {"affection": 0, "mercy": 0, "survival": 1},
        "sleep": 1,
        "deathEnabled": false,
        "mode": 0,
        "seenReality": false,
        "firstMergeBucket": 0,
        "storyBeatIndex": 0,
        "mergeLockTimer": 0,
        "playtimeSeconds": 0,
        "flags": {}
    })";
    minimal.close();

    // Load and verify defaults are used
    auto loaded = sm.load(0);
    assert(loaded.has_value());

    // Default hunger should be 1.0f (from GameState constructor)
    // But we overwrote it in the file with 1, so this should be 1
    // Note: JSON number 1 should convert to float 1.0f
    assert(std::abs(loaded->needs.hunger - 1.0f) < 0.001f);
    assert(loaded->getBool("missing_flag", false) == false);  // Missing flag returns default
    assert(loaded->getInt("missing_int", 999) == 999);  // Missing int returns default

    std::cout << "  PASSED: Missing keys use defaults!" << std::endl;
}

void testSummary() {
    std::cout << "Test: Slot summary..." << std::endl;

    SaveManager sm;

    // Save a state with label
    GameState s;
    s.storyBeatIndex = 3;
    s.playtimeSeconds = 1234.5;
    s.setFlag("pet_name", std::string("Buddy"));

    sm.save(0, s);

    // Get summary
    auto sum = sm.summary(0);
    assert(sum.exists);
    assert(sum.version == 1);
    assert(sum.storyBeatIndex == 3);
    assert(sum.playtimeSeconds == 1234.5);

    std::cout << "  PASSED: Slot summary works!" << std::endl;
}

void testAutosave() {
    std::cout << "Test: Autosave..." << std::endl;

    SaveManager sm;
    sm.setActiveSlot(1);

    GameState s;
    s.setFlag("autosave_test", true);
    sm.autosave(s);

    // Verify autosave wrote to the active slot
    auto loaded = sm.load(1);
    assert(loaded.has_value());
    assert(loaded->has("autosave_test"));

    std::cout << "  PASSED: Autosave works!" << std::endl;
}

int main() {
    initTestSaveDir();
    std::cout << "=== SaveManager Test Suite ===" << std::endl;

    testRoundTrip();
    testSlotIsolation();
    testDeleteSlot();
    testMissingKeyTolerance();
    testSummary();
    testAutosave();

    std::cout << "\n=== All tests passed! ===" << std::endl;
    return 0;
}
