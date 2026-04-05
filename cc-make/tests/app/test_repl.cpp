#include <catch2/catch_test_macros.hpp>
#include "app/repl.hpp"
#include "query/query_engine.hpp"
#include "tools/file/read_tool.hpp"
#include <sstream>

using namespace ccmake;

// Helper: create a QueryEngine with a mock response and bypass permissions
static std::unique_ptr<QueryEngine> make_test_engine(const std::string& mock_response = "Mock response") {
    auto engine = std::make_unique<QueryEngine>("test-model");
    engine->set_cwd("/tmp");
    engine->set_permission_mode(PermissionMode::BypassPermissions);
    engine->register_tool(std::make_unique<ReadTool>());
    engine->set_mock_response(Message::assistant("a1", {TextBlock{mock_response}}));
    return engine;
}

#define ENGINE(name, ...) auto name = make_test_engine(__VA_ARGS__)

TEST_CASE("Repl constructor") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE(true);
}

TEST_CASE("Repl process_prompt returns 0") {
    ENGINE(engine, "Hello!");
    CLIArgs args;
    Repl repl(*engine, args);
    int result = repl.process_prompt("test prompt");
    REQUIRE(result == 0);
}

TEST_CASE("Repl process_prompt adds messages to engine") {
    ENGINE(engine, "Response");
    CLIArgs args;
    Repl repl(*engine, args);
    repl.process_prompt("hello");
    REQUIRE(engine->messages().size() >= 2);
}

TEST_CASE("Repl handle_command /help") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE(repl.handle_command("/help"));
}

TEST_CASE("Repl handle_command /quit") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE(repl.handle_command("/quit"));
}

TEST_CASE("Repl handle_command /model") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE(repl.handle_command("/model opus"));
    REQUIRE(engine->model() == "opus");
}

TEST_CASE("Repl handle_command /mode") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE(repl.handle_command("/mode bypassPermissions"));
    REQUIRE(engine->permission_manager().mode() == PermissionMode::BypassPermissions);
}

TEST_CASE("Repl handle_command /status") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE(repl.handle_command("/status"));
}

TEST_CASE("Repl handle_command ignores non-commands") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE_FALSE(repl.handle_command("regular text"));
    REQUIRE_FALSE(repl.handle_command(""));
    REQUIRE_FALSE(repl.handle_command("not a command"));
}

TEST_CASE("Repl handle_command unknown command") {
    ENGINE(engine);
    CLIArgs args;
    Repl repl(*engine, args);
    REQUIRE(repl.handle_command("/unknown"));
}

TEST_CASE("Repl multiple prompts accumulate messages") {
    auto engine = std::make_unique<QueryEngine>("test-model");
    engine->set_cwd("/tmp");
    engine->set_permission_mode(PermissionMode::BypassPermissions);
    engine->register_tool(std::make_unique<ReadTool>());
    engine->set_mock_responses({
        Message::assistant("a1", {TextBlock{"First"}}),
        Message::assistant("a2", {TextBlock{"Second"}})
    });

    CLIArgs args;
    Repl repl(*engine, args);
    repl.process_prompt("prompt 1");
    repl.process_prompt("prompt 2");

    REQUIRE(engine->messages().size() >= 4);
}
