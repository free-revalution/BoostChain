#include <boostchain/agent.hpp>
#include <boostchain/error.hpp>
#include <stdexcept>

namespace boostchain {

Agent::Agent(std::shared_ptr<ILLMProvider> llm)
    : llm_(std::move(llm)) {
    if (!llm_) {
        throw ConfigError("Agent requires a non-null LLM provider");
    }
}

void Agent::set_system_prompt(const std::string& prompt) {
    system_prompt_ = prompt;
}

void Agent::reset() {
    conversation_history_.clear();
}

AgentResult Agent::run(const std::string& user_input) {
    AgentResult result;

    if (!llm_) {
        result.completed = false;
        result.steps_taken = 0;
        result.final_answer = "Error: LLM provider is null";
        return result;
    }

    // Build the message list for this turn
    std::vector<Message> messages;

    // Add system prompt if set
    if (!system_prompt_.empty()) {
        messages.push_back(Message(Message::system, system_prompt_));
    }

    // Add conversation history
    messages.insert(messages.end(),
                    conversation_history_.begin(),
                    conversation_history_.end());

    // Add the new user message
    Message user_msg(Message::user, user_input);
    messages.push_back(user_msg);

    try {
        ChatRequest request;
        request.model = llm_->get_model();
        request.messages = std::move(messages);

        ChatResponse response = llm_->chat(request);

        if (response.messages.empty()) {
            result.completed = false;
            result.steps_taken = 0;
            result.final_answer = "Error: LLM returned empty response";
            return result;
        }

        // Take the last assistant message as the answer
        const Message& assistant_msg = response.messages.back();
        result.final_answer = assistant_msg.content;
        result.steps_taken = 1;
        result.completed = true;

        // Update conversation history with this exchange
        conversation_history_.push_back(user_msg);
        conversation_history_.push_back(assistant_msg);

    } catch (const std::exception& e) {
        result.completed = false;
        result.steps_taken = 0;
        result.final_answer = std::string("Error: ") + e.what();
    }

    return result;
}

} // namespace boostchain
