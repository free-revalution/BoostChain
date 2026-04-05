#include <catch2/catch_test_macros.hpp>
#include "tools/search/grep_tool.hpp"
#include <filesystem>
#include <fstream>
using namespace ccmake;

static std::string make_test_dir() {
    static int counter = 0;
    auto dir = "/tmp/ccmake_grep_" + std::to_string(counter++);
    std::filesystem::create_directories(dir);
    std::ofstream(dir + "/hello.js") << "function hello() {\n  console.log('hi');\n}\n";
    std::ofstream(dir + "/world.js") << "function world() {\n  return 42;\n}\n";
    std::ofstream(dir + "/readme.txt") << "This is a readme.\nNothing to see.\n";
    std::filesystem::create_directories(dir + "/sub");
    std::ofstream(dir + "/sub/nested.py") << "def greet():\n    print('hello world')\n";
    return dir;
}

TEST_CASE("GrepTool finds pattern in files") {
    auto dir = make_test_dir();
    GrepTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "function"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("hello.js") != std::string::npos);
    REQUIRE(result.content.find("world.js") != std::string::npos);
    REQUIRE(result.content.find("readme.txt") == std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GrepTool searches recursively") {
    auto dir = make_test_dir();
    GrepTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "hello"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("hello.js") != std::string::npos);
    REQUIRE(result.content.find("nested.py") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GrepTool uses explicit path") {
    auto dir = make_test_dir();
    GrepTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"pattern", "42"}, {"path", dir}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("world.js") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GrepTool no matches") {
    auto dir = make_test_dir();
    GrepTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "NONEXISTENT_PATTERN_XYZ"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("No matches found") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GrepTool invalid regex") {
    GrepTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"pattern", "[invalid"}}, ctx);
    REQUIRE(result.is_error);
}

TEST_CASE("GrepTool invalid directory") {
    GrepTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"pattern", "test"}, {"path", "/nonexistent_dir_xyz"}}, ctx);
    REQUIRE(result.is_error);
}

TEST_CASE("GrepTool includes line numbers") {
    auto dir = make_test_dir();
    GrepTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "console"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    // Output should be filepath:line:content
    REQUIRE(result.content.find(":2:") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GrepTool includes match summary") {
    auto dir = make_test_dir();
    GrepTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "return"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("matches in") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GrepTool definition") {
    GrepTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Grep");
    REQUIRE(def.is_read_only);
    REQUIRE_FALSE(def.aliases.empty());
}

TEST_CASE("GrepTool validate_input") {
    GrepTool tool;
    ToolContext ctx;
    REQUIRE_FALSE(tool.validate_input({}, ctx).empty());
    REQUIRE_FALSE(tool.validate_input({{"pattern", ""}}, ctx).empty());
    REQUIRE(tool.validate_input({{"pattern", "test"}}, ctx).empty());
}
