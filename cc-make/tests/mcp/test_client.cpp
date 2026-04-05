#include <catch2/catch_test_macros.hpp>
#include "mcp/client.hpp"
#include "mcp/json_rpc.hpp"

using namespace ccmake;

// ============================================================
// McpClient construction and lifecycle tests
// ============================================================

TEST_CASE("McpClient constructor with stdio transport") {
    auto transport = std::make_unique<StdioTransport>("/bin/echo", std::vector<std::string>{});
    McpClient client("test-server", std::move(transport));

    REQUIRE(client.server_name() == "test-server");
    REQUIRE_FALSE(client.is_connected());
}

TEST_CASE("McpClient constructor with SSE transport") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("sse-server", std::move(transport));

    REQUIRE(client.server_name() == "sse-server");
    REQUIRE_FALSE(client.is_connected());
}

TEST_CASE("McpClient disconnect when not connected") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("test", std::move(transport));

    // Should not crash
    client.disconnect();
    REQUIRE_FALSE(client.is_connected());
}

TEST_CASE("McpClient connect to invalid server returns error") {
    auto transport = std::make_unique<SseTransport>("http://localhost:1/nonexistent");
    McpClient client("test", std::move(transport));

    auto result = client.connect();
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("McpClient capabilities default to false") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("test", std::move(transport));

    auto caps = client.capabilities();
    REQUIRE_FALSE(caps.tools);
    REQUIRE_FALSE(caps.resources);
    REQUIRE_FALSE(caps.prompts);
}

TEST_CASE("McpClient list_tools when not connected returns error") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("test", std::move(transport));

    auto result = client.list_tools();
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == "Not connected");
}

TEST_CASE("McpClient call_tool when not connected returns error") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("test", std::move(transport));

    auto result = client.call_tool("some_tool", nlohmann::json::object());
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == "Not connected");
}

TEST_CASE("McpClient list_resources when not connected returns error") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("test", std::move(transport));

    auto result = client.list_resources();
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == "Not connected");
}

TEST_CASE("McpClient read_resource when not connected returns error") {
    auto transport = std::make_unique<SseTransport>("http://localhost:3001");
    McpClient client("test", std::move(transport));

    auto result = client.read_resource("file:///test.txt");
    REQUIRE_FALSE(result.has_value());
    REQUIRE(result.error() == "Not connected");
}

TEST_CASE("McpClient connect is idempotent") {
    auto transport = std::make_unique<SseTransport>("http://localhost:1");
    McpClient client("test", std::move(transport));

    // First connect fails
    auto r1 = client.connect();
    REQUIRE_FALSE(r1.has_value());

    // Second connect also reports error (not connected)
    auto r2 = client.connect();
    REQUIRE_FALSE(r2.has_value());
}
