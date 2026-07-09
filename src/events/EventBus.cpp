#include "EventBus.h"
#include <algorithm>

int EventBus::subscribe(EventType t, Handler h) {
    int token = nextToken_++;
    handlers_[t].emplace_back(token, std::move(h));
    return token;
}

void EventBus::unsubscribe(int token) {
    for (auto& pair : handlers_) {
        auto& vec = pair.second;
        auto it = std::find_if(vec.begin(), vec.end(),
            [token](const std::pair<int, Handler>& p) { return p.first == token; });
        if (it != vec.end()) {
            vec.erase(it);
            // If this was the last handler for this event type, clean up the map entry
            if (vec.empty()) {
                handlers_.erase(pair.first);
            }
            return;
        }
    }
}

void EventBus::emit(const Event& e) {
    auto it = handlers_.find(e.type);
    if (it != handlers_.end()) {
        // Copy handlers to avoid issues if handlers modify subscription during emit
        auto handlersCopy = it->second;
        for (auto& pair : handlersCopy) {
            pair.second(e);
        }
    }
}
