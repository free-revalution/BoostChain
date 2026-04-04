#include <catch2/catch_test_macros.hpp>
#include "api/sse_parser.hpp"
using namespace ccmake;

TEST_CASE("SSE parser emits event from complete input") {
    SSEParser parser;
    auto events = parser.feed("data: {\"type\":\"message_start\"}\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data == "{\"type\":\"message_start\"}");
    REQUIRE(events[0].event_type.empty());
}

TEST_CASE("SSE parser handles event type field") {
    SSEParser parser;
    auto events = parser.feed("event: content_block_delta\ndata: {\"delta\":\"hi\"}\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].event_type == "content_block_delta");
    REQUIRE(events[0].data == "{\"delta\":\"hi\"}");
}

TEST_CASE("SSE parser handles multi-line data") {
    SSEParser parser;
    auto events = parser.feed("data: line1\ndata: line2\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data == "line1\nline2");
}

TEST_CASE("SSE parser buffers incomplete event") {
    SSEParser parser;
    auto events = parser.feed("data: partial");
    REQUIRE(events.empty());
    events = parser.feed(" data\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data == "partial data");
}

TEST_CASE("SSE parser handles multiple events in one feed") {
    SSEParser parser;
    auto events = parser.feed("data: first\n\ndata: second\n\n");
    REQUIRE(events.size() == 2);
    REQUIRE(events[0].data == "first");
    REQUIRE(events[1].data == "second");
}

TEST_CASE("SSE parser ignores comments") {
    SSEParser parser;
    auto events = parser.feed(": this is a comment\ndata: hello\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data == "hello");
}

TEST_CASE("SSE parser handles id field") {
    SSEParser parser;
    auto events = parser.feed("id: evt-123\ndata: hello\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].id == "evt-123");
}

TEST_CASE("SSE parser handles retry field") {
    SSEParser parser;
    auto events = parser.feed("retry: 5000\ndata: hello\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].retry_ms == 5000);
}

TEST_CASE("SSE parser empty data field emits empty string") {
    SSEParser parser;
    auto events = parser.feed("data:\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data.empty());
}

TEST_CASE("SSE parser reset clears buffer") {
    SSEParser parser;
    parser.feed("data: partial");
    parser.reset();
    auto events = parser.feed("data: new\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data == "new");
}

TEST_CASE("SSE parser handles BOM-prefixed streams") {
    SSEParser parser;
    auto events = parser.feed("\xef\xbb" "\xbf" "data: hello\n\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data == "hello");
}

TEST_CASE("SSE parser handles real Anthropic streaming events") {
    SSEParser parser;
    std::string stream =
        "event: message_start\ndata: {\"type\":\"message_start\"}\n\n"
        "event: content_block_start\ndata: {\"type\":\"content_block_start\"}\n\n"
        "event: content_block_delta\ndata: {\"type\":\"content_block_delta\"}\n\n"
        "event: content_block_stop\ndata: {\"type\":\"content_block_stop\"}\n\n"
        "event: message_delta\ndata: {\"type\":\"message_delta\"}\n\n"
        "event: message_stop\ndata: {\"type\":\"message_stop\"}\n\n";
    auto events = parser.feed(stream);
    REQUIRE(events.size() == 6);
    REQUIRE(events[0].event_type == "message_start");
    REQUIRE(events[5].event_type == "message_stop");
}

TEST_CASE("SSE parser handles field without colon") {
    SSEParser parser;
    auto events = parser.feed("data\ndata: actual\n\n");
    REQUIRE(events.size() == 1);
    // "data" without colon is treated as field "data" with empty value,
    // then "data: actual" appends. Result: "\nactual"
    REQUIRE(events[0].data == "\nactual");
}

TEST_CASE("SSE parser handles \\r\\n line endings") {
    SSEParser parser;
    auto events = parser.feed("data: hello\r\n\r\n");
    REQUIRE(events.size() == 1);
    REQUIRE(events[0].data == "hello");
}
