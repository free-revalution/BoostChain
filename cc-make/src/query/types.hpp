#pragma once

#include "core/types.hpp"
#include "api/types.hpp"
#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <atomic>
#include <variant>

namespace ccmake {

// ============================================================
// Query Configuration
// ============================================================

struct QueryConfig {
    std::string model;
    std::string system_prompt;
    std::string cwd;
    int64_t max_tokens = 8192;
    int max_turns = 10;
    int max_retries = 10;
    int timeout_ms = 600000;
    std::optional<APIThinkingConfig> thinking;
    std::vector<APIToolDefinition> tools;
};

// ============================================================
// Loop State (carried across iterations)
// ============================================================

enum class LoopExitReason {
    Completed,
    Aborted,
    MaxTurns,
    ModelError,
    PromptTooLong
};

struct LoopState {
    struct PendingToolUse {
        std::string id;
        std::string name;
        nlohmann::json input;
        int index = 0;
    };

    std::vector<Message> messages;
    int turn_count = 0;
    TokenBudget total_usage;
    std::optional<LoopExitReason> exit_reason;
    std::vector<PendingToolUse> pending_tool_use_blocks;

    bool needs_follow_up() const { return !pending_tool_use_blocks.empty(); }

    // Extract tool_use blocks from an assistant message into pending list
    void collect_tool_uses(const Message& msg) {
        for (const auto& block : msg.content) {
            if (auto* tu = std::get_if<ToolUseBlock>(&block)) {
                pending_tool_use_blocks.push_back({
                    tu->id, tu->name, tu->input,
                    static_cast<int>(pending_tool_use_blocks.size())
                });
            }
        }
    }
};

// ============================================================
// Event Callbacks (observer pattern for streaming)
// ============================================================

struct QueryEventAssistant {
    Message message;
};

struct QueryEventToolUse {
    std::string tool_id;
    std::string tool_name;
    nlohmann::json input;
};

struct QueryEventToolResult {
    std::string tool_id;
    std::string content;
    bool is_error;
};

struct QueryEventUsage {
    TokenBudget usage;
};

struct QueryEventError {
    std::string error_message;
    bool is_retryable;
};

struct QueryEventThinking {
    std::string thinking;
};

using QueryEvent = std::variant<
    QueryEventAssistant,
    QueryEventToolUse,
    QueryEventToolResult,
    QueryEventUsage,
    QueryEventError,
    QueryEventThinking
>;

using QueryEventCallback = std::function<void(const QueryEvent&)>;
using ToolExecuteFunction = std::function<nlohmann::json(const std::string& tool_name, const nlohmann::json& input)>;

// ============================================================
// Turn Result
// ============================================================

struct TurnResult {
    LoopExitReason exit_reason = LoopExitReason::Completed;
    std::vector<Message> messages;
    TokenBudget total_usage;
    std::string error_message;
};

// ============================================================
// Abort Signal
// ============================================================

class AbortSignal {
public:
    void abort() { aborted_ = true; }
    bool is_aborted() const { return aborted_.load(); }
    void reset() { aborted_ = false; }
private:
    std::atomic<bool> aborted_{false};
};

}  // namespace ccmake
