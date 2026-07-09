#ifndef EVENT_TYPE_H
#define EVENT_TYPE_H

#include <string>

// events/EventType.h — HUMAN-OWNED shared contract. Agents APPEND, do not rewrite.
//
// The bus is the seam that lets Tier-1 systems be built in parallel: they talk
// through events + the shared struct instead of calling into each other.

enum class EventType {
    ButtonPressed,        // payload: which button
    MergeRequested,       // player pressed merge (or forced)
    MergeCompleted,       // crossed into story
    MergeBackCompleted,   // returned to gotchi; button locks
    CareAction,           // feed / pet / wash / hex-item; payload: which + magnitude
    NeedCritical,         // payload: which need (drives beep intrusion)
    StoryBeatStarted,
    StoryBeatCompleted,
    ChoiceMade,           // payload: scene id + choice id
    DeathEnabled,         // climax flag flipped
    PetCollapsed,         // routes to ending 3
    StateChanged,         // optional: UI repaint hint
};

// Event payload structure
// Small payload; a tagged struct or a couple of ints/floats + a string is plenty.
struct Event {
    EventType type = EventType::StateChanged;
    // small payload; a tagged struct or a couple of ints/floats + a string is plenty.
    int   ia = 0, ib = 0;
    float fa = 0.f;
    std::string sa;

    // Convenience constructors for common event patterns
    Event() = default;

    Event(EventType t, int a = 0, int b = 0, float f = 0.f, std::string s = "")
        : type(t), ia(a), ib(b), fa(f), sa(std::move(s)) {}

    // Factory methods for common event types
    static Event buttonPressed(int buttonId) {
        return Event{EventType::ButtonPressed, buttonId};
    }

    static Event mergeRequested() {
        return Event{EventType::MergeRequested};
    }

    static Event mergeCompleted() {
        return Event{EventType::MergeCompleted};
    }

    static Event mergeBackCompleted() {
        return Event{EventType::MergeBackCompleted};
    }

    static Event careAction(int actionType, float magnitude) {
        return Event{EventType::CareAction, actionType, 0, magnitude};
    }

    static Event needCritical(int needType) {
        return Event{EventType::NeedCritical, needType};
    }

    static Event storyBeatStarted(int beatIndex) {
        return Event{EventType::StoryBeatStarted, beatIndex};
    }

    static Event storyBeatCompleted(int beatIndex) {
        return Event{EventType::StoryBeatCompleted, beatIndex};
    }

    static Event choiceMade(const std::string& sceneId, int choiceId) {
        return Event{EventType::ChoiceMade, choiceId, 0, 0.f, sceneId};
    }

    static Event deathEnabled() {
        return Event{EventType::DeathEnabled};
    }

    static Event petCollapsed() {
        return Event{EventType::PetCollapsed};
    }

    static Event stateChanged() {
        return Event{EventType::StateChanged};
    }
};

#endif // EVENT_TYPE_H
