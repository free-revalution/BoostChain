#include <catch2/catch_test_macros.hpp>
#include "query/context.hpp"
using namespace ccmake;

TEST_CASE("build_system_prompt with custom prompt") {
    QueryConfig config;
    config.system_prompt = "You are helpful.";
    auto prompt = build_system_prompt(config, "", "");
    REQUIRE(prompt == "You are helpful.\n");
}

TEST_CASE("build_system_prompt with user context") {
    QueryConfig config;
    config.system_prompt = "You are helpful.";
    auto prompt = build_system_prompt(config, "# Current Directory\n/tmp", "No git info");
    REQUIRE(prompt.find("You are helpful.") != std::string::npos);
    REQUIRE(prompt.find("/tmp") != std::string::npos);
}

TEST_CASE("build_system_prompt without custom prompt uses default") {
    QueryConfig config;
    auto prompt = build_system_prompt(config, "Date: 2026-04-04", "");
    REQUIRE(!prompt.empty());
    REQUIRE(prompt.find("Claude Code") != std::string::npos);
}

TEST_CASE("build_system_prompt with system context (git info)") {
    QueryConfig config;
    config.system_prompt = "Help.";
    auto prompt = build_system_prompt(config, "", "Branch: main\nStatus: clean");
    REQUIRE(prompt.find("Branch: main") != std::string::npos);
}

TEST_CASE("build_system_prompt combines all parts") {
    QueryConfig config;
    config.system_prompt = "Base prompt.";
    auto prompt = build_system_prompt(config, "# Files\nREADME.md", "Branch: main");
    REQUIRE(prompt.find("Base prompt.") != std::string::npos);
    REQUIRE(prompt.find("README.md") != std::string::npos);
    REQUIRE(prompt.find("Branch: main") != std::string::npos);
}

TEST_CASE("build_user_context from git repo") {
    auto ctx = build_user_context("/Users/jiang/development/BoostChain/cc-make");
    REQUIRE(!ctx.empty());
    REQUIRE(ctx.find("2026-04") != std::string::npos);  // Date is present
}

TEST_CASE("build_system_context from git repo") {
    auto ctx = build_system_context("/Users/jiang/development/BoostChain/cc-make");
    REQUIRE(!ctx.empty());
    REQUIRE(ctx.find("main") != std::string::npos);
}

TEST_CASE("build_user_context for non-git directory") {
    auto ctx = build_user_context("/tmp");
    // Should have date but no git branch
    REQUIRE(ctx.find("2026-04") != std::string::npos);
}

TEST_CASE("format_git_status formats correctly") {
    QueryGitStatus status;
    status.branch = "main";
    status.is_git = true;
    status.status = "M src/file.cpp";
    auto formatted = format_git_status(status);
    REQUIRE(formatted.find("main") != std::string::npos);
    REQUIRE(formatted.find("src/file.cpp") != std::string::npos);
}

TEST_CASE("get_git_status returns values for cc-make") {
    auto status = get_git_status("/Users/jiang/development/BoostChain/cc-make");
    REQUIRE(status.is_git);
    REQUIRE(!status.branch.empty());
}

TEST_CASE("format_git_status returns empty for non-git") {
    QueryGitStatus status;
    status.is_git = false;
    REQUIRE(format_git_status(status).empty());
}

TEST_CASE("build_system_prompt empty context strings") {
    QueryConfig config;
    config.system_prompt = "Only prompt.";
    auto prompt = build_system_prompt(config, "", "");
    REQUIRE(prompt == "Only prompt.\n");
}
