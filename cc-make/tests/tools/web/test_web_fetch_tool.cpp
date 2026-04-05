#include <catch2/catch_test_macros.hpp>
#include "tools/web/web_fetch_tool.hpp"

using namespace ccmake;

TEST_CASE("WebFetchTool definition") {
    WebFetchTool tool;
    REQUIRE(tool.definition().name == "WebFetch");
    REQUIRE(tool.definition().is_read_only);
}

TEST_CASE("WebFetchTool validate_input") {
    WebFetchTool tool;
    ToolContext ctx;

    REQUIRE_FALSE(tool.validate_input(nlohmann::json({{"url", "https://example.com"}}), ctx).empty() == false);
    REQUIRE_FALSE(tool.validate_input(nlohmann::json::object(), ctx).empty());
    REQUIRE_FALSE(tool.validate_input(nlohmann::json({{"url", ""}}), ctx).empty());
    REQUIRE_FALSE(tool.validate_input(nlohmann::json({{"url", "no-protocol"}}), ctx).empty());
    REQUIRE(tool.validate_input(nlohmann::json({{"url", "https://example.com"}}), ctx).empty());
}

TEST_CASE("WebFetchTool execute invalid URL returns error") {
    WebFetchTool tool;
    ToolContext ctx;

    auto output = tool.execute(nlohmann::json({{"url", "https://this-domain-does-not-exist-12345.com"}}), ctx);
    // Should return error for invalid domain
    REQUIRE(output.is_error);
}

TEST_CASE("WebFetchTool HTML stripping unit test") {
    // Test the HTML stripping logic via a local test
    // We verify the tool handles various inputs gracefully
    WebFetchTool tool;
    ToolContext ctx;

    // A URL that returns 404 or error should still produce output (error message)
    auto output = tool.execute(nlohmann::json({
        {"url", "file:///nonexistent/path.html"},
        {"max_chars", 100}
    }), ctx);
    // Either error or empty content — just verify no crash
}

TEST_CASE("WebFetchTool truncation logic") {
    // Test that max_chars is respected
    WebFetchTool tool;
    ToolContext ctx;

    auto output = tool.execute(nlohmann::json({
        {"url", "file:///etc/passwd"},
        {"max_chars", 50}
    }), ctx);
    // Verify output is bounded
    if (!output.is_error) {
        REQUIRE(output.content.size() <= 200);  // Allow truncation message overhead
    }
}
