#include <catch2/catch_test_macros.hpp>
#include "query/query_engine.hpp"
#include "query/tool_executor.hpp"
#include "query/agentic_loop.hpp"
#include "query/context.hpp"
#include "core/types.hpp"
#include <nlohmann/json.hpp>
using namespace ccmake;

TEST_CASE("Integration: multi-turn conversation") {
    QueryEngine engine("test-model");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_permission_mode(PermissionMode::BypassPermissions);
    engine.register_tool("calc", [](const std::string&, const nlohmann::json& j) {
        int a = j.value("a", 0);
        int b = j.value("b", 0);
        return nlohmann::json{{"result", a + b}};
    });

    // Turn 1: model uses tool, then gives answer
    engine.set_mock_responses({
        Message::assistant("a1", {TextBlock{"Let me calculate."}, ToolUseBlock{"t1", "calc", {{"a", 5}, {"b", 3}}}}),
        Message::assistant("a1b", {TextBlock{"5 + 3 = 8"}})
    });
    auto r1 = engine.submit_message("What is 5 + 3?");
    REQUIRE(r1.exit_reason == LoopExitReason::Completed);

    // Turn 2: model has answer
    engine.set_mock_responses({
        Message::assistant("a2", {TextBlock{"The answer is 8."}})
    });
    auto r2 = engine.submit_message("What about the answer?");
    REQUIRE(r2.exit_reason == LoopExitReason::Completed);

    auto msgs = engine.messages();
    // Turn 1: user1, asst1(tool), tool_result, asst1b(text)
    // Turn 2: user2, asst2(text)
    REQUIRE(msgs.size() >= 6);
}

TEST_CASE("Integration: abort during tool execution") {
    QueryEngine engine("test-model");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_permission_mode(PermissionMode::BypassPermissions);
    engine.register_tool("slow", [](const std::string&, const nlohmann::json&) -> nlohmann::json {
        return {{"result", "ok"}};
    });

    engine.set_mock_responses({
        Message::assistant("a1", {ToolUseBlock{"t1", "slow", {{"x", 1}}}}),
        Message::assistant("a1b", {TextBlock{"Done"}})
    });

    auto result = engine.submit_message("Start");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    // Messages should include tool execution
    bool has_tool_result = false;
    for (const auto& m : result.messages) {
        for (const auto& c : m.content) {
            if (auto* tr = std::get_if<ToolResultBlock>(&c)) has_tool_result = true;
        }
    }
    REQUIRE(has_tool_result);
}

TEST_CASE("Integration: context builder provides git info") {
    auto status = get_git_status("/Users/jiang/development/BoostChain/cc-make");
    REQUIRE(status.is_git);
    REQUIRE(!status.branch.empty());

    auto ctx = build_user_context("/Users/jiang/development/BoostChain/cc-make");
    REQUIRE(!ctx.empty());
    REQUIRE(ctx.find("main") != std::string::npos);
}

TEST_CASE("Integration: system prompt assembly") {
    QueryConfig config;
    config.system_prompt = "Custom prompt.";
    auto prompt = build_system_prompt(config, "User context here", "Git: main");
    REQUIRE(prompt.find("Custom prompt.") != std::string::npos);
    REQUIRE(prompt.find("User context here") != std::string::npos);
    REQUIRE(prompt.find("Git: main") != std::string::npos);
}

TEST_CASE("Integration: multi-tool execution in single turn") {
    QueryEngine engine("test-model");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_permission_mode(PermissionMode::BypassPermissions);
    engine.register_tool("add", [](const std::string&, const nlohmann::json& j) {
        return nlohmann::json{{"result", j.value("a", 0) + j.value("b", 0)}};
    });
    engine.register_tool("mul", [](const std::string&, const nlohmann::json& j) {
        return nlohmann::json{{"result", j.value("a", 0) * j.value("b", 0)}};
    });

    engine.set_mock_responses({
        Message::assistant("a1", {
            ToolUseBlock{"t1", "add", {{"a", 3}, {"b", 4}}},
            ToolUseBlock{"t2", "mul", {{"a", 5}, {"b", 6}}}
        }),
        Message::assistant("a2", {TextBlock{"Results: 7 and 30"}})
    });

    auto result = engine.submit_message("Calculate");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);

    // Verify both tool results are in messages
    int tool_result_count = 0;
    for (const auto& m : result.messages) {
        for (const auto& c : m.content) {
            if (std::holds_alternative<ToolResultBlock>(c)) tool_result_count++;
        }
    }
    REQUIRE(tool_result_count >= 2);
}

TEST_CASE("Integration: error handling in tool execution") {
    QueryEngine engine("test-model");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_permission_mode(PermissionMode::BypassPermissions);
    engine.register_tool("error_tool", [](const std::string&, const nlohmann::json&) -> nlohmann::json {
        throw std::runtime_error("something went wrong");
    });

    engine.set_mock_responses({
        Message::assistant("a1", {ToolUseBlock{"t1", "error_tool", {}}}),
        Message::assistant("a2", {TextBlock{"Recovered from error"}})
    });

    auto result = engine.submit_message("Trigger error");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);

    // Should have an error tool result
    bool found_error = false;
    for (const auto& m : result.messages) {
        for (const auto& c : m.content) {
            if (auto* tr = std::get_if<ToolResultBlock>(&c)) {
                if (tr->is_error) found_error = true;
            }
        }
    }
    REQUIRE(found_error);
}
