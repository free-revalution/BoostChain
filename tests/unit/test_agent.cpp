#include <boostchain/agent.hpp>
#include <boostchain/llm_provider.hpp>
#include <boostchain/error.hpp>
#include <boostchain/result.hpp>
#include <cassert>
#include <string>
#include <vector>
#include <optional>
#include <map>

using namespace boostchain;

// A mock LLM provider that returns a canned response for testing.
class MockLLMProvider : public ILLMProvider {
public:
    // Track calls
    int chat_call_count = 0;
    std::string last_model;
    std::vector<Message> last_messages;

    // Configurable response
    std::string canned_response = "Mock response";
    bool should_throw = false;

    ChatResponse chat(const ChatRequest& req) override {
        chat_call_count++;
        last_model = req.model;
        last_messages = req.messages;

        if (should_throw) {
            throw NetworkError("Simulated network failure", 500);
        }

        ChatResponse resp;
        resp.model = req.model;
        resp.messages.push_back(Message(Message::assistant, canned_response));
        resp.finish_reason = "stop";
        return resp;
    }

    Result<ChatResponse> chat_safe(const ChatRequest& req) override {
        try {
            return Result<ChatResponse>::ok(chat(req));
        } catch (...) {
            return Result<ChatResponse>::error("mock error");
        }
    }

    std::future<ChatResponse> chat_async(const ChatRequest&) override {
        return std::async(std::launch::deferred, [&]() { return ChatResponse{}; });
    }

    void chat_stream(const ChatRequest&, StreamCallback) override {}

    EmbeddingResponse embed(const EmbeddingRequest&) override {
        return EmbeddingResponse{};
    }

    void set_api_key(const std::string&) override {}
    void set_model(const std::string& m) override { model_ = m; }
    void set_base_url(const std::string&) override {}
    void set_timeout(int) override {}

    std::string get_api_key() const override { return ""; }
    std::string get_model() const override { return model_; }
    std::string get_base_url() const override { return ""; }
    int get_timeout() const override { return 0; }

private:
    std::string model_ = "mock-model";
};

// ============================================================================
// Tests
// ============================================================================

void test_agent_construction() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);
    // Agent was constructed without throwing - success
}

void test_agent_basic_run() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);

    AgentResult result = agent.run("Hello");

    assert(result.completed == true);
    assert(result.steps_taken == 1);
    assert(result.final_answer == "Mock response");
    assert(llm->chat_call_count == 1);
}

void test_agent_system_prompt() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);
    agent.set_system_prompt("You are a test assistant.");

    agent.run("Hello");

    // The first message sent to the LLM should be the system prompt
    assert(!llm->last_messages.empty());
    assert(llm->last_messages[0].role == Message::system);
    assert(llm->last_messages[0].content == "You are a test assistant.");

    // The second message should be the user message
    assert(llm->last_messages.size() >= 2);
    assert(llm->last_messages[1].role == Message::user);
    assert(llm->last_messages[1].content == "Hello");
}

void test_agent_conversation_history() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);

    agent.run("First message");
    assert(llm->chat_call_count == 1);
    int msgs_after_first = static_cast<int>(llm->last_messages.size());

    agent.run("Second message");
    assert(llm->chat_call_count == 2);

    // The second call should include the history (user + assistant from first call)
    // plus the new user message
    assert(static_cast<int>(llm->last_messages.size()) == msgs_after_first + 2);

    // The message before the last should be the assistant's first response
    assert(llm->last_messages[llm->last_messages.size() - 2].role == Message::assistant);
    assert(llm->last_messages[llm->last_messages.size() - 2].content == "Mock response");
}

void test_agent_reset() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);

    agent.run("First message");
    assert(llm->chat_call_count == 1);

    agent.reset();
    agent.run("Second message");
    assert(llm->chat_call_count == 2);

    // After reset, the second call should only have 1 user message (no history)
    int expected = 1; // just the user message
    assert(static_cast<int>(llm->last_messages.size()) == expected);
}

void test_agent_llm_error() {
    auto llm = std::make_shared<MockLLMProvider>();
    llm->should_throw = true;
    Agent agent(llm);

    AgentResult result = agent.run("Hello");

    assert(result.completed == false);
    assert(result.steps_taken == 0);
    assert(result.final_answer.find("Error:") != std::string::npos);
}

void test_agent_empty_response() {
    // Test with a provider that returns empty messages
    class EmptyMockProvider : public MockLLMProvider {
    public:
        ChatResponse chat(const ChatRequest& req) override {
            MockLLMProvider::chat(req); // for call count
            ChatResponse resp;
            resp.model = req.model;
            // No messages added
            return resp;
        }
    };

    auto llm = std::make_shared<EmptyMockProvider>();
    Agent agent(llm);

    AgentResult result = agent.run("Hello");

    assert(result.completed == false);
    assert(result.steps_taken == 0);
    assert(result.final_answer.find("Error:") != std::string::npos);
}

void test_agent_result_struct() {
    AgentResult result;
    result.final_answer = "test";
    result.steps_taken = 5;
    result.completed = true;

    assert(result.final_answer == "test");
    assert(result.steps_taken == 5);
    assert(result.completed == true);
}

int main() {
    test_agent_construction();
    test_agent_basic_run();
    test_agent_system_prompt();
    test_agent_conversation_history();
    test_agent_reset();
    test_agent_llm_error();
    test_agent_empty_response();
    test_agent_result_struct();
    return 0;
}
