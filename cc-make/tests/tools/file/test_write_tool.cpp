#include <catch2/catch_test_macros.hpp>
#include "tools/file/write_tool.hpp"
#include <fstream>
#include <filesystem>
using namespace ccmake;

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    return std::string(std::istreambuf_iterator<char>(f), {});
}

TEST_CASE("WriteTool creates new file") {
    static int counter = 0;
    auto path = "/tmp/ccmake_test_write_" + std::to_string(counter++) + ".txt";
    WriteTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"content", "hello\nworld\n"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("Created") != std::string::npos);
    REQUIRE(read_file(path) == "hello\nworld\n");
    std::filesystem::remove(path);
}

TEST_CASE("WriteTool overwrites existing file") {
    static int counter = 0;
    auto path = "/tmp/ccmake_test_write_" + std::to_string(counter++) + ".txt";
    { std::ofstream f(path); f << "old content"; }

    WriteTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"content", "new content"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("Updated") != std::string::npos);
    REQUIRE(read_file(path) == "new content");
    std::filesystem::remove(path);
}

TEST_CASE("WriteTool creates parent directories") {
    static int counter = 0;
    auto path = "/tmp/ccmake_test_write_" + std::to_string(counter++) + "/subdir/file.txt";
    WriteTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"content", "nested"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(read_file(path) == "nested");
    std::filesystem::remove(path);
    std::filesystem::remove(std::filesystem::path(path).parent_path());
}

TEST_CASE("WriteTool validate_input rejects missing file_path") {
    WriteTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"content", "x"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("WriteTool validate_input rejects missing content") {
    WriteTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"file_path", "x"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("WriteTool validate_input accepts valid input") {
    WriteTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"file_path", "x"}, {"content", "y"}}, ctx);
    REQUIRE(err.empty());
}

TEST_CASE("WriteTool definition is correct") {
    WriteTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Write");
    REQUIRE_FALSE(def.is_destructive);
}
