#include <catch2/catch_test_macros.hpp>
#include "tools/brief/brief_tool.hpp"
using namespace ccmake;

TEST_CASE("BriefTool definition", "[brief]") {
    BriefTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Brief");
    REQUIRE_FALSE(def.description.empty());
}

TEST_CASE("BriefTool is read only", "[brief]") {
    BriefTool tool;
    auto& def = tool.definition();
    REQUIRE(def.is_read_only);
    REQUIRE_FALSE(def.is_destructive);
}

TEST_CASE("BriefTool truncate short content", "[brief]") {
    BriefTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"content", "Hello world"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content == "Hello world");
    // No truncation marker
    REQUIRE(result.content.find("...") == std::string::npos);
}

TEST_CASE("BriefTool truncate long content", "[brief]") {
    BriefTool tool;
    ToolContext ctx;

    // Build a string longer than 50 chars
    std::string long_content;
    for (int i = 0; i < 100; i++) {
        long_content += "word" + std::to_string(i) + " ";
    }

    auto result = tool.execute({{"content", long_content}, {"max_length", 50}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.size() <= 53);  // max_length + "..."
    REQUIRE(result.content.find("...") != std::string::npos);
    // Should not be cut off mid-word (within reason)
    REQUIRE(result.content.back() != ' ');
}

TEST_CASE("BriefTool default max length", "[brief]") {
    BriefTool tool;
    ToolContext ctx;

    // Content under 500 chars
    std::string short_content = "This is a short summary.";
    auto result = tool.execute({{"content", short_content}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content == short_content);

    // Content over 500 chars
    std::string long_content;
    for (int i = 0; i < 200; i++) {
        long_content += "word" + std::to_string(i) + " ";
    }
    REQUIRE(long_content.size() > 500);

    auto result2 = tool.execute({{"content", long_content}}, ctx);
    REQUIRE_FALSE(result2.is_error);
    REQUIRE(result2.content.size() <= 503);  // 500 + "..."
    REQUIRE(result2.content.find("...") != std::string::npos);
}

TEST_CASE("BriefTool validate missing content", "[brief]") {
    BriefTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("BriefTool validate accepts valid input", "[brief]") {
    BriefTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"content", "hello"}}, ctx);
    REQUIRE(err.empty());
}

TEST_CASE("BriefTool truncates at word boundary", "[brief]") {
    BriefTool tool;
    ToolContext ctx;

    // Content where truncation would land mid-word
    std::string content = "The quick brown fox jumps over the lazy dog and then some more text here";
    // max_length = 20 would land on "he" from "the"
    auto result = tool.execute({{"content", content}, {"max_length", 20}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("...") != std::string::npos);
    // Should end at a word boundary, not mid-word
    REQUIRE(result.content.find("he...") == std::string::npos);
}
