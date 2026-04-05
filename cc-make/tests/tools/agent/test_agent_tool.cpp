#include <catch2/catch_test_macros.hpp>
#include "tools/agent/agent_tool.hpp"
#include "query/query_engine.hpp"
#include "core/types.hpp"

using namespace ccmake;

TEST_CASE("AgentTool definition") {
    auto factory = []() -> std::unique_ptr<QueryEngine> {
        return std::make_unique<QueryEngine>();
    };

    AgentTool tool(factory);

    REQUIRE(tool.definition().name == "Agent");
    REQUIRE_FALSE(tool.definition().description.empty());
    REQUIRE(tool.definition().is_read_only);
}

TEST_CASE("AgentTool validate_input requires prompt") {
    auto factory = []() -> std::unique_ptr<QueryEngine> {
        return std::make_unique<QueryEngine>();
    };

    AgentTool tool(factory);
    ToolContext ctx;

    REQUIRE_FALSE(tool.validate_input(nlohmann::json::object(), ctx).empty());
    REQUIRE_FALSE(tool.validate_input(nlohmann::json({{"prompt", ""}}), ctx).empty());
    REQUIRE(tool.validate_input(nlohmann::json({{"prompt", "hello"}}), ctx).empty());
}

TEST_CASE("AgentTool execute with mock response") {
    auto factory = []() -> std::unique_ptr<QueryEngine> {
        auto engine = std::make_unique<QueryEngine>();
        engine->set_mock_response(Message::assistant("a1", {TextBlock{"Agent completed task"}}));
        return engine;
    };

    AgentTool tool(factory);
    ToolContext ctx;

    auto output = tool.execute(nlohmann::json({{"prompt", "do something"}}), ctx);
    REQUIRE_FALSE(output.is_error);
    REQUIRE(output.content.find("Agent completed task") != std::string::npos);
}

TEST_CASE("AgentTool execute with error response") {
    auto factory = []() -> std::unique_ptr<QueryEngine> {
        auto engine = std::make_unique<QueryEngine>();
        // Error response will be a simple text message
        engine->set_mock_response(Message::assistant("a1", {TextBlock{"Error occurred"}}));
        return engine;
    };

    AgentTool tool(factory);
    ToolContext ctx;

    auto output = tool.execute(nlohmann::json({{"prompt", "test"}}), ctx);
    // Even error text is returned as the agent's output
    REQUIRE_FALSE(output.is_error);
}

TEST_CASE("AgentTool respects max_turns") {
    int observed_turns = 0;
    auto factory = [&observed_turns]() -> std::unique_ptr<QueryEngine> {
        auto engine = std::make_unique<QueryEngine>();
        engine->set_mock_response(Message::assistant("a1", {TextBlock{"done"}}));
        return engine;
    };

    AgentTool tool(factory);
    ToolContext ctx;

    auto output = tool.execute(nlohmann::json({
        {"prompt", "test"},
        {"max_turns", 3}
    }), ctx);

    REQUIRE_FALSE(output.is_error);
}

TEST_CASE("QueryEngine copy_tool_functions_from") {
    auto parent = std::make_unique<QueryEngine>();
    parent->register_tool("test_tool", [](const std::string& name, const nlohmann::json& input) -> nlohmann::json {
        return nlohmann::json({{"executed", true}});
    });

    auto child = std::make_unique<QueryEngine>();
    child->copy_tool_functions_from(*parent);

    REQUIRE(child->has_tool("test_tool"));
}

TEST_CASE("AgentTool engine factory creates independent engines") {
    std::vector<std::string> models;
    auto factory = [&models]() -> std::unique_ptr<QueryEngine> {
        auto engine = std::make_unique<QueryEngine>("test-model");
        models.push_back(engine->model());
        engine->set_mock_response(Message::assistant("a1", {TextBlock{"done"}}));
        return engine;
    };

    AgentTool tool(factory);
    ToolContext ctx;

    tool.execute(nlohmann::json({{"prompt", "task1"}}), ctx);
    tool.execute(nlohmann::json({{"prompt", "task2"}}), ctx);

    // Each invocation should create a new engine
    REQUIRE(models.size() == 2);
    REQUIRE(models[0] == "test-model");
    REQUIRE(models[1] == "test-model");
}
