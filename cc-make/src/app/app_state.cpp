#include "app/app_state.hpp"

namespace ccmake {

void AppStateStore::notify_subscribers() {
    for (const auto& [id, sub] : subscribers_) {
        sub(state_);
    }
}

int AppStateStore::subscribe(Subscriber sub) {
    std::lock_guard<std::mutex> lock(mutex_);
    int id = next_sub_id_++;
    subscribers_.emplace_back(id, std::move(sub));
    return id;
}

void AppStateStore::unsubscribe(int id) {
    std::lock_guard<std::mutex> lock(mutex_);
    subscribers_.erase(
        std::remove_if(subscribers_.begin(), subscribers_.end(),
            [id](const auto& pair) { return pair.first == id; }),
        subscribers_.end()
    );
}

AppStateStore& global_store() {
    static AppStateStore store;
    return store;
}

}  // namespace ccmake
