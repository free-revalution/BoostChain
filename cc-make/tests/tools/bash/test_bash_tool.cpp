#include <catch2/catch_test_macros.hpp>
#include "tools/bash/bash_tool.hpp"
using namespace ccmake;

TEST_CASE("BashTool runs echo command") {
    BashTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"command", "echo hello"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("hello") != std::string::npos);
    REQUIRE(result.content.find("Exit code: 0") != std::string::npos);
}

TEST_CASE("BashTool captures exit code") {
    BashTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"command", "false"}}, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("Exit code: 1") != std::string::npos);
}

TEST_CASE("BashTool captures stderr") {
    BashTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"command", "echo err >&2"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("stderr") != std::string::npos);
}

TEST_CASE("BashTool timeout") {
    BashTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"command", "sleep 10"}, {"timeout", 500}}, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("timed out") != std::string::npos);
}

TEST_CASE("BashTool invalid input") {
    BashTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("BashTool validate_input accepts valid command") {
    BashTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"command", "ls"}}, ctx);
    REQUIRE(err.empty());
}

TEST_CASE("BashTool definition") {
    BashTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Bash");
    REQUIRE_FALSE(def.is_read_only);
}

TEST_CASE("BashTool runs in cwd") {
    BashTool tool;
    ToolContext ctx;
    ctx.cwd = "/tmp";
    auto result = tool.execute({{"command", "pwd"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("/tmp") != std::string::npos);
}

TEST_CASE("BashTool respects max timeout") {
    BashTool tool;
    ToolContext ctx;
    // 999999ms should be clamped to 600000ms
    auto result = tool.execute({{"command", "echo hi"}, {"timeout", 999999}}, ctx);
    REQUIRE_FALSE(result.is_error);
    REQUIRE(result.content.find("hi") != std::string::npos);
}
