// Test program for GotchiSim
#include "game/GotchiSim.hpp"
#include "events/EventBus.h"
#include "engine/GameState.h"
#include "events/EventType.h"

// GotchiSim test - no source files included, linked via CMake
#include <iostream>
#include <cassert>
#include <cmath>

void testSleepDrains() {
    std::cout << "T1 sleep_drains: ";

    GameState state;
    state.seenReality = true;
    state.mode = Mode::Gotchi;
    state.sleep = 1.0f;

    EventBus bus;
    GotchiSim sim(bus, state);

    float dt = 0.1f;  // 100ms per frame
    int frames = 500; // 50 seconds total

    float startSleep = state.sleep;
    for (int i = 0; i < frames; i++) {
        sim.update(dt);
    }
    float endSleep = state.sleep;

    assert(endSleep < startSleep && "Sleep should have decreased");
    assert(endSleep <= 0.001f && "Sleep should have reached 0");

    std::cout << "PASS sleep_drains (start=" << startSleep << ", end=" << endSleep << ")" << std::endl;
}

void testSleepNoDrainPreLock() {
    std::cout << "T2 sleep_no_drain_prelock: ";

    GameState state;
    state.seenReality = false;  // seenReality is false
    state.mode = Mode::Gotchi;
    state.sleep = 1.0f;

    EventBus bus;
    GotchiSim sim(bus, state);

    float dt = 0.1f;
    for (int i = 0; i < 100; i++) {
        sim.update(dt);
    }

    // Sleep should remain at 1.0 since seenReality is false
    assert(std::abs(state.sleep - 1.0f) < 0.001f && "Sleep should not drain when seenReality=false");

    std::cout << "PASS sleep_no_drain_prelock (sleep=" << state.sleep << ")" << std::endl;
}

void testSleepPausesInStory() {
    std::cout << "T3 sleep_pauses_in_story: ";

    GameState state;
    state.seenReality = true;
    state.mode = Mode::Story;  // Story mode
    state.sleep = 1.0f;

    EventBus bus;
    GotchiSim sim(bus, state);

    float dt = 0.1f;
    for (int i = 0; i < 100; i++) {
        sim.update(dt);
    }

    // Sleep should remain at 1.0 since mode is Story
    assert(std::abs(state.sleep - 1.0f) < 0.001f && "Sleep should not drain in Story mode");

    std::cout << "PASS sleep_pauses_in_story (sleep=" << state.sleep << ")" << std::endl;
}

void testSleepDrainOnly() {
    std::cout << "T4 sleep_drain_only: ";

    GameState state;
    state.seenReality = true;
    state.mode = Mode::Gotchi;
    state.sleep = 0.5f;  // Start at 0.5

    EventBus bus;
    GotchiSim sim(bus, state);

    float dt = 0.01f;  // Small steps
    float prevSleep = state.sleep;
    for (int i = 0; i < 100; i++) {
        sim.update(dt);
        assert(state.sleep <= prevSleep + 0.0001f && "Sleep should never increase");
        prevSleep = state.sleep;
    }

    std::cout << "PASS sleep_drain_only (sleep went from 0.5 to " << state.sleep << ")" << std::endl;
}

void testAffectionWarmthOnly() {
    std::cout << "T5 affection_warmth_only: ";

    GameState state;
    state.seenReality = true;
    state.mode = Mode::Gotchi;
    state.drivers.affection = 0.0f;

    EventBus bus;
    GotchiSim sim(bus, state);

    // Test warmth action (pet) - should increase affection
    // CARE_ACTION_PET = 1 (from GotchiSim.hpp)
    Event petEvent = Event::careAction(1, 1.0f);
    bus.emit(petEvent);

    float affectionAfterPet = state.drivers.affection;

    // Test feed action - should NOT increase affection
    // CARE_ACTION_FEED = 0 (from GotchiSim.hpp)
    state.drivers.affection = 0.0f;
    Event feedEvent = Event::careAction(0, 1.0f);
    bus.emit(feedEvent);

    float affectionAfterFeed = state.drivers.affection;

    assert(affectionAfterPet > 0.04f && "Pet should increase affection");
    assert(affectionAfterFeed < 0.02f && "Feed should NOT increase affection");

    std::cout << "PASS affection_warmth_only (pet=" << affectionAfterPet << ", feed=" << affectionAfterFeed << ")" << std::endl;
}

void testMercyEarliness() {
    std::cout << "T6 mercy_earliness: ";

    // Test 1: First merge with Immediate bucket
    {
        GameState state;
        state.mergeCount = 0;
        state.firstMergeBucket = FirstMerge::Immediate;
        state.drivers.mercy = 0.0f;

        EventBus bus;
        GotchiSim sim(bus, state);

        // Emit MergeCompleted
        bus.emit(Event::mergeCompleted());

        float mercyImmediate = state.drivers.mercy;
        std::cout << "  First merge (Immediate): mercy=" << mercyImmediate << " ";

        assert(mercyImmediate > 0.09f && mercyImmediate <= 1.0f && "Immediate first merge should give high mercy");
    }

    // Test 2: First merge with AfterNeglect bucket
    {
        GameState state;
        state.mergeCount = 0;
        state.firstMergeBucket = FirstMerge::AfterNeglect;
        state.drivers.mercy = 0.0f;

        EventBus bus;
        GotchiSim sim(bus, state);

        bus.emit(Event::mergeCompleted());

        float mercyNeglect = state.drivers.mercy;
        std::cout << "First merge (AfterNeglect): mercy=" << mercyNeglect << " ";

        // AfterNeglect should give less mercy than Immediate
        assert(mercyNeglect < 0.06f && mercyNeglect > 0.01f && "AfterNeglect first merge should give low mercy");
    }

    // Test 3: Later merge at high sleep
    {
        GameState state;
        state.mergeCount = 2;  // Not first merge
        state.sleep = 1.0f;    // High sleep = gave him a break
        state.drivers.mercy = 0.0f;

        EventBus bus;
        GotchiSim sim(bus, state);

        bus.emit(Event::mergeCompleted());

        float mercyHighSleep = state.drivers.mercy;
        std::cout << "Later merge (sleep=1.0): mercy=" << mercyHighSleep << " ";

        assert(mercyHighSleep > 0.09f && "Later merge at high sleep should give good mercy");
    }

    // Test 4: Later merge at low sleep
    {
        GameState state;
        state.mergeCount = 2;  // Not first merge
        state.sleep = 0.1f;    // Low sleep = grind
        state.drivers.mercy = 0.0f;

        EventBus bus;
        GotchiSim sim(bus, state);

        bus.emit(Event::mergeCompleted());

        float mercyLowSleep = state.drivers.mercy;
        std::cout << "Later merge (sleep=0.1): mercy=" << mercyLowSleep << std::endl;

        // Low sleep should give much less mercy
        assert(mercyLowSleep < 0.02f && mercyLowSleep > 0.001f && "Later merge at low sleep should give little mercy");
    }

    std::cout << "PASS mercy_earliness" << std::endl;
}

void testGrimeCreepAndCare() {
    std::cout << "T7 grime_creep_and_care: ";

    GameState state;
    state.grime = 0.0f;

    EventBus bus;
    GotchiSim sim(bus, state);

    float dt = 0.1f;

    // Simulate neglect - grime should creep up
    for (int i = 0; i < 50; i++) {  // 5 seconds
        sim.update(dt);
    }

    float grimeAfterNeglect = state.grime;
    std::cout << "  After 5s neglect: grime=" << grimeAfterNeglect << " ";

    assert(grimeAfterNeglect > 0.04f && "Grime should have increased");

    // Now emit a care action - grime should decrease
    // CARE_ACTION_WASH = 2 (from GotchiSim.hpp)
    Event washEvent = Event::careAction(2, 1.0f);
    bus.emit(washEvent);

    float grimeAfterCare = state.grime;
    std::cout << "After wash: grime=" << grimeAfterCare << " ";

    // Grime reduction is 0.08, and we had 0.05 grime after 5s
    // So grime becomes 0 (clamped) after care
    assert(grimeAfterCare < grimeAfterNeglect && "Care should reduce grime");
    assert(grimeAfterCare >= 0.0f && "Grime should never go negative");

    // Verify grime stays in [0, 1] range
    for (int i = 0; i < 200; i++) {  // 20 seconds
        sim.update(dt);
    }
    assert(state.grime <= 1.01f && "Grime should not exceed 1.0 much");

    std::cout << "PASS grime_creep_and_care" << std::endl;
}

void testCollapseGated() {
    std::cout << "T8 collapse_gated: ";

    // Test 1: deathEnabled=false - should NOT collapse even with needs at 0
    {
        GameState state;
        state.deathEnabled = false;
        state.collapsed = false;
        state.needs.hunger = 0.0f;   // At threshold
        state.needs.hygiene = 0.0f;
        state.needs.affection = 0.0f;
        state.needs.energy = 0.0f;
        state.drivers.survival = 1.0f;

        EventBus bus;
        GotchiSim sim(bus, state);

        bool collapsedEmitted = false;
        int token = bus.subscribe(EventType::PetCollapsed, [&collapsedEmitted](const Event&) {
            collapsedEmitted = true;
        });

        sim.update(0.1f);

        assert(!state.collapsed && "Should NOT collapse when deathEnabled=false");
        assert(state.drivers.survival == 1.0f && "Survival should remain 1.0");
        assert(!collapsedEmitted && "PetCollapsed should NOT be emitted");

        bus.unsubscribe(token);
    }

    // Test 2: deathEnabled=true - should collapse with needs at 0
    {
        GameState state;
        state.deathEnabled = true;
        state.collapsed = false;
        state.needs.hunger = 0.0f;   // At threshold
        state.needs.hygiene = 0.0f;
        state.needs.affection = 0.0f;
        state.needs.energy = 0.0f;
        state.drivers.survival = 1.0f;

        EventBus bus;
        GotchiSim sim(bus, state);

        bool collapsedEmitted = false;
        int token = bus.subscribe(EventType::PetCollapsed, [&collapsedEmitted](const Event&) {
            collapsedEmitted = true;
        });

        sim.update(0.1f);

        assert(state.collapsed && "Should collapse when deathEnabled=true with needs at threshold");
        assert(state.drivers.survival == 0.0f && "Survival should be set to 0");
        assert(collapsedEmitted && "PetCollapsed should be emitted");

        bus.unsubscribe(token);
    }

    std::cout << "PASS collapse_gated" << std::endl;
}

int main() {
    std::cout << "=== GotchiSim Unit Tests ===" << std::endl;
    std::cout << std::endl;

    testSleepDrains();
    testSleepNoDrainPreLock();
    testSleepPausesInStory();
    testSleepDrainOnly();
    testAffectionWarmthOnly();
    testMercyEarliness();
    testGrimeCreepAndCare();
    testCollapseGated();

    std::cout << std::endl;
    std::cout << "=== All tests passed! ===" << std::endl;

    return 0;
}
