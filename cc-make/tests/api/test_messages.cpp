#include <catch2/catch_test_macros.hpp>
#include "api/messages.hpp"
#include "core/types.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace ccmake;

TEST_CASE("message_to_api_param converts user text") {
    Message msg = Message::user("u1", "Hello");
    auto param = message_to_api_param(msg);
    REQUIRE(param.role == "user");
    REQUIRE(param.content.size() == 1);
}

TEST_CASE("message_to_api_param converts tool result") {
    Message msg = Message::user("u2", "");
    msg.content.clear();
    msg.content.push_back(ToolResultBlock{"tool-1", "contents", false});
    auto param = message_to_api_param(msg);
    REQUIRE(param.role == "user");
    REQUIRE(param.content.size() == 1);
}

TEST_CASE("build_api_request produces valid JSON") {
    std::vector<Message> msgs;
    msgs.push_back(Message::user("u1", "Hello"));
    auto req = build_api_request("claude-sonnet-4-20250514", msgs, "Helpful assistant.", {});
    auto j = api_request_to_json(req);
    REQUIRE(j["model"] == "claude-sonnet-4-20250514");
    REQUIRE(j["max_tokens"] == 8192);
    REQUIRE(j["stream"] == true);
    REQUIRE(j["messages"].size() == 1);
    REQUIRE(j["messages"][0]["role"] == "user");
    REQUIRE(j["system"].size() == 1);
    REQUIRE(j["system"][0] == "Helpful assistant.");
}

TEST_CASE("build_api_request with thinking config") {
    std::vector<Message> msgs;
    msgs.push_back(Message::user("u1", "Hello"));
    auto req = build_api_request("claude-sonnet-4-20250514", msgs, "", {}, 8192, APIThinkingConfig{"adaptive", std::nullopt});
    auto j = api_request_to_json(req);
    REQUIRE(j.contains("thinking"));
    REQUIRE(j["thinking"]["type"] == "adaptive");
}

TEST_CASE("build_api_request with tools") {
    std::vector<Message> msgs;
    msgs.push_back(Message::user("u1", "Hello"));
    std::vector<APIToolDefinition> tools;
    tools.push_back({"Read", "Read a file", {{"type","object"},{"properties",{{"path",{{"type","string"}}}}},{"required",json::array({"path"})}}});
    auto req = build_api_request("claude-sonnet-4-20250514", msgs, "", tools);
    auto j = api_request_to_json(req);
    REQUIRE(j["tools"].size() == 1);
    REQUIRE(j["tools"][0]["name"] == "Read");
}

TEST_CASE("build_api_request with assistant message") {
    std::vector<Message> msgs;
    msgs.push_back(Message::user("u1", "Read the file"));
    msgs.push_back(Message::assistant("a1", {
        ToolUseBlock{"toolu_1", "Bash", json({{"command","ls"}})}
    }));
    auto req = build_api_request("claude-sonnet-4-20250514", msgs, "", {});
    auto j = api_request_to_json(req);
    REQUIRE(j["messages"].size() == 2);
    REQUIRE(j["messages"][1]["role"] == "assistant");
}

TEST_CASE("parse_stream_event message_start") {
    std::string data = R"({"type":"message_start","message":{"id":"msg_1","model":"claude-sonnet-4-20250514","usage":{"input_tokens":25,"output_tokens":0}}})";
    auto event = parse_stream_event("message_start", data);
    auto* s = std::get_if<StreamMessageStart>(&event);
    REQUIRE(s != nullptr);
    REQUIRE(s->id == "msg_1");
    REQUIRE(s->usage.input_tokens == 25);
}

TEST_CASE("parse_stream_event content_block_start text") {
    std::string data = R"({"type":"content_block_start","index":0,"content_block":{"type":"text","text":""}})";
    auto event = parse_stream_event("content_block_start", data);
    auto* s = std::get_if<StreamContentBlockStart>(&event);
    REQUIRE(s != nullptr);
    REQUIRE(std::holds_alternative<APIBlockStartText>(s->block));
}

TEST_CASE("parse_stream_event content_block_start tool_use") {
    std::string data = R"({"type":"content_block_start","index":1,"content_block":{"type":"tool_use","id":"toolu_1","name":"Read","input":""}})";
    auto event = parse_stream_event("content_block_start", data);
    auto* s = std::get_if<StreamContentBlockStart>(&event);
    REQUIRE(s != nullptr);
    auto* t = std::get_if<APIBlockStartToolUse>(&s->block);
    REQUIRE(t != nullptr);
    REQUIRE(t->name == "Read");
}

TEST_CASE("parse_stream_event content_block_start thinking") {
    std::string data = R"({"type":"content_block_start","index":0,"content_block":{"type":"thinking","thinking":"","signature":""}})";
    auto event = parse_stream_event("content_block_start", data);
    auto* s = std::get_if<StreamContentBlockStart>(&event);
    REQUIRE(s != nullptr);
    REQUIRE(std::holds_alternative<APIBlockStartThinking>(s->block));
}

TEST_CASE("parse_stream_event content_block_delta text") {
    std::string data = R"({"type":"content_block_delta","index":0,"delta":{"type":"text_delta","text":"Hello"}})";
    auto event = parse_stream_event("content_block_delta", data);
    auto* d = std::get_if<StreamContentBlockDelta>(&event);
    REQUIRE(d != nullptr);
    auto* t = std::get_if<APIDeltaText>(&d->delta);
    REQUIRE(t != nullptr);
    REQUIRE(t->text == "Hello");
}

TEST_CASE("parse_stream_event content_block_delta thinking") {
    std::string data = R"({"type":"content_block_delta","index":0,"delta":{"type":"thinking_delta","thinking":"hmm"}})";
    auto event = parse_stream_event("content_block_delta", data);
    auto* d = std::get_if<StreamContentBlockDelta>(&event);
    REQUIRE(d != nullptr);
    auto* t = std::get_if<APIDeltaThinking>(&d->delta);
    REQUIRE(t != nullptr);
}

TEST_CASE("parse_stream_event content_block_delta tool input") {
    std::string data = R"({"type":"content_block_delta","index":1,"delta":{"type":"input_json_delta","partial_json":"{\"path\":"}})";
    auto event = parse_stream_event("content_block_delta", data);
    auto* d = std::get_if<StreamContentBlockDelta>(&event);
    REQUIRE(d != nullptr);
    auto* t = std::get_if<APIDeltaToolInput>(&d->delta);
    REQUIRE(t != nullptr);
}

TEST_CASE("parse_stream_event content_block_delta signature") {
    std::string data = R"({"type":"content_block_delta","index":0,"delta":{"type":"signature_delta","signature":"sig123"}})";
    auto event = parse_stream_event("content_block_delta", data);
    auto* d = std::get_if<StreamContentBlockDelta>(&event);
    REQUIRE(d != nullptr);
    auto* t = std::get_if<APIDeltaSignature>(&d->delta);
    REQUIRE(t != nullptr);
    REQUIRE(t->signature == "sig123");
}

TEST_CASE("parse_stream_event content_block_stop") {
    std::string data = R"({"type":"content_block_stop","index":0})";
    auto event = parse_stream_event("content_block_stop", data);
    REQUIRE(std::holds_alternative<StreamContentBlockStop>(event));
}

TEST_CASE("parse_stream_event message_delta") {
    std::string data = R"({"type":"message_delta","delta":{"stop_reason":"end_turn"},"usage":{"output_tokens":15}})";
    auto event = parse_stream_event("message_delta", data);
    auto* d = std::get_if<StreamMessageDelta>(&event);
    REQUIRE(d != nullptr);
    REQUIRE(d->stop_reason == "end_turn");
    REQUIRE(d->usage.output_tokens == 15);
}

TEST_CASE("parse_stream_event error") {
    std::string data = R"({"type":"error","error":{"type":"overloaded_error","message":"Overloaded"}})";
    auto event = parse_stream_event("error", data);
    auto* e = std::get_if<StreamError>(&event);
    REQUIRE(e != nullptr);
    REQUIRE(e->error_type == "overloaded_error");
}

TEST_CASE("parse_error_response from JSON") {
    std::string body = R"({"type":"error","error":{"type":"invalid_request_error","message":"bad request"}})";
    auto err = parse_error_response(400, body);
    REQUIRE(err.status_code == 400);
    REQUIRE(err.error_type == "invalid_request_error");
}

TEST_CASE("parse_error_response from malformed") {
    auto err = parse_error_response(500, "not json");
    REQUIRE(err.status_code == 500);
}
