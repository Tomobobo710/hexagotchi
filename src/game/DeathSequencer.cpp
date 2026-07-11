#include "DeathSequencer.hpp"
#include "EventBus.h"
#include "EventType.h"
#include "SceneManager.hpp"
#include "MergeScene.hpp"

DeathSequencer::DeathSequencer(EventBus& bus, GameState& state, SceneManager& scenes)
    : bus_(bus), state_(state), scenes_(scenes) {
    (void)state_;
    bus_.subscribe(EventType::PetCollapsed, [this](const Event& e) {
        this->onPetCollapsed(e);
    });
}

void DeathSequencer::onPetCollapsed(const Event& e) {
    (void)e;
    if (phase_ != Phase::Idle) return;  // PetCollapsed only ever fires once anyway
    phase_ = Phase::Holding;
    holdTimer_ = DEATH_HOLD_SECONDS;
}

void DeathSequencer::update(float dt) {
    if (phase_ == Phase::Holding) {
        holdTimer_ -= dt;
        if (holdTimer_ <= 0.0f) {
            phase_ = Phase::Merging;
            scenes_.switchScene("merge");
            MergeScene* merge = static_cast<MergeScene*>(scenes_.getScene("merge"));
            if (merge) merge->startMerge(MergeScene::Mode::MERGE_OUT);
        }
        return;
    }

    if (phase_ == Phase::Merging) {
        MergeScene* merge = static_cast<MergeScene*>(scenes_.getScene("merge"));
        if (merge && merge->isFinished()) {
            phase_ = Phase::Idle;
            scenes_.switchScene("death");
        }
    }
}
