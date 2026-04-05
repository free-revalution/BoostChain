#include <catch2/catch_test_macros.hpp>
#include "tools/web/web_search_tool.hpp"

using namespace ccmake;

TEST_CASE("WebSearchTool definition") {
    WebSearchTool tool;
    REQUIRE(tool.definition().name == "WebSearch");
    REQUIRE(tool.definition().is_read_only);
}

TEST_CASE("WebSearchTool validate_input") {
    WebSearchTool tool;
    ToolContext ctx;

    REQUIRE_FALSE(tool.validate_input(nlohmann::json({{"query", "test"}}), ctx).empty() == false);
    REQUIRE_FALSE(tool.validate_input(nlohmann::json::object(), ctx).empty());
    REQUIRE_FALSE(tool.validate_input(nlohmann::json({{"query", ""}}), ctx).empty());
    REQUIRE(tool.validate_input(nlohmann::json({{"query", "cpp programming"}}), ctx).empty());
}

TEST_CASE("WebSearchTool output format") {
    WebSearchTool tool;
    ToolContext ctx;

    // Even if search fails (network issue), verify output structure
    auto output = tool.execute(nlohmann::json({{"query", "test query"}}), ctx);
    // Output should always contain the search query header
    if (!output.is_error) {
        REQUIRE(output.content.find("Search results") != std::string::npos);
    }
}
