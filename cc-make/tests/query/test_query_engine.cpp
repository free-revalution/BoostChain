#include <catch2/catch_test_macros.hpp>
#include "query/query_engine.hpp"
#include "core/types.hpp"
#include <nlohmann/json.hpp>
using namespace ccmake;

TEST_CASE("QueryEngine default construction") {
    QueryEngine engine("default-model");
    REQUIRE(engine.model() == "default-model");
    REQUIRE(engine.messages().empty());
}

TEST_CASE("QueryEngine set model") {
    QueryEngine engine("model-a");
    engine.set_model("model-b");
    REQUIRE(engine.model() == "model-b");
}

TEST_CASE("QueryEngine set system prompt") {
    QueryEngine engine("m");
    engine.set_system_prompt("You are helpful.");
    REQUIRE(engine.system_prompt() == "You are helpful.");
}

TEST_CASE("QueryEngine register tools") {
    QueryEngine engine("m");
    engine.register_tool("echo", [](const std::string&, const nlohmann::json& j) { return j; });
    REQUIRE(engine.has_tool("echo"));
    REQUIRE_FALSE(engine.has_tool("nope"));
}

TEST_CASE("QueryEngine submit_message text-only returns completed") {
    QueryEngine engine("m");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_mock_response(Message::assistant("a1", {TextBlock{"Hi!"}}));

    auto result = engine.submit_message("Hello");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    REQUIRE_FALSE(result.messages.empty());
}

TEST_CASE("QueryEngine submit_message accumulates messages") {
    QueryEngine engine("m");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_mock_response(Message::assistant("a1", {TextBlock{"First"}}));
    engine.submit_message("One");

    engine.set_mock_response(Message::assistant("a2", {TextBlock{"Second"}}));
    engine.submit_message("Two");

    auto msgs = engine.messages();
    REQUIRE(msgs.size() >= 4);
}

TEST_CASE("QueryEngine interrupt doesn't crash") {
    QueryEngine engine("m");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.register_tool("echo", [](const std::string&, const nlohmann::json& j) { return j; });
    engine.set_mock_response(Message::assistant("a1", {
        ToolUseBlock{"t1", "echo", {{"wait", "forever"}}}
    }));
    engine.submit_message("Start");
    engine.interrupt();
    REQUIRE(engine.has_tool("echo"));
}

TEST_CASE("QueryEngine tool execution with echo") {
    QueryEngine engine("m");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.register_tool("echo", [](const std::string&, const nlohmann::json& j) { return j; });
    engine.set_mock_responses({
        Message::assistant("a1", {ToolUseBlock{"t1", "echo", {{"x", 42}}}}),
        Message::assistant("a2", {TextBlock{"Got 42"}})
    });

    auto result = engine.submit_message("Echo test");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
    REQUIRE(result.messages.size() >= 4);
}

TEST_CASE("QueryEngine max turns") {
    QueryEngine engine("m");
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_max_turns(1);
    engine.set_mock_response(Message::assistant("a1", {
        ToolUseBlock{"t1", "echo", {{"x", 1}}}
    }));

    auto result = engine.submit_message("Max turn test");
    REQUIRE(result.exit_reason == LoopExitReason::MaxTurns);
}

TEST_CASE("QueryEngine set cwd") {
    QueryEngine engine("m");
    engine.set_cwd("/tmp");
    engine.set_mock_response(Message::assistant("a1", {TextBlock{"ok"}}));
    auto result = engine.submit_message("test");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
}

TEST_CASE("QueryEngine set max tokens") {
    QueryEngine engine("m");
    engine.set_max_tokens(4096);
    engine.set_cwd("/Users/jiang/development/BoostChain/cc-make");
    engine.set_mock_response(Message::assistant("a1", {TextBlock{"ok"}}));
    auto result = engine.submit_message("test");
    REQUIRE(result.exit_reason == LoopExitReason::Completed);
}
