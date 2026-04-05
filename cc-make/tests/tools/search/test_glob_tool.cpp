#include <catch2/catch_test_macros.hpp>
#include "tools/search/glob_tool.hpp"
#include <filesystem>
#include <fstream>
using namespace ccmake;

static std::string make_test_dir() {
    static int counter = 0;
    auto dir = "/tmp/ccmake_glob_" + std::to_string(counter++);
    std::filesystem::create_directories(dir);
    std::ofstream(dir + "/hello.js") << "a";
    std::ofstream(dir + "/world.js") << "b";
    std::ofstream(dir + "/readme.txt") << "c";
    std::filesystem::create_directories(dir + "/sub");
    std::ofstream(dir + "/sub/nested.py") << "d";
    return dir;
}

TEST_CASE("GlobTool matches *.js pattern") {
    auto dir = make_test_dir();
    GlobTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "*.js"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("hello.js") != std::string::npos);
    REQUIRE(result.content.find("world.js") != std::string::npos);
    REQUIRE(result.content.find("readme.txt") == std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GlobTool matches recursive pattern") {
    auto dir = make_test_dir();
    GlobTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "**/*.py"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("nested.py") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GlobTool uses explicit path") {
    auto dir = make_test_dir();
    GlobTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"pattern", "*.txt"}, {"path", dir}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("readme.txt") != std::string::npos);
    REQUIRE(result.content.find("hello.js") == std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GlobTool no matches") {
    auto dir = make_test_dir();
    GlobTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "*.md"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("No files matched") != std::string::npos);
    std::filesystem::remove_all(dir);
}

TEST_CASE("GlobTool invalid directory") {
    GlobTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"pattern", "*"}, {"path", "/nonexistent_dir_xyz"}}, ctx);
    REQUIRE(result.is_error);
}

TEST_CASE("GlobTool definition") {
    GlobTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Glob");
    REQUIRE(def.is_read_only);
    REQUIRE_FALSE(def.aliases.empty());
    REQUIRE_FALSE(def.aliases[0].empty());
}

TEST_CASE("GlobTool validate_input") {
    GlobTool tool;
    ToolContext ctx;
    REQUIRE_FALSE(tool.validate_input({}, ctx).empty());
    REQUIRE_FALSE(tool.validate_input({{"pattern", ""}}, ctx).empty());
    REQUIRE(tool.validate_input({{"pattern", "*.js"}}, ctx).empty());
}

TEST_CASE("GlobTool skips hidden files") {
    auto dir = make_test_dir();
    std::ofstream(dir + "/.hidden") << "x";
    std::ofstream(dir + "/.config/secret") << "y";
    std::filesystem::create_directories(dir + "/.config");

    GlobTool tool;
    ToolContext ctx;
    ctx.cwd = dir;
    auto result = tool.execute({{"pattern", "*"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find(".hidden") == std::string::npos);
    REQUIRE(result.content.find("secret") == std::string::npos);
    std::filesystem::remove_all(dir);
}
