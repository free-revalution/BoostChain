#pragma once

#include <boostchain/llm_provider.hpp>
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

    /// Run the agent on a user input. The agent maintains conversation
    /// history across multiple calls, so successive runs continue the
    /// conversation.
    AgentResult run(const std::string& user_input);

    /// Set the system prompt used for every conversation turn.
    void set_system_prompt(const std::string& prompt);

    /// Reset the conversation history.
    void reset();

private:
    std::shared_ptr<ILLMProvider> llm_;
    std::string system_prompt_;
    std::vector<Message> conversation_history_;
};

} // namespace boostchain
