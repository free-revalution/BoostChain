#pragma once

#include "core/types.hpp"
#include "config/config.hpp"

#include <string>
#include <vector>
#include <optional>
#include <atomic>
#include <mutex>
#include <functional>

namespace ccmake {

struct AppState {
    std::string model;
    std::string small_fast_model = "claude-3-5-haiku-20241022";
    APIProvider provider = APIProvider::FirstParty;
    std::string conversation_id;
    std::vector<Message> messages;
    Screen screen = Screen::Prompt;
    InputMode input_mode = InputMode::Prompt;
    bool is_loading = false;
    std::string streaming_text;
    PermissionMode permission_mode = PermissionMode::Default;
    TokenBudget token_budget;
    int turn_count = 0;
    Config config;
    std::optional<ProjectConfig> project_config;
    std::optional<ClaudeMdContent> claude_md;
    std::string session_id;
    std::string working_directory;
    std::atomic<bool> cancel_requested{false};
    std::atomic<bool> should_exit{false};
    bool in_plan_mode = false;
    std::string plan_content;
};

class AppStateStore {
public:
    AppState& state() { return state_; }
    const AppState& state() const { return state_; }

    template<typename Mutator>
    void update(Mutator&& mutator) {
        std::lock_guard<std::mutex> lock(mutex_);
        mutator(state_);
        notify_subscribers();
    }

    using Subscriber = std::function<void(const AppState&)>;
    int subscribe(Subscriber sub);
    void unsubscribe(int id);

private:
    void notify_subscribers();
    AppState state_;
    std::mutex mutex_;
    std::vector<std::pair<int, Subscriber>> subscribers_;
    int next_sub_id_ = 0;
};

AppStateStore& global_store();

}  // namespace ccmake
