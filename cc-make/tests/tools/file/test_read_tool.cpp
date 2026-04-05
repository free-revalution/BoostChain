#include <catch2/catch_test_macros.hpp>
#include "tools/file/read_tool.hpp"
#include <fstream>
#include <filesystem>
using namespace ccmake;

static std::string make_test_file(const std::string& content) {
    static int counter = 0;
    auto path = "/tmp/ccmake_test_read_" + std::to_string(counter++) + ".txt";
    std::ofstream f(path);
    f << content;
    f.close();
    return path;
}

TEST_CASE("ReadTool reads entire file") {
    auto path = make_test_file("line1\nline2\nline3\n");
    ReadTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("line1") != std::string::npos);
    REQUIRE(result.content.find("line2") != std::string::npos);
    REQUIRE(result.content.find("line3") != std::string::npos);
    std::filesystem::remove(path);
}

TEST_CASE("ReadTool reads with offset") {
    auto path = make_test_file("a\nb\nc\nd\n");
    ReadTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"offset", 3}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("a") == std::string::npos);
    REQUIRE(result.content.find("b") == std::string::npos);
    REQUIRE(result.content.find("c") != std::string::npos);
    REQUIRE(result.content.find("d") != std::string::npos);
    std::filesystem::remove(path);
}

TEST_CASE("ReadTool reads with limit") {
    auto path = make_test_file("1\n2\n3\n4\n5\n");
    ReadTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"offset", 1}, {"limit", 2}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("1") != std::string::npos);
    REQUIRE(result.content.find("2") != std::string::npos);
    REQUIRE(result.content.find("3") == std::string::npos);
    std::filesystem::remove(path);
}

TEST_CASE("ReadTool returns error for nonexistent file") {
    ReadTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", "/nonexistent/file.txt"}}, ctx);
    REQUIRE(result.is_error);
}

TEST_CASE("ReadTool returns empty for empty file") {
    auto path = make_test_file("");
    ReadTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.empty());
    std::filesystem::remove(path);
}

TEST_CASE("ReadTool has line numbers") {
    auto path = make_test_file("first\nsecond\n");
    ReadTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}}, ctx);
    REQUIRE(result.content.find("1\tfirst") != std::string::npos);
    REQUIRE(result.content.find("2\tsecond") != std::string::npos);
    std::filesystem::remove(path);
}

TEST_CASE("ReadTool definition is correct") {
    ReadTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Read");
    REQUIRE(def.is_read_only);
    REQUIRE_FALSE(def.is_destructive);
    REQUIRE_FALSE(def.aliases.empty());
}

TEST_CASE("ReadTool validate_input rejects missing file_path") {
    ReadTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"other", "value"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("ReadTool validate_input accepts valid input") {
    ReadTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"file_path", "/some/file.txt"}}, ctx);
    REQUIRE(err.empty());
}
