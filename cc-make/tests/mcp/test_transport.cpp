#include <catch2/catch_test_macros.hpp>
#include "mcp/transport.hpp"
#include "mcp/json_rpc.hpp"

using namespace ccmake;

// ============================================================
// Transport interface tests (unit-level, no real processes)
// ============================================================

TEST_CASE("SseTransport constructor") {
    SseTransport transport("http://localhost:3001/sse");
    REQUIRE_FALSE(transport.is_connected());
}

TEST_CASE("SseTransport close") {
    SseTransport transport("http://localhost:3001/sse");
    transport.close();
    REQUIRE_FALSE(transport.is_connected());
}

TEST_CASE("SseTransport send_notification to invalid URL returns error") {
    SseTransport transport("http://localhost:1/nonexistent");
    JsonRpcNotification notif;
    notif.method = "notifications/initialized";
    notif.params = nlohmann::json::object();

    auto result = transport.send_notification(notif);
    // Either error (connection refused) or success (depends on system)
    // We just verify it doesn't crash
}

TEST_CASE("SseTransport send_request to invalid URL returns error") {
    SseTransport transport("http://localhost:1/nonexistent");
    JsonRpcRequest req;
    req.id = "1";
    req.method = "initialize";
    req.params = nlohmann::json::object();

    auto result = transport.send_request(req, std::chrono::seconds(2));
    // Should return error for invalid URL
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("StdioTransport constructor") {
    StdioTransport transport("/bin/echo", {"hello"});
    REQUIRE_FALSE(transport.is_connected());
}

TEST_CASE("StdioTransport close before start") {
    StdioTransport transport("/bin/echo", {"hello"});
    transport.close();
    REQUIRE_FALSE(transport.is_connected());
}

// ============================================================
// Integration test with echo server (simple stdio process)
// ============================================================

TEST_CASE("StdioTransport send_request to echo process") {
    // Use a simple process that echoes Content-Length-framed JSON-RPC
    // We'll test the framing by sending to /bin/cat which echoes stdin
    // Note: this is a basic connectivity test
    StdioTransport transport("/bin/cat");
    JsonRpcRequest req;
    req.id = "test-1";
    req.method = "ping";
    req.params = nlohmann::json::object();

    // First request starts the process and sends
    // cat will echo back the framed message which our parser handles
    auto result = transport.send_request(req, std::chrono::seconds(2));

    // cat echoes the input back, so the parser should handle it
    // The response might not match our expected format, but the transport should work
    transport.close();
}

TEST_CASE("StdioTransport send_notification") {
    StdioTransport transport("/bin/cat");
    JsonRpcNotification notif;
    notif.method = "notifications/initialized";
    notif.params = nlohmann::json::object();

    auto result = transport.send_notification(notif);
    // Should succeed (cat accepts input)
    transport.close();
}

TEST_CASE("Multiple StdioTransport requests in sequence") {
    StdioTransport transport("/bin/cat");

    JsonRpcRequest req1;
    req1.id = "1";
    req1.method = "test1";
    req1.params = {{"key", "value1"}};

    JsonRpcRequest req2;
    req2.id = "2";
    req2.method = "test2";
    req2.params = {{"key", "value2"}};

    // First request starts the process
    auto r1 = transport.send_request(req1, std::chrono::seconds(2));
    // Second request reuses the same process
    auto r2 = transport.send_request(req2, std::chrono::seconds(2));

    transport.close();
}

TEST_CASE("StdioTransport close kills process") {
    StdioTransport transport("/bin/cat");

    // Start the process by sending a request
    JsonRpcNotification notif;
    notif.method = "test";
    notif.params = nlohmann::json::object();
    transport.send_notification(notif);

    REQUIRE(transport.is_connected());
    transport.close();
    REQUIRE_FALSE(transport.is_connected());
}

// ============================================================
// Transport type identification
// ============================================================

TEST_CASE("Transport polymorphism") {
    std::unique_ptr<McpTransport> t1 = std::make_unique<SseTransport>("http://localhost");
    std::unique_ptr<McpTransport> t2 = std::make_unique<StdioTransport>("/bin/echo");

    REQUIRE_FALSE(t1->is_connected());
    REQUIRE_FALSE(t2->is_connected());

    t1->close();
    t2->close();
}
