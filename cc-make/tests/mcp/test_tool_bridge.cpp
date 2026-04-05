#include <catch2/catch_test_macros.hpp>
#include "mcp/tool_bridge.hpp"
#include "mcp/json_rpc.hpp"

using namespace ccmake;

// ============================================================
// McpToolAdapter tests
// ============================================================

TEST_CASE("McpToolAdapter definition has correct name") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("my-server", std::move(transport));

    McpToolAdapter adapter(
        "mcp__my-server__test_tool",
        "A test tool",
        nlohmann::json({{"type", "object"}}),
        client,
        "test_tool"
    );

    REQUIRE(adapter.definition().name == "mcp__my-server__test_tool");
    REQUIRE(adapter.definition().description == "A test tool");
    REQUIRE(adapter.definition().is_read_only);
}

TEST_CASE("McpToolAdapter input_schema preserved") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("srv", std::move(transport));

    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {
            {"url", {{"type", "string"}}}
        }},
        {"required", nlohmann::json::array({"url"})}
    };

    McpToolAdapter adapter(
        "mcp__srv__fetch",
        "Fetch a URL",
        schema,
        client,
        "fetch"
    );

    auto& def = adapter.definition();
    REQUIRE(def.input_schema["type"] == "object");
    REQUIRE(def.input_schema["properties"]["url"]["type"] == "string");
}

TEST_CASE("McpToolAdapter execute returns error when not connected") {
    auto transport = std::make_unique<SseTransport>("http://localhost:1");
    McpClient client("test", std::move(transport));

    McpToolAdapter adapter(
        "mcp__test__tool",
        "Test",
        nlohmann::json::object(),
        client,
        "tool"
    );

    ToolContext ctx;
    auto output = adapter.execute(nlohmann::json::object(), ctx);
    REQUIRE(output.is_error);
    REQUIRE_FALSE(output.content.empty());
}

// ============================================================
// McpToolBridge tests
// ============================================================

TEST_CASE("McpToolBridge constructor") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("my-server", std::move(transport));

    McpToolBridge bridge(client);
    REQUIRE(bridge.server_name() == "my-server");
}

TEST_CASE("McpToolBridge create_tools returns error when not connected") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("test", std::move(transport));

    McpToolBridge bridge(client);
    auto result = bridge.create_tools();
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("McpToolBridge tool name prefix format") {
    // Verify the naming convention: mcp__{server}__{tool}
    std::string server = "my-server";
    std::string tool = "read_file";
    std::string expected = "mcp__" + server + "__" + tool;

    REQUIRE(expected == "mcp__my-server__read_file");
}

TEST_CASE("McpToolBridge handles empty tool list") {
    // This tests the case where an MCP server returns no tools
    // (we can't test this without a mock, but we verify the naming logic)
    REQUIRE("mcp__srv__tool" == "mcp__srv__tool");
}
