#include <catch2/catch_test_macros.hpp>
#include "api/messages.hpp"
#include "api/claude_client.hpp"
#include "api/sse_parser.hpp"
#include "api/errors.hpp"
#include "core/types.hpp"
#include <nlohmann/json.hpp>
using json = nlohmann::json;
using namespace ccmake;

TEST_CASE("End-to-end: build request from messages") {
    std::vector<Message> messages;
    messages.push_back(Message::user("u1", "What files are in /tmp?"));
    messages.push_back(Message::assistant("a1", {
        ToolUseBlock{"toolu_1", "Bash", json({{"command", "ls /tmp"}})}
    }));
    Message user2 = Message::user("u2", "");
    user2.content.clear();
    user2.content.push_back(ToolResultBlock{"toolu_1", "file1.txt\nfile2.txt", false});
    messages.push_back(user2);

    auto req = build_api_request("claude-sonnet-4-20250514", messages, "Helpful.", {});
    auto j = api_request_to_json(req);

    REQUIRE(j["model"] == "claude-sonnet-4-20250514");
    REQUIRE(j["messages"].size() == 3);
    REQUIRE(j["messages"][0]["role"] == "user");
    REQUIRE(j["messages"][1]["role"] == "assistant");
    REQUIRE(j["messages"][2]["role"] == "user");
    REQUIRE(j["system"].size() == 1);
    REQUIRE(j["system"][0] == "Helpful.");
}

TEST_CASE("End-to-end: full streaming session simulation") {
    StreamSession session;

    session.on_message_start(StreamMessageStart{"msg_1", "claude-sonnet-4-20250514", {100, 0}});

    // Thinking block
    session.on_content_block_start(StreamContentBlockStart{0, APIBlockStartThinking{"", ""}});
    session.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaThinking{"User wants file info."}});
    session.on_content_block_delta(StreamContentBlockDelta{0, APIDeltaSignature{"sig"}});
    session.on_content_block_stop(StreamContentBlockStop{0});

    // Text block
    session.on_content_block_start(StreamContentBlockStart{1, APIBlockStartText{""}});
    session.on_content_block_delta(StreamContentBlockDelta{1, APIDeltaText{"Found the following:"}});
    session.on_content_block_stop(StreamContentBlockStop{1});

    // Tool use block
    session.on_content_block_start(StreamContentBlockStart{2, APIBlockStartToolUse{"toolu_1", "Glob", ""}});
    session.on_content_block_delta(StreamContentBlockDelta{2, APIDeltaToolInput{"{\"pattern\":\"*.txt\"}"}});
    session.on_content_block_stop(StreamContentBlockStop{2});

    // End
    StreamMessageDelta delta;
    delta.stop_reason = "tool_use";
    delta.usage.output_tokens = 42;
    session.on_message_delta(delta);

    auto msg = session.build_message();
    REQUIRE(msg.role == MessageRole::Assistant);
    REQUIRE(msg.id == "msg_1");
    REQUIRE(msg.content.size() == 3);
    REQUIRE(msg.stop_reason == StopReason::ToolUse);
    REQUIRE(msg.usage.total_input_tokens == 100);
    REQUIRE(msg.usage.total_output_tokens == 42);
}

TEST_CASE("End-to-end: SSE + parse_stream_event pipeline") {
    SSEParser parser;
    std::string stream =
        "event: message_start\ndata: {\"type\":\"message_start\",\"message\":{\"id\":\"msg_1\",\"model\":\"claude-sonnet-4-20250514\",\"usage\":{\"input_tokens\":50,\"output_tokens\":0}}}\n\n"
        "event: content_block_start\ndata: {\"type\":\"content_block_start\",\"index\":0,\"content_block\":{\"type\":\"text\",\"text\":\"\"}}\n\n"
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hello!\"}}\n\n"
        "event: content_block_stop\ndata: {\"type\":\"content_block_stop\",\"index\":0}\n\n"
        "event: message_delta\ndata: {\"type\":\"message_delta\",\"delta\":{\"stop_reason\":\"end_turn\"},\"usage\":{\"output_tokens\":6}}\n\n"
        "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n";

    auto sse_events = parser.feed(stream);
    REQUIRE(sse_events.size() == 6);

    StreamSession session;
    for (auto& sse : sse_events) {
        auto event = parse_stream_event(sse.event_type, sse.data);
        std::visit([&](auto& e) {
            using T = std::decay_t<decltype(e)>;
            if constexpr (std::is_same_v<T, StreamMessageStart>) session.on_message_start(e);
            else if constexpr (std::is_same_v<T, StreamContentBlockStart>) session.on_content_block_start(e);
            else if constexpr (std::is_same_v<T, StreamContentBlockDelta>) session.on_content_block_delta(e);
            else if constexpr (std::is_same_v<T, StreamContentBlockStop>) session.on_content_block_stop(e);
            else if constexpr (std::is_same_v<T, StreamMessageDelta>) session.on_message_delta(e);
        }, event);
    }

    auto msg = session.build_message();
    REQUIRE(msg.id == "msg_1");
    REQUIRE(msg.content.size() == 1);
    REQUIRE(msg.stop_reason == StopReason::EndTurn);
    REQUIRE(msg.usage.total_input_tokens == 50);
    REQUIRE(msg.usage.total_output_tokens == 6);

    auto* text = std::get_if<TextBlock>(&msg.content[0]);
    REQUIRE(text != nullptr);
    REQUIRE(text->text == "Hello!");
}

TEST_CASE("End-to-end: error classification in retry loop") {
    // Simulate retry decision for a 429 error
    std::string body = R"({"type":"error","error":{"type":"rate_limit_error","message":"Rate limit"}})";
    auto err = parse_api_error(429, body);
    REQUIRE(should_retry_error(err));
    REQUIRE(err.error_type == "rate_limit_error");

    // Non-retryable 400 error
    auto err2 = parse_api_error(400, R"({"type":"error","error":{"type":"invalid_request_error","message":"Bad"}})");
    REQUIRE_FALSE(should_retry_error(err2));
}

TEST_CASE("End-to-end: retry delay sequence") {
    // Simulate retry delays for 3 attempts
    int total_delay = 0;
    for (int i = 1; i <= 3; ++i) {
        total_delay += get_retry_delay_ms(i, std::nullopt);
    }
    // Should be in a reasonable range (500 + 1000 + 2000 + jitter)
    REQUIRE(total_delay > 3000);
    REQUIRE(total_delay < 5000);
}

TEST_CASE("End-to-end: request with tools") {
    std::vector<Message> messages;
    messages.push_back(Message::user("u1", "Read /tmp/test.txt"));

    std::vector<APIToolDefinition> tools;
    tools.push_back({"Read", "Read a file", {{"type","object"},{"properties",{{"path",{{"type","string"}}}}},{"required",json::array({"path"})}}});
    tools.push_back({"Write", "Write a file", {{"type","object"},{"properties",{{"path",{{"type","string"}}},{"content",{{"type","string"}}}}},{"required",json::array({"path","content"})}}});

    auto req = build_api_request("claude-sonnet-4-20250514", messages, "", tools);
    auto j = api_request_to_json(req);

    REQUIRE(j["tools"].size() == 2);
    REQUIRE(j["tools"][0]["name"] == "Read");
    REQUIRE(j["tools"][1]["name"] == "Write");
}

TEST_CASE("End-to-end: request with thinking config") {
    std::vector<Message> messages;
    messages.push_back(Message::user("u1", "Think about this"));
    auto req = build_api_request("claude-sonnet-4-20250514", messages, "", {}, 8192, APIThinkingConfig{"enabled", 10000});
    auto j = api_request_to_json(req);
    REQUIRE(j.contains("thinking"));
    REQUIRE(j["thinking"]["type"] == "enabled");
    REQUIRE(j["thinking"]["budget_tokens"] == 10000);
}

TEST_CASE("End-to-end: SSE chunked input") {
    // Simulate SSE data arriving in small chunks
    SSEParser parser;

    auto e1 = parser.feed("event: content_block_start\n");
    REQUIRE(e1.empty());  // Incomplete event

    auto e2 = parser.feed("data: {\"type\":\"content_block_start\",\"index\":0,\"cont");
    REQUIRE(e2.empty());

    auto e3 = parser.feed("ent_block\":{\"type\":\"text\",\"text\":\"\"}}\n\n");
    REQUIRE(e3.size() == 1);
    REQUIRE(e3[0].event_type == "content_block_start");

    auto e4 = parser.feed("data: {\"type\":\"content_block_delta\",\"index\":0,\"delta\":{\"type\":\"text_delta\",\"text\":\"Hi\"}}\n\n");
    REQUIRE(e4.size() == 1);
}
