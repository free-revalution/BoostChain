#include <catch2/catch_test_macros.hpp>
#include "query/agentic_loop.hpp"
#include "core/types.hpp"
#include <nlohmann/json.hpp>
using namespace ccmake;

// Mock API call that returns predefined responses
static Message make_text_response(const std::string& id, const std::string& text) {
    return Message::assistant(id, {TextBlock{text}});
}

static Message make_tool_response(const std::string& id, const std::string& tool_id,
                                   const std::string& tool_name, const nlohmann::json& input) {
    return Message::assistant(id, {ToolUseBlock{tool_id, tool_name, input}});
}

TEST_CASE("Agentic loop completes with text-only response") {
    int call_count = 0;
    auto api_call = [&](const std::vector<Message>&, const std::string&,
                        const std::vector<APIToolDefinition>&, QueryEventCallback,
                        const std::vector<APIToolDefinition>&) -> Message {
        return make_text_response("m" + std::to_string(call_count++), "Hello!");
    };

    AgenticLoopConfig config;
    config.max_turns = 10;

    auto result = run_agentic_loop(api_call, config, Message::user("u1", "Hi"));
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    REQUIRE(result.messages.size() == 2);  // user + assistant
}

TEST_CASE("Agentic loop executes tool and continues") {
    int call_count = 0;
    auto api_call = [&](const std::vector<Message>&, const std::string&,
                        const std::vector<APIToolDefinition>&, QueryEventCallback,
                        const std::vector<APIToolDefinition>&) -> Message {
        if (call_count == 0) {
            call_count++;
            return make_tool_response("m1", "toolu_1", "echo", {{"text", "hi"}});
        } else {
            call_count++;
            return make_text_response("m2", "Done!");
        }
    };

    AgenticLoopConfig config;
    config.max_turns = 10;

    ToolExecutor executor;
    executor.register_tool("echo", [](const std::string&, const nlohmann::json& input) { return input; });

    config.tool_executor = &executor;

    auto result = run_agentic_loop(api_call, config, Message::user("u1", "Test"));
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    REQUIRE(result.messages.size() >= 4);  // user, asst(tool), user(tool_result), asst(text)
}

TEST_CASE("Agentic loop stops at max turns") {
    int call_count = 0;
    auto api_call = [&](const std::vector<Message>&, const std::string&,
                        const std::vector<APIToolDefinition>&, QueryEventCallback,
                        const std::vector<APIToolDefinition>&) -> Message {
        call_count++;
        return make_tool_response("m1", "toolu_1", "echo", {{"text", "loop"}});
    };

    AgenticLoopConfig config;
    config.max_turns = 2;

    auto result = run_agentic_loop(api_call, config, Message::user("u1", "Loop"));
    REQUIRE(result.exit_reason == LoopExitReason::MaxTurns);
}

TEST_CASE("Agentic loop handles abort signal") {
    AbortSignal signal;

    auto api_call = [&](const std::vector<Message>&, const std::string&,
                        const std::vector<APIToolDefinition>&, QueryEventCallback,
                        const std::vector<APIToolDefinition>&) -> Message {
        signal.abort();  // Abort before returning
        return make_tool_response("m1", "toolu_1", "echo", {{"text", "abort_me"}});
    };

    AgenticLoopConfig config;
    config.max_turns = 10;
    config.abort_signal = &signal;

    auto result = run_agentic_loop(api_call, config, Message::user("u1", "Abort"));
    REQUIRE(result.exit_reason == LoopExitReason::Aborted);
}

TEST_CASE("Agentic loop handles API error") {
    auto api_call = [&](const std::vector<Message>&, const std::string&,
                        const std::vector<APIToolDefinition>&, QueryEventCallback,
                        const std::vector<APIToolDefinition>&) -> Message {
        throw std::runtime_error("API unavailable");
    };

    AgenticLoopConfig config;
    config.max_turns = 10;

    auto result = run_agentic_loop(api_call, config, Message::user("u1", "Error test"));
    REQUIRE(result.exit_reason == LoopExitReason::ModelError);
    REQUIRE(result.error_message == "API unavailable");
}

TEST_CASE("Agentic loop emits events via callback") {
    std::vector<std::string> received_events;

    auto api_call = [&](const std::vector<Message>&, const std::string&,
                        const std::vector<APIToolDefinition>&, QueryEventCallback on_event,
                        const std::vector<APIToolDefinition>&) -> Message {
        if (on_event) {
            on_event(QueryEventAssistant{make_text_response("m1", "Hi")});
        }
        return make_text_response("m1", "Hi");
    };

    AgenticLoopConfig config;
    config.max_turns = 10;
    config.on_event = [&](const QueryEvent& event) {
        if (std::holds_alternative<QueryEventAssistant>(event)) {
            received_events.push_back("assistant");
        }
    };

    auto result = run_agentic_loop(api_call, config, Message::user("u1", "Event test"));
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    REQUIRE_FALSE(received_events.empty());
}

TEST_CASE("Agentic loop no tool executor produces error results") {
    int call_count = 0;
    auto api_call = [&](const std::vector<Message>&, const std::string&,
                        const std::vector<APIToolDefinition>&, QueryEventCallback,
                        const std::vector<APIToolDefinition>&) -> Message {
        if (call_count == 0) {
            call_count++;
            return make_tool_response("m1", "toolu_1", "missing_tool", {{"x", 1}});
        }
        call_count++;
        return make_text_response("m2", "Done despite error");
    };

    AgenticLoopConfig config;
    config.max_turns = 10;
    // No tool_executor set

    auto result = run_agentic_loop(api_call, config, Message::user("u1", "No executor"));
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    // Should have error tool result in messages
    bool found_error = false;
    for (const auto& msg : result.messages) {
        for (const auto& block : msg.content) {
            if (auto* tr = std::get_if<ToolResultBlock>(&block)) {
                if (tr->is_error) found_error = true;
            }
        }
    }
    REQUIRE(found_error);
}
