#pragma once

#include "query/types.hpp"
#include "query/tool_executor.hpp"
#include "tools/registry.hpp"
#include "api/types.hpp"

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace ccmake {

class QueryEngine {
public:
    explicit QueryEngine(const std::string& model = "claude-sonnet-4-20250514");

    // Configuration
    std::string model() const;
    void set_model(const std::string& model);
    std::string system_prompt() const;
    void set_system_prompt(const std::string& prompt);
    void set_max_turns(int max_turns);
    void set_max_tokens(int64_t max_tokens);
    void set_thinking(const std::optional<APIThinkingConfig>& thinking);

    // Tool registration (via function)
    void register_tool(const std::string& name, ToolExecuteFunction fn);
    bool has_tool(const std::string& name) const;

    // Tool registration (via ToolBase subclass)
    bool register_tool(std::unique_ptr<ToolBase> tool);
    ToolRegistry& tool_registry();
    const ToolRegistry& tool_registry() const;

    // Main entry point: submit a user message and run the agentic loop
    TurnResult submit_message(const std::string& prompt);

    // Access message history
    const std::vector<Message>& messages() const;

    // Abort current query
    void interrupt();

    // Mock support for testing
    void set_mock_response(const Message& response);
    void set_mock_responses(const std::vector<Message>& responses);

    void set_cwd(const std::string& cwd);

private:
    std::string model_;
    std::string system_prompt_;
    std::string cwd_;
    int max_turns_ = 10;
    int64_t max_tokens_ = 8192;
    std::optional<APIThinkingConfig> thinking_;

    ToolExecutor tool_executor_;
    ToolRegistry tool_registry_;
    std::vector<Message> messages_;

    AbortSignal abort_signal_;
    bool use_mock_ = false;
    std::vector<Message> mock_responses_;
    size_t mock_index_ = 0;

    Message next_mock_response();
};

}  // namespace ccmake
