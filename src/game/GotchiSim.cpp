#include "GotchiSim.hpp"
#include "EventBus.h"

GotchiSim::GotchiSim(EventBus& bus, GameState& state)
    : bus_(bus), state_(state) {
    // Subscribe to events
    careActionToken_ = bus_.subscribe(EventType::CareAction, [this](const Event& e) {
        this->onCareAction(e);
    });
    mergeCompletedToken_ = bus_.subscribe(EventType::MergeCompleted, [this](const Event& e) {
        this->onMergeCompleted(e);
    });
}

GotchiSim::~GotchiSim() {
    // Unsubscribe from events
    if (careActionToken_ != 0) {
        bus_.unsubscribe(careActionToken_);
    }
    if (mergeCompletedToken_ != 0) {
        bus_.unsubscribe(mergeCompletedToken_);
    }
}

void GotchiSim::update(float dt) {
    // 1. Sleep metronome: drain while seenReality is true and mode != Story
    if (state_.seenReality && state_.mode != Mode::Story) {
        state_.sleep -= SLEEP_DRAIN_RATE * dt;
        if (state_.sleep < 0.0f) {
            state_.sleep = 0.0f;
        }
    }

    // 4. Grime: rises with neglect each frame (clamped <= 1)
    state_.grime += GRIME_CREEP_RATE * dt;
    if (state_.grime > 1.0f) {
        state_.grime = 1.0f;
    }

    // 5. Collapse / survival: only while deathEnabled is true
    if (state_.deathEnabled && !state_.collapsed) {
        // Check if any visible need is at/under COLLAPSE_NEED_THRESHOLD
        if (state_.needs.hunger <= COLLAPSE_NEED_THRESHOLD ||
            state_.needs.hygiene <= COLLAPSE_NEED_THRESHOLD ||
            state_.needs.affection <= COLLAPSE_NEED_THRESHOLD ||
            state_.needs.energy <= COLLAPSE_NEED_THRESHOLD) {
            state_.collapsed = true;
            state_.drivers.survival = 0.0f;
            if (!collapsedEmitted_) {
                bus_.emit(Event::petCollapsed());
                collapsedEmitted_ = true;
            }
        }
    }
}

void GotchiSim::onCareAction(const Event& e) {
    // e.ia = actionType, e.fa = magnitude
    int actionType = e.ia;
    float magnitude = e.fa;

    // 2. Affection driver: warmth actions (pet/play/ball) only
    if (isWarmthAction(actionType)) {
        state_.drivers.affection += AFFECTION_GAIN * magnitude;
        if (state_.drivers.affection > 1.0f) {
            state_.drivers.affection = 1.0f;
        }
    }

    // 4. Grime reduction: any care action reduces grime
    state_.grime -= GRIME_CARE_REDUCE * magnitude;
    if (state_.grime < 0.0f) {
        state_.grime = 0.0f;
    }
}

void GotchiSim::onMergeCompleted(const Event& e) {
    // 3. Mercy driver: weighted by earliness
    float mercyGain = MERCY_GAIN;

    if (state_.mergeCount == 0) {
        // First merge: weight by firstMergeBucket
        switch (state_.firstMergeBucket) {
            case FirstMerge::Immediate:
                mercyGain *= 1.0f;   // Immediate = most mercy
                break;
            case FirstMerge::AfterABit:
                mercyGain *= 0.6f;   // AfterABit = mid mercy
                break;
            case FirstMerge::AfterNeglect:
                mercyGain *= 0.3f;   // AfterNeglect = least mercy
                break;
            default:
                mercyGain *= 0.5f;   // NotYet = mid mercy
                break;
        }
    } else {
        // Later merges: weight by sleep at moment of merge
        // Higher sleep = merged earlier / gave him a break = more mercy
        // Near-zero sleep = grind = little mercy
        mercyGain *= state_.sleep;  // sleep is 0.0 to 1.0
    }

    state_.drivers.mercy += mercyGain;
    if (state_.drivers.mercy > 1.0f) {
        state_.drivers.mercy = 1.0f;
    }
}

bool GotchiSim::isWarmthAction(int actionType) const {
    // Warmth actions (affection-building): pet/ball only (code 1)
    // Feed (0), wash (2), groom (3), water (4) are NOT warmth actions
    // per design §5 - warmth is specifically for affection/driver
    return actionType == CARE_ACTION_PET;
}
