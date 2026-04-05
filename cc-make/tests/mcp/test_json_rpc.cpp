#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include "mcp/json_rpc.hpp"
#include "mcp/types.hpp"

using namespace ccmake;
using Catch::Matchers::ContainsSubstring;

// ============================================================
// Encoding Tests
// ============================================================

TEST_CASE("encode_request produces valid JSON-RPC request") {
    JsonRpcRequest req;
    req.id = "1";
    req.method = "initialize";
    req.params = nlohmann::json({
        {"protocolVersion", "2024-11-05"},
        {"capabilities", nlohmann::json::object()},
        {"clientInfo", {{"name", "cc-make"}, {"version", "0.1.0"}}}
    });

    auto encoded = encode_request(req);
    auto j = nlohmann::json::parse(encoded);

    REQUIRE(j["jsonrpc"] == "2.0");
    REQUIRE(j["id"] == "1");
    REQUIRE(j["method"] == "initialize");
    REQUIRE(j["params"]["protocolVersion"] == "2024-11-05");
    REQUIRE(j["params"]["clientInfo"]["name"] == "cc-make");
}

TEST_CASE("encode_request with null params omits params") {
    JsonRpcRequest req;
    req.id = "2";
    req.method = "ping";

    auto encoded = encode_request(req);
    auto j = nlohmann::json::parse(encoded);

    REQUIRE(j["method"] == "ping");
    REQUIRE_FALSE(j.contains("params"));
}

TEST_CASE("encode_notification produces valid JSON-RPC notification") {
    JsonRpcNotification notif;
    notif.method = "notifications/initialized";
    notif.params = nlohmann::json::object();

    auto encoded = encode_notification(notif);
    auto j = nlohmann::json::parse(encoded);

    REQUIRE(j["jsonrpc"] == "2.0");
    REQUIRE_FALSE(j.contains("id"));
    REQUIRE(j["method"] == "notifications/initialized");
}

TEST_CASE("encode_notification with empty params") {
    JsonRpcNotification notif;
    notif.method = "notifications/cancelled";

    auto encoded = encode_notification(notif);
    auto j = nlohmann::json::parse(encoded);

    REQUIRE(j["method"] == "notifications/cancelled");
    REQUIRE_FALSE(j.contains("params"));
}

// ============================================================
// Decoding Tests
// ============================================================

TEST_CASE("decode_message parses response with result") {
    std::string json = R"({
        "jsonrpc": "2.0",
        "id": "1",
        "result": {
            "protocolVersion": "2024-11-05",
            "capabilities": {"tools": {}},
            "serverInfo": {"name": "test-server", "version": "1.0"}
        }
    })";

    auto result = decode_message(json);
    REQUIRE(result.has_value());

    auto& msg = result.value();
    auto* resp = std::get_if<JsonRpcResponse>(&msg);
    REQUIRE(resp != nullptr);
    REQUIRE(resp->result.has_value());
    REQUIRE((*resp->result)["protocolVersion"] == "2024-11-05");
}

TEST_CASE("decode_message parses response with error") {
    std::string json = R"({
        "jsonrpc": "2.0",
        "id": "2",
        "error": {"code": -32601, "message": "Method not found"}
    })";

    auto result = decode_message(json);
    REQUIRE(result.has_value());

    auto& msg = result.value();
    auto* resp = std::get_if<JsonRpcResponse>(&msg);
    REQUIRE(resp != nullptr);
    REQUIRE(resp->error.has_value());
    REQUIRE(resp->error->code == -32601);
    REQUIRE(resp->error->message == "Method not found");
}

TEST_CASE("decode_message parses error with data") {
    std::string json = R"({
        "jsonrpc": "2.0",
        "id": "3",
        "error": {"code": -32602, "message": "Invalid params", "data": {"field": "name"}}
    })";

    auto result = decode_message(json);
    REQUIRE(result.has_value());

    auto* resp = std::get_if<JsonRpcResponse>(&result.value());
    REQUIRE(resp != nullptr);
    REQUIRE(resp->error->data.has_value());
    REQUIRE((*resp->error->data)["field"] == "name");
}

TEST_CASE("decode_message parses notification") {
    std::string json = R"({
        "jsonrpc": "2.0",
        "method": "notifications/initialized"
    })";

    auto result = decode_message(json);
    REQUIRE(result.has_value());

    auto* notif = std::get_if<JsonRpcNotification>(&result.value());
    REQUIRE(notif != nullptr);
    REQUIRE(notif->method == "notifications/initialized");
}

TEST_CASE("decode_message parses server request") {
    std::string json = R"({
        "jsonrpc": "2.0",
        "id": "10",
        "method": "tools/list",
        "params": {}
    })";

    auto result = decode_message(json);
    REQUIRE(result.has_value());

    auto* req = std::get_if<JsonRpcRequest>(&result.value());
    REQUIRE(req != nullptr);
    REQUIRE(req->method == "tools/list");
    REQUIRE(req->id == "10");
}

TEST_CASE("decode_message handles numeric id") {
    std::string json = R"({
        "jsonrpc": "2.0",
        "id": 42,
        "result": "ok"
    })";

    auto result = decode_message(json);
    REQUIRE(result.has_value());

    auto* resp = std::get_if<JsonRpcResponse>(&result.value());
    REQUIRE(resp != nullptr);
    REQUIRE(resp->id == 42);
}

TEST_CASE("decode_message rejects invalid JSON") {
    auto result = decode_message("{not valid json}");
    REQUIRE_FALSE(result.has_value());
    REQUIRE_THAT(result.error(), ContainsSubstring("parse error"));
}

TEST_CASE("decode_message rejects missing jsonrpc field") {
    std::string json = R"({"id": "1", "method": "test"})";
    auto result = decode_message(json);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("decode_message rejects wrong jsonrpc version") {
    std::string json = R"({"jsonrpc": "1.0", "id": "1", "method": "test"})";
    auto result = decode_message(json);
    REQUIRE_FALSE(result.has_value());
}

// ============================================================
// Content-Length Framing Tests
// ============================================================

TEST_CASE("frame_message creates Content-Length header") {
    std::string body = R"({"jsonrpc":"2.0","id":"1","method":"test"})";
    auto framed = frame_message(body);

    REQUIRE_THAT(framed, ContainsSubstring("Content-Length:"));
    REQUIRE_THAT(framed, ContainsSubstring("\r\n\r\n"));
    REQUIRE_THAT(framed, ContainsSubstring(body));
}

TEST_CASE("frame_message has correct Content-Length value") {
    std::string body = R"({"test":true})";
    auto framed = frame_message(body);

    // Extract Content-Length value
    auto cl_pos = framed.find("Content-Length: ");
    REQUIRE(cl_pos != std::string::npos);
    auto value_start = cl_pos + 16;
    auto value_end = framed.find("\r\n", value_start);
    auto cl_value = std::stoi(framed.substr(value_start, value_end - value_start));
    REQUIRE(cl_value == static_cast<int>(body.size()));
}

TEST_CASE("parse_framed_message extracts body from complete frame") {
    std::string body = R"({"jsonrpc":"2.0","id":"1","result":"ok"})";
    auto framed = frame_message(body);

    auto result = parse_framed_message(framed);
    REQUIRE(result.has_value());
    REQUIRE(result->first == body);
    REQUIRE(result->second == framed.size());
}

TEST_CASE("parse_framed_message returns nullopt for incomplete header") {
    std::string partial = "Content-Length: 50\r\n";
    auto result = parse_framed_message(partial);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("parse_framed_message returns nullopt for incomplete body") {
    std::string partial = "Content-Length: 100\r\n\r\n{";  // Only 1 byte of 100
    auto result = parse_framed_message(partial);
    REQUIRE_FALSE(result.has_value());
}

TEST_CASE("parse_framed_message handles extra data after frame") {
    std::string body = R"({"ok":true})";
    std::string extra = "NEXT_FRAME";
    auto framed = frame_message(body) + extra;

    auto result = parse_framed_message(framed);
    REQUIRE(result.has_value());
    REQUIRE(result->first == body);
    // Consumed should be exactly the first frame
    REQUIRE(framed.substr(result->second) == extra);
}

TEST_CASE("parse_framed_message handles multiple frames sequentially") {
    std::string body1 = R"({"id":"1"})";
    std::string body2 = R"({"id":"2"})";
    auto frame1 = frame_message(body1);
    auto frame2 = frame_message(body2);
    std::string combined = frame1 + frame2;

    auto result1 = parse_framed_message(combined);
    REQUIRE(result1.has_value());
    REQUIRE(result1->first == body1);

    auto remaining = combined.substr(result1->second);
    auto result2 = parse_framed_message(remaining);
    REQUIRE(result2.has_value());
    REQUIRE(result2->first == body2);
}

// ============================================================
// Request ID Generation
// ============================================================

TEST_CASE("generate_request_id returns unique IDs") {
    std::string id1 = generate_request_id();
    std::string id2 = generate_request_id();
    REQUIRE_FALSE(id1.empty());
    REQUIRE_FALSE(id2.empty());
    REQUIRE(id1 != id2);
}

TEST_CASE("generate_request_id returns numeric strings") {
    auto id = generate_request_id();
    REQUIRE_NOTHROW(std::stoi(id));
}
