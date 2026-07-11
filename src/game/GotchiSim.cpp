#include "GotchiSim.hpp"
#include "EventBus.h"
#include "GotchiStats.hpp"
#include "GotchiMood.hpp"
#include "GameConstants.hpp"

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

void GotchiSim::tickVitals(float dt) {
    // Apply every frame instead of batching into GOTCHI_TICK_RATE-sized
    // jumps -- tick()'s per-second math is unchanged (still divided by the
    // same GOTCHI_TICK_RATE), only how often it's applied changes, so bars
    // creep smoothly frame-to-frame instead of sitting still for 10s then
    // lurching. dt itself is untouched, so this can't reintroduce the
    // dt-spike-during-scene-load bug from shrinking GOTCHI_TICK_RATE.
    float ticks = dt / GOTCHI_TICK_RATE;
    state_.vitals.tick(ticks, state_.sleeping, state_.onHexboard);
}

// Separate method for mood updates (runs every frame, outside vitals tick gate)
void GotchiSim::updateMood(float dt) {
    // Mood updates must run every frame for overlay timing to work correctly
    state_.mood.update(dt, state_.vitals);
}

void GotchiSim::update(float dt) {
    // Skip vitals tick when in Story mode (simulator is paused), while the
    // tutorial/collapse gate has frozen stats, or once the gotchi has died --
    // collapsed is a terminal game-over state, nothing should keep ticking
    // underneath the death animation/hold/DeathScene transition.
    if (state_.mode != Mode::Story && !state_.statsFrozen && !state_.collapsed) {
        // 1. Sleep metronome: always drains, including before the first-ever
        // merge -- it's what forces/unlocks that first merge via the
        // sleep-collapse gate (see GotchiScene::applySleepCollapseGate()).
        // seenReality only affects the button's label/branch bucket, not
        // whether the metronome itself is running.
        state_.sleep -= SLEEP_DRAIN_RATE * dt;
        if (state_.sleep < 0.0f) {
            state_.sleep = 0.0f;
        }

        // Tick vitals every GOTCHI_TICK_RATE seconds
        tickVitals(dt);

        // Update mood every frame (overlay timing needs precise dt)
        updateMood(dt);

        // 4. Grime: rises with neglect each frame (clamped <= 1)
        state_.grime += GRIME_CREEP_RATE * dt;
        if (state_.grime > 1.0f) {
            state_.grime = 1.0f;
        }

        // 5. Collapse / survival: death is always possible now (no more
        // story-climax gate) -- driven directly by the visible Health bar
        // (FITALITY) hitting 0, since that's the stat the player actually
        // watches drain. The old state_.needs struct is mostly dead (only
        // .affection is ever written elsewhere), so it's no longer checked.
        if (!state_.collapsed) {
            if (state_.vitals.getStat(SecondaryStat::FITALITY) <= 0.0f) {
                state_.collapsed = true;
                state_.drivers.survival = 0.0f;
                if (!collapsedEmitted_) {
                    bus_.emit(Event::petCollapsed());
                    collapsedEmitted_ = true;
                }
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
