// Test program for ExploreGate predicate
#include "game/ExploreGate.h"

#include <iostream>
#include <cassert>

void testConfidenceHighExcitementLow() {
    std::cout << "T1 confidence_high_excitement_low: ";
    // confidence >= 0.60, excitement <= 0.40 -> true
    assert(shouldShowExplore(0.70f, 0.30f) && "Should show Explore when confidence high + excitement low");
    std::cout << "PASS confidence_high_excitement_low" << std::endl;
}

void testConfidenceHighExcitementHigh() {
    std::cout << "T2 confidence_high_excitement_high: ";
    // confidence >= 0.60, excitement > 0.40 -> false
    assert(!shouldShowExplore(0.70f, 0.50f) && "Should NOT show Explore when confidence high + excitement high");
    assert(!shouldShowExplore(0.70f, 0.90f) && "Should NOT show Explore when confidence high + excitement very high");
    std::cout << "PASS confidence_high_excitement_high" << std::endl;
}

void testConfidenceLowExcitementLow() {
    std::cout << "T3 confidence_low_excitement_low: ";
    // confidence < 0.60, excitement <= 0.40 -> false
    assert(!shouldShowExplore(0.40f, 0.30f) && "Should NOT show Explore when confidence low + excitement low");
    assert(!shouldShowExplore(0.10f, 0.00f) && "Should NOT show Explore when confidence very low + excitement low");
    std::cout << "PASS confidence_low_excitement_low" << std::endl;
}

void testConfidenceLowExcitementHigh() {
    std::cout << "T4 confidence_low_excitement_high: ";
    // confidence < 0.60, excitement > 0.40 -> false
    assert(!shouldShowExplore(0.30f, 0.70f) && "Should NOT show Explore when confidence low + excitement high");
    std::cout << "PASS confidence_low_excitement_high" << std::endl;
}

void testBoundaryValues() {
    std::cout << "T5 boundary_values: ";
    // At exactly the thresholds - boundary behavior
    // confidence = 0.60 (exactly at min), excitement = 0.40 (exactly at max) -> true
    assert(shouldShowExplore(0.60f, 0.40f) && "Should show Explore at exact thresholds");

    // Just below confidence threshold -> false
    assert(!shouldShowExplore(0.59f, 0.40f) && "Should NOT show Explore just below confidence threshold");

    // Just above excitement threshold -> false
    assert(!shouldShowExplore(0.60f, 0.41f) && "Should NOT show Explore just above excitement threshold");
    std::cout << "PASS boundary_values" << std::endl;
}

void testAllCombinations() {
    std::cout << "T6 all_combinations: ";
    // Test all 4 quadrants
    // Quadrant 1: high confidence, low excitement -> true
    assert(shouldShowExplore(0.80f, 0.20f) && "Q1: high conf, low exc -> true");

    // Quadrant 2: high confidence, high excitement -> false
    assert(!shouldShowExplore(0.80f, 0.80f) && "Q2: high conf, high exc -> false");

    // Quadrant 3: low confidence, low excitement -> false
    assert(!shouldShowExplore(0.20f, 0.20f) && "Q3: low conf, low exc -> false");

    // Quadrant 4: low confidence, high excitement -> false
    assert(!shouldShowExplore(0.20f, 0.80f) && "Q4: low conf, high exc -> false");
    std::cout << "PASS all_combinations" << std::endl;
}

int main() {
    std::cout << "=== ExploreGate Test Suite ===" << std::endl;
    std::cout << std::endl;

    std::cout << "Thresholds: confidence >= " << EXPLORE_CONFIDENCE_MIN
              << ", excitement <= " << EXPLORE_EXCITEMENT_MAX << std::endl;
    std::cout << std::endl;

    testConfidenceHighExcitementLow();
    testConfidenceHighExcitementHigh();
    testConfidenceLowExcitementLow();
    testConfidenceLowExcitementHigh();
    testBoundaryValues();
    testAllCombinations();

    std::cout << std::endl;
    std::cout << "=== All tests passed! ===" << std::endl;
    return 0;
}
