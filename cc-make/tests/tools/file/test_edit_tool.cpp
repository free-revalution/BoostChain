#include <catch2/catch_test_macros.hpp>
#include "tools/file/edit_tool.hpp"
#include <fstream>
#include <filesystem>
using namespace ccmake;

static std::string make_test_file(const std::string& content) {
    static int counter = 0;
    auto path = "/tmp/ccmake_test_edit_" + std::to_string(counter++) + ".txt";
    std::ofstream f(path);
    f << content;
    f.close();
    return path;
}

static std::string read_file(const std::string& path) {
    std::ifstream f(path);
    return std::string(std::istreambuf_iterator<char>(f), {});
}

TEST_CASE("EditTool basic replacement") {
    auto path = make_test_file("hello world\n");
    EditTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"old_string", "world"}, {"new_string", "there"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(read_file(path) == "hello there\n");
    std::filesystem::remove(path);
}

TEST_CASE("EditTool replace_all") {
    auto path = make_test_file("aaa bbb aaa\n");
    EditTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"old_string", "aaa"}, {"new_string", "ccc"}, {"replace_all", true}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(read_file(path) == "ccc bbb ccc\n");
    std::filesystem::remove(path);
}

TEST_CASE("EditTool rejects same old and new string") {
    EditTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"file_path", "x"}, {"old_string", "same"}, {"new_string", "same"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("EditTool rejects empty old_string") {
    EditTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"file_path", "x"}, {"old_string", ""}, {"new_string", "new"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("EditTool rejects missing fields") {
    EditTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"file_path", "x"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("EditTool error for nonexistent file") {
    EditTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", "/nonexistent"}, {"old_string", "x"}, {"new_string", "y"}}, ctx);
    REQUIRE(result.is_error);
}

TEST_CASE("EditTool error when old_string not found") {
    auto path = make_test_file("hello\n");
    EditTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"old_string", "notfound"}, {"new_string", "x"}}, ctx);
    REQUIRE(result.is_error);
    std::filesystem::remove(path);
}

TEST_CASE("EditTool error for multiple matches without replace_all") {
    auto path = make_test_file("aa aa\n");
    EditTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"file_path", path}, {"old_string", "aa"}, {"new_string", "bb"}}, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("multiple") != std::string::npos);
    std::filesystem::remove(path);
}

TEST_CASE("EditTool definition is correct") {
    EditTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Edit");
    REQUIRE_FALSE(def.is_destructive);
}

TEST_CASE("EditTool validate_input accepts valid input") {
    EditTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"file_path", "x"}, {"old_string", "old"}, {"new_string", "new"}}, ctx);
    REQUIRE(err.empty());
}
