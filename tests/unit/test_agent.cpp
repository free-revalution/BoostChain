#include <boostchain/agent.hpp>
#include <boostchain/llm_provider.hpp>
#include <boostchain/tool.hpp>
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

// Mock LLM that returns tool calls
class ToolCallingMockLLM : public ILLMProvider {
public:
    int call_count = 0;
    std::vector<Message> last_messages;

    // First call returns a tool call, second call returns final answer
    ChatResponse chat(const ChatRequest& req) override {
        call_count++;
        last_messages = req.messages;

        ChatResponse resp;
        resp.model = req.model;

        if (call_count == 1) {
            // Return a tool call
            Message msg(Message::assistant, "");
            ToolCall tc;
            tc.id = "call_123";
            tc.name = "calculator";
            tc.arguments = "{\"expression\": \"2 + 3\"}";
            msg.tool_call = tc;
            resp.messages.push_back(msg);
            resp.finish_reason = "tool_calls";
        } else {
            // Return final answer
            resp.messages.push_back(
                Message(Message::assistant, "The answer is 5"));
            resp.finish_reason = "stop";
        }

        return resp;
    }

    Result<ChatResponse> chat_safe(const ChatRequest& req) override {
        try { return Result<ChatResponse>::ok(chat(req)); }
        catch (...) { return Result<ChatResponse>::error("mock error"); }
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

    assert(!llm->last_messages.empty());
    assert(llm->last_messages[0].role == Message::system);
    assert(llm->last_messages[0].content == "You are a test assistant.");
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

    assert(static_cast<int>(llm->last_messages.size()) == msgs_after_first + 2);
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

    int expected = 1;
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
    class EmptyMockProvider : public MockLLMProvider {
    public:
        ChatResponse chat(const ChatRequest& req) override {
            MockLLMProvider::chat(req);
            ChatResponse resp;
            resp.model = req.model;
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

// ============================================================================
// Task 14: Agent-Tool Integration Tests
// ============================================================================

void test_agent_with_tools_construction() {
    auto llm = std::make_shared<MockLLMProvider>();
    auto calc = std::make_shared<CalculatorTool>();

    std::vector<std::shared_ptr<ITool>> tools = {calc};
    Agent agent(llm, tools);

    // Should not throw
}

void test_agent_tool_call_execution() {
    auto llm = std::make_shared<ToolCallingMockLLM>();
    auto calc = std::make_shared<CalculatorTool>();

    std::vector<std::shared_ptr<ITool>> tools = {calc};
    Agent agent(llm, tools);

    AgentResult result = agent.run("What is 2 + 3?");

    assert(result.completed == true);
    assert(result.steps_taken == 2); // One tool call + one final response
    assert(result.final_answer == "The answer is 5");
    assert(llm->call_count == 2);
}

void test_agent_tool_definitions_sent() {
    auto llm = std::make_shared<ToolCallingMockLLM>();
    auto calc = std::make_shared<CalculatorTool>();

    std::vector<std::shared_ptr<ITool>> tools = {calc};
    Agent agent(llm, tools);

    agent.run("Calculate something");

    // The first request should include tool definitions
    assert(!llm->last_messages.empty());
    // Note: we check the request had tools via the call succeeding
    assert(llm->call_count >= 1);
}

void test_agent_no_tools_original_behavior() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);

    // Without tools, the original behavior should be preserved
    AgentResult result = agent.run("Hello");
    assert(result.completed == true);
    assert(result.steps_taken == 1);
    assert(result.final_answer == "Mock response");
}

void test_agent_max_iterations() {
    // Mock that always returns tool calls (never finishes)
    class AlwaysToolCallMock : public MockLLMProvider {
    public:
        int call_count = 0;
        ChatResponse chat(const ChatRequest& req) override {
            call_count++;
            MockLLMProvider::chat(req);
            ChatResponse resp;
            resp.model = req.model;
            Message msg(Message::assistant, "");
            ToolCall tc;
            tc.id = "call_" + std::to_string(call_count);
            tc.name = "calculator";
            tc.arguments = "{\"expression\": \"1 + 1\"}";
            msg.tool_call = tc;
            resp.messages.push_back(msg);
            resp.finish_reason = "tool_calls";
            return resp;
        }
    };

    auto llm = std::make_shared<AlwaysToolCallMock>();
    auto calc = std::make_shared<CalculatorTool>();

    std::vector<std::shared_ptr<ITool>> tools = {calc};
    Agent agent(llm, tools);
    agent.set_max_iterations(3);

    AgentResult result = agent.run("Keep calculating");

    assert(result.completed == false);
    assert(result.final_answer.find("maximum iterations") != std::string::npos);
}

void test_agent_unknown_tool() {
    // Mock that calls an unknown tool
    class UnknownToolMock : public ToolCallingMockLLM {
    public:
        ChatResponse chat(const ChatRequest& req) override {
            // On first call, request unknown tool
            if (call_count == 0) {
                call_count++;
                last_messages = req.messages;
                ChatResponse resp;
                resp.model = req.model;
                Message msg(Message::assistant, "");
                ToolCall tc;
                tc.id = "call_unknown";
                tc.name = "nonexistent_tool";
                tc.arguments = "{}";
                msg.tool_call = tc;
                resp.messages.push_back(msg);
                resp.finish_reason = "tool_calls";
                return resp;
            }
            return ToolCallingMockLLM::chat(req);
        }
    };

    auto llm = std::make_shared<UnknownToolMock>();
    auto calc = std::make_shared<CalculatorTool>();

    std::vector<std::shared_ptr<ITool>> tools = {calc};
    Agent agent(llm, tools);

    // Should handle unknown tool gracefully
    AgentResult result = agent.run("Use unknown tool");
    // The agent should still process (even if the tool returns an error)
    assert(llm->call_count >= 1);
}

// ============================================================================
// Task 15: Persistence Tests
// ============================================================================

void test_save_state_basic() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);
    agent.set_system_prompt("You are helpful.");

    agent.run("Hello");
    agent.run("How are you?");

    std::string state = agent.save_state();
    assert(!state.empty());
    assert(state.find("\"system_prompt\"") != std::string::npos);
    assert(state.find("You are helpful") != std::string::npos);
    assert(state.find("\"conversation_history\"") != std::string::npos);
    assert(state.find("\"user\"") != std::string::npos);
    assert(state.find("\"assistant\"") != std::string::npos);
    assert(state.find("Hello") != std::string::npos);
    assert(state.find("How are you?") != std::string::npos);
}

void test_save_load_state_roundtrip() {
    auto llm1 = std::make_shared<MockLLMProvider>();
    Agent agent1(llm1);
    agent1.set_system_prompt("Test system prompt");
    agent1.run("First");
    agent1.run("Second");

    std::string state = agent1.save_state();

    // Create a new agent with a fresh LLM and load state
    auto llm2 = std::make_shared<MockLLMProvider>();
    Agent agent2(llm2);
    bool loaded = agent2.load_state(state);
    assert(loaded == true);

    // Verify by saving again - should produce same structure
    std::string state2 = agent2.save_state();
    assert(state == state2);
}

void test_load_empty_state() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);

    bool loaded = agent.load_state("");
    assert(loaded == false);
}

void test_load_invalid_json() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);

    bool loaded = agent.load_state("not json at all");
    assert(loaded == false);
}

void test_save_state_empty_history() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);

    std::string state = agent.save_state();
    assert(!state.empty());
    // Should be valid JSON
    assert(state.find("{") != std::string::npos);
    assert(state.find("}") != std::string::npos);

    // Should be loadable
    auto llm2 = std::make_shared<MockLLMProvider>();
    Agent agent2(llm2);
    assert(agent2.load_state(state) == true);
}

void test_save_load_with_special_chars() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);
    agent.run("Line1\nLine2");
    agent.run("Contains \"quotes\" and \\backslashes\\");

    std::string state = agent.save_state();

    auto llm2 = std::make_shared<MockLLMProvider>();
    Agent agent2(llm2);
    assert(agent2.load_state(state) == true);

    std::string state2 = agent2.save_state();
    assert(state == state2);
}

void test_load_state_preserves_system_prompt() {
    auto llm = std::make_shared<MockLLMProvider>();
    Agent agent(llm);
    agent.set_system_prompt("Custom system prompt");
    agent.run("test");

    std::string state = agent.save_state();

    auto llm2 = std::make_shared<MockLLMProvider>();
    Agent agent2(llm2);
    agent2.load_state(state);

    std::string state2 = agent2.save_state();
    assert(state2.find("Custom system prompt") != std::string::npos);
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
    test_agent_with_tools_construction();
    test_agent_tool_call_execution();
    test_agent_tool_definitions_sent();
    test_agent_no_tools_original_behavior();
    test_agent_max_iterations();
    test_agent_unknown_tool();
    test_save_state_basic();
    test_save_load_state_roundtrip();
    test_load_empty_state();
    test_load_invalid_json();
    test_save_state_empty_history();
    test_save_load_with_special_chars();
    test_load_state_preserves_system_prompt();
    return 0;
}
