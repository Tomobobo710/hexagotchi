#include "DriversController.hpp"
#include "EventType.h"
#include <iostream>

// Debug logging flag
static const bool DEBUG_LOG = false;

DriversController::DriversController(EventBus& bus, GameState& state)
    : bus_(bus), state_(state) {

    // Subscribe to CareAction events
    careActionToken_ = bus_.subscribe(EventType::CareAction, [this](const Event& e) {
        this->onCareAction(e);
    });

    if (DEBUG_LOG) {
        std::cout << "[DriversController] Subscribed to CareAction events" << std::endl;
    }
}

DriversController::~DriversController() {
    if (careActionToken_ > 0) {
        bus_.unsubscribe(careActionToken_);
    }
}

void DriversController::update(float dt) {
    // Grime creep: only after seenReality and not in Story mode
    // This matches the sleep gating logic
    if (state_.seenReality && state_.mode != Mode::Story) {
        state_.grime += GRIME_CREEP_RATE * dt;
        // Clamp grime to [0, 1]
        if (state_.grime > 1.0f) {
            state_.grime = 1.0f;
        }
    }

    // Decay affection in needs due to time passing (neglect)
    // Only when in Gotchi mode (player should be actively caring)
    // Note: needs does NOT have mercy - that's only in drivers
    if (state_.mode == Mode::Gotchi) {
        float affectionDrain = 0.005f * dt;  // slow decay

        state_.needs.affection -= affectionDrain;

        // Clamp to [0, 1]
        if (state_.needs.affection < 0.0f) state_.needs.affection = 0.0f;
    }
}

void DriversController::onCareAction(const Event& e) {
    int actionCode = e.ia;
    float magnitude = e.fa;

    if (DEBUG_LOG) {
        std::cout << "[DriversController] CareAction code=" << actionCode
                  << " magnitude=" << magnitude << std::endl;
    }

    // Track accumulators for debug/readout
    if (isWarmthAction(actionCode)) {
        affectionAccum_ += magnitude;
    }
    if (isHygieneAction(actionCode)) {
        hygieneAccum_ += magnitude;
    }

    // Warmth actions (pet only) drive affection in GameState.needs
    // This activates C-core's collapse logic
    if (isWarmthAction(actionCode)) {
        // Affection in needs represents "how much care the gotchi has received"
        // Starts at 1.0 and decreases with neglect (time between care actions)
        // Each warmth action restores it partially
        float gain = AFFECTION_GAIN * magnitude;
        state_.needs.affection += gain;
        // Clamp to [0, 1]
        if (state_.needs.affection > 1.0f) {
            state_.needs.affection = 1.0f;
        }

        if (DEBUG_LOG) {
            std::cout << "[DriversController] Warmth action! needs.affection=" << state_.needs.affection << std::endl;
        }
    }

    // Hygiene actions (wash/groom) drive mercy in GameState.drivers
    if (isHygieneAction(actionCode)) {
        // Mercy in drivers represents "how merciful the player has been"
        // Starts at 0.0 and grows with mercy actions
        // This is used for ending determination (Box E)
        float gain = MERCY_GAIN * magnitude;
        state_.drivers.mercy += gain;
        // Clamp to [0, 1]
        if (state_.drivers.mercy > 1.0f) {
            state_.drivers.mercy = 1.0f;
        }

        if (DEBUG_LOG) {
            std::cout << "[DriversController] Hygiene action! drivers.mercy=" << state_.drivers.mercy << std::endl;
        }
    }
}
