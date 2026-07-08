#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#include "EventType.h"
#include <functional>
#include <unordered_map>
#include <vector>
#include <algorithm>
#include <mutex>

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    // Subscribe to an event type, returns a token for unsubscribing
    int subscribe(EventType t, Handler h);

    // Unsubscribe using the token returned by subscribe
    void unsubscribe(int token);

    // Emit an event - SYNCHRONOUS dispatch
    void emit(const Event& e);

private:
    // Handler storage: event type -> list of (token, handler) pairs
    std::unordered_map<EventType, std::vector<std::pair<int, Handler>>> handlers_;
    int nextToken_ = 1;  // Start at 1 so 0 is always invalid
};

#endif // EVENT_BUS_H
