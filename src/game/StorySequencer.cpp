#include "StorySequencer.hpp"
#include "EventBus.h"
#include "EventType.h"
#include "GameState.h"
#include "SceneManager.hpp"
#include "DialogBox.hpp"
#include "MergeScene.hpp"
#include <iostream>

// Debug logging flag — set to true to see event flow in console
// Clean up before merge (remove or gate behind #ifdef)
static const bool DEBUG_LOG = true;

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
    (void)dt; // MergeScene/DialogBox are driven by SceneManager/main.cpp's own update calls

    if (phase_ == Phase::MergeIn || phase_ == Phase::MergeOut) {
        Scene* mergeScene = scenes_.getScene("merge");
        MergeScene* merge = static_cast<MergeScene*>(mergeScene);
        if (!merge || !merge->isFinished()) return;

        if (phase_ == Phase::MergeIn) {
            // Merge-in transition done -- start the sequence's first step.
            startStep(0);
        } else {
            // Merge-out transition done -- whole sequence is over.
            finishSequence();
        }
        return;
    }

    if (phase_ == Phase::EnteringStep) {
        // Wait for SceneManager to actually flip currentScene and init() it
        // (happens at the transition's halfway point) before triggering the
        // scenario -- triggering any earlier hits an empty/stale events
        // table and silently no-ops.
        if (scenes_.isTransitioning()) return;

        const SequenceStep& step = sequence_[activeStepIndex_];
        Scene* scene = scenes_.getScene(step.sceneName);
        if (scene) scene->triggerStoryEvent(step.scenarioId);
        phase_ = Phase::PlayingStep;
        return;
    }

    if (phase_ == Phase::PlayingStep) {
        const SequenceStep& step = sequence_[activeStepIndex_];
        Scene* scene = scenes_.getScene(step.sceneName);
        // isPlayingEvent() is the scenario-level "still going" signal (true
        // from triggerStoryEvent() until the scene ends its own event), NOT
        // DialogBox's per-line onFinished -- that fires the instant each
        // individual line finishes typing out and would end the step after
        // just its first line instead of its last.
        if (scene && !scene->isPlayingEvent()) {
            onStepFinished();
        }
    }
}

void StorySequencer::onStoryBeatStarted(const Event& e) {
    (void)e; // sequence content now comes from ScenarioDirector, not the event payload

    // Guard: ignore if already mid-sequence (shouldn't happen per MergeController lock)
    if (sequenceInProgress_) {
        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] Ignoring StoryBeatStarted: sequence already in progress" << std::endl;
        }
        return;
    }

    startSequence();
}

void StorySequencer::startSequence() {
    sequence_ = ScenarioDirector::SelectSequence(state_);
    sequenceInProgress_ = true;
    activeStepIndex_ = -1;

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Starting sequence with " << sequence_.size() << " step(s)" << std::endl;
    }

    if (sequence_.empty()) {
        // Nothing to play -- treat as an immediately-completed sequence
        // rather than leaving MergeController waiting forever.
        finishSequence();
        return;
    }

    startMergeTransition(Phase::MergeIn);
}

// visualMode is which MergeScene::Mode actually plays -- MergeScene's own
// MERGE_IN/MERGE_OUT names read backwards from the player's perspective
// (confirmed by eye), so this is the ONLY place that mapping lives. `phase`
// is this sequencer's own position-in-sequence state (start vs end) and
// must stay independent of which visual mode gets played, or update()'s
// MergeIn-vs-MergeOut branch (start first step vs finish sequence) breaks.
void StorySequencer::startMergeTransition(Phase phase) {
    phase_ = phase;
    MergeScene::Mode visualMode = (phase == Phase::MergeIn) ? MergeScene::Mode::MERGE_OUT : MergeScene::Mode::MERGE_IN;

    scenes_.switchScene("merge", TransitionEffect::FADE, 0.3f);
    Scene* mergeScene = scenes_.getScene("merge");
    MergeScene* merge = static_cast<MergeScene*>(mergeScene);
    if (merge) merge->startMerge(visualMode);

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Playing merge transition, phase=" << (int)phase
                  << ", visualMode=" << (int)visualMode << std::endl;
    }
}

void StorySequencer::startStep(int stepIndex) {
    activeStepIndex_ = stepIndex;
    phase_ = Phase::EnteringStep;

    const SequenceStep& step = sequence_[stepIndex];

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Starting step " << stepIndex
                  << ": scene=" << step.sceneName
                  << ", scenarioId=" << step.scenarioId << std::endl;
    }

    // triggerStoryEvent() is NOT called here -- SceneManager only flips
    // currentScene and calls the new scene's init() at the transition's
    // halfway point, so the scene's events table would still be empty/stale
    // this frame. update()'s Phase::EnteringStep branch waits for the
    // transition to finish before actually triggering the scenario.
    scenes_.switchScene(step.sceneName, TransitionEffect::FADE, 0.5f);

    if (!scenes_.getScene(step.sceneName)) {
        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] No scene registered for '" << step.sceneName << "'!" << std::endl;
        }
    }
}

void StorySequencer::onStepFinished() {
    const SequenceStep& step = sequence_[activeStepIndex_];

    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Step " << activeStepIndex_ << " finished" << std::endl;
    }

    if (step.isClimax) {
        if (DEBUG_LOG) {
            std::cout << "[StorySequencer] Climax step detected, setting deathEnabled=true" << std::endl;
        }
        state_.deathEnabled = true;

        // Emit DeathEnabled BEFORE StoryBeatCompleted so MergeController sees it
        Event deathEvent = Event::deathEnabled();
        bus_.emit(deathEvent);
    }

    int nextIndex = activeStepIndex_ + 1;
    if (nextIndex < (int)sequence_.size()) {
        startStep(nextIndex);
    } else {
        startMergeTransition(Phase::MergeOut);
    }
}

void StorySequencer::finishSequence() {
    if (DEBUG_LOG) {
        std::cout << "[StorySequencer] Sequence completed" << std::endl;
    }

    phase_ = Phase::Idle;
    activeStepIndex_ = -1;
    sequenceInProgress_ = false;
    sequence_.clear();

    Event completed = Event::storyBeatCompleted(0);
    bus_.emit(completed);
}
