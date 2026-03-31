#pragma once

#include <boostchain/llm_provider.hpp>
#include <boostchain/tool.hpp>
#include <string>
#include <vector>
#include <memory>

namespace boostchain {

struct AgentResult {
    std::string final_answer;
    int steps_taken;
    bool completed;
};

class Agent {
public:
    Agent(std::shared_ptr<ILLMProvider> llm);

    /// Construct an agent with tools available for use.
    Agent(std::shared_ptr<ILLMProvider> llm,
          std::vector<std::shared_ptr<ITool>> tools);

    /// Run the agent on a user input. The agent maintains conversation
    /// history across multiple calls, so successive runs continue the
    /// conversation.
    AgentResult run(const std::string& user_input);

    /// Set the system prompt used for every conversation turn.
    void set_system_prompt(const std::string& prompt);

    /// Reset the conversation history.
    void reset();

    /// Set the maximum number of tool-use iterations before the agent
    /// stops and returns whatever it has.
    void set_max_iterations(int max_iterations);

    // ----- Persistence (Task 15) -----

    /// Serialize the conversation history to a JSON string.
    std::string save_state() const;

    /// Restore conversation history from a JSON string.
    /// Returns true on success, false on parse error.
    bool load_state(const std::string& json_data);

private:
    /// Build the message list for an LLM request.
    std::vector<Message> build_messages(const std::string& user_input) const;

    /// Execute a single tool call and return the result as a Message.
    Message execute_tool_call(const ToolCall& tool_call);

    /// Run one LLM step, possibly executing tool calls in a loop.
    AgentResult run_with_tools(const std::string& user_input);

private:
    std::shared_ptr<ILLMProvider> llm_;
    std::string system_prompt_;
    std::vector<Message> conversation_history_;
    std::vector<std::shared_ptr<ITool>> tools_;
    int max_iterations_ = 10;
};

} // namespace boostchain
