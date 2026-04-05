#pragma once

#include "query/types.hpp"
#include "query/tool_executor.hpp"

#include <string>
#include <vector>
#include <functional>

namespace ccmake {

struct AgenticLoopConfig {
    int max_turns = 10;
    AbortSignal* abort_signal = nullptr;
    std::string system_prompt;
    std::vector<APIToolDefinition> tools;
    ToolExecutor* tool_executor = nullptr;
    std::string model;
    int64_t max_tokens = 8192;
    std::optional<APIThinkingConfig> thinking;
    QueryEventCallback on_event;
};

// API call function type: takes messages, system prompt, tools, event callback -> returns Message
using APICallFunction = std::function<Message(
    const std::vector<Message>& messages,
    const std::string& system_prompt,
    const std::vector<APIToolDefinition>& tools,
    QueryEventCallback on_event,
    const std::vector<APIToolDefinition>& api_tools
)>;

// Core agentic while-true loop
TurnResult run_agentic_loop(
    APICallFunction api_call,
    const AgenticLoopConfig& config,
    const Message& user_message
);

}  // namespace ccmake
