#ifndef STORY_SEQUENCER_HPP
#define STORY_SEQUENCER_HPP

// StorySequencer.hpp — Box D: story sequencer
// Glues StoryBeatStarted events to actual story scene playback.
// HUMAN-OWNED: The beat table is the single source of story order.
// Agents: implement the event flow, do not rewrite the design.

#include <vector>
#include <string>

// Forward declarations
class EventBus;
class GameState;
class SceneManager;
class DialogBox;
class Scene;
struct Event;

// One story beat — what to play, where, and how to finish.
// AUTHORED ORDER — the single place the story sequence lives.
// Reorder beats here to change story flow; never hardcode "beat 3 = office" in logic.
struct StoryBeat {
    std::string sceneName;      // Registered scene name (from main.cpp)
    int         eventIndex = 0; // Event index to trigger in that scene
    bool        isClimax = false;  // Last beat: sets deathEnabled and emits DeathEnabled
};

class StorySequencer {
public:
    // Constructor wires up the sequencer to the event bus
    // Takes non-owning pointers to all collaborators
    StorySequencer(EventBus& bus, GameState& state, SceneManager& scenes, DialogBox& dialog);

    // Destructor: unsubscribes from event bus
    ~StorySequencer();

    // Update is called each frame (but sequencer is event-driven mostly)
    void update(float dt);

    // Public for tests to simulate dialog completion
    // Call this when the active beat's dialog finishes playing
    void onDialogFinished();

private:
    // Event handlers
    void onStoryBeatStarted(const Event& e);

    // Internal helpers
    void startBeat(int beatIndex);
    void emitBeatCompleted(int beatIndex);
    void playBeatDialog(const StoryBeat& beat);

    // State
    EventBus&    bus_;
    GameState&   state_;
    SceneManager& scenes_;
    DialogBox&   dialog_;

    // Runtime state
    int activeBeatIndex_ = -1;      // Currently playing beat, or -1 if idle
    bool beatInProgress_ = false;   // Guard against overlapping beats
};

#endif // STORY_SEQUENCER_HPP
