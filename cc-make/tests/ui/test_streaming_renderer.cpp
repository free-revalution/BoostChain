#include <catch2/catch_test_macros.hpp>
#include "ui/streaming_renderer.hpp"
#include "query/types.hpp"
#include <sstream>
#include <iostream>

// Redirect cout to a string for testing
static std::string capture_stdout(std::function<void()> fn) {
    std::ostringstream oss;
    auto old_cout = std::cout.rdbuf(oss.rdbuf());
    auto old_cerr = std::cerr.rdbuf(oss.rdbuf());
    fn();
    std::cout.rdbuf(old_cout);
    std::cerr.rdbuf(old_cerr);
    return oss.str();
}

using namespace ccmake;

TEST_CASE("StreamingRenderer constructor") {
    StreamingRenderer renderer;
    REQUIRE(renderer.text_output().empty());
    REQUIRE(renderer.tool_call_count() == 0);
}

TEST_CASE("StreamingRenderer handle text event") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventAssistant{Message::assistant("a1", {TextBlock{"Hello world"}})});
    });

    REQUIRE_FALSE(renderer.text_output().empty());
    REQUIRE(output.find("Hello world") != std::string::npos);
}

TEST_CASE("StreamingRenderer handle tool use event") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventToolUse{"t1", "Read", {{"file_path", "/tmp/test.txt"}}});
    });

    REQUIRE(renderer.tool_call_count() == 1);
    REQUIRE(output.find("Read") != std::string::npos);
}

TEST_CASE("StreamingRenderer handle tool result event") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventToolUse{"t1", "Read", {{"file_path", "/tmp/test.txt"}}});
        cb(QueryEventToolResult{"t1", "file content", false});
    });

    REQUIRE(renderer.tool_call_count() == 1);
    REQUIRE(renderer.error_count() == 0);
}

TEST_CASE("StreamingRenderer handle error tool result") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    capture_stdout([&]() {
        cb(QueryEventToolResult{"t1", "error message", true});
    });

    REQUIRE(renderer.error_count() == 1);
}

TEST_CASE("StreamingRenderer handle error event") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventError{"something went wrong", false});
    });

    REQUIRE(renderer.error_count() == 1);
}

TEST_CASE("StreamingRenderer handle thinking event") {
    StreamingRenderer renderer;
    renderer.set_show_thinking(true);
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventThinking{"hmm..."});
    });

    REQUIRE(output.find("hmm...") != std::string::npos);
}

TEST_CASE("StreamingRenderer thinking hidden by default") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventThinking{"hmm..."});
    });

    REQUIRE(output.find("hmm...") == std::string::npos);
}

TEST_CASE("StreamingRenderer reset") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    capture_stdout([&]() {
        cb(QueryEventAssistant{Message::assistant("a1", {TextBlock{"test"}})});
    });
    REQUIRE_FALSE(renderer.text_output().empty());

    renderer.reset();
    REQUIRE(renderer.text_output().empty());
    REQUIRE(renderer.tool_call_count() == 0);
    REQUIRE(renderer.error_count() == 0);
}

TEST_CASE("StreamingRenderer on_complete with tools") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventToolUse{"t1", "Read", {{"file_path", "/tmp/a"}}});
        cb(QueryEventToolResult{"t1", "ok", false});
        renderer.on_complete();
    });

    REQUIRE(output.find("1 tool call") != std::string::npos);
}

TEST_CASE("StreamingRenderer on_complete with errors") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventToolUse{"t1", "Bash", {}});
        cb(QueryEventToolResult{"t1", "failed", true});
        renderer.on_complete();
    });

    REQUIRE(output.find("1 error") != std::string::npos);
}

TEST_CASE("StreamingRenderer on_complete empty") {
    StreamingRenderer renderer;
    auto cb = renderer.make_callback();

    std::string output = capture_stdout([&]() {
        cb(QueryEventAssistant{Message::assistant("a1", {TextBlock{"Hi"}})});
        renderer.on_complete();
    });

    REQUIRE(output.find("tool call") == std::string::npos);
}
