#include <catch2/catch_test_macros.hpp>

#include "tools/worktree/worktree_tool.hpp"
#include "git/git.hpp"
#include "utils/process.hpp"

#include <filesystem>

using namespace ccmake;

// Helper: create a temporary git repo for integration tests
static std::filesystem::path make_tmp_repo() {
    auto tmp = std::filesystem::temp_directory_path() / "ccmake-test-worktree-XXXXXX";
    std::filesystem::create_directories(tmp);
    run_command("git init", tmp.string());
    run_command("git config user.email 'test@test.com'", tmp.string());
    run_command("git config user.name 'Test'", tmp.string());
    run_command("git commit --allow-empty -m 'init'", tmp.string());
    return tmp;
}

// ============================================================
// Definition tests
// ============================================================

TEST_CASE("WorktreeTool definition") {
    WorktreeTool tool;
    auto& def = tool.definition();
    REQUIRE(def.name == "Worktree");
    REQUIRE_FALSE(def.description.empty());
    REQUIRE(def.input_schema.contains("properties"));
    REQUIRE(def.input_schema["properties"].contains("action"));
}

TEST_CASE("WorktreeTool definition properties") {
    WorktreeTool tool;
    auto& def = tool.definition();
    REQUIRE_FALSE(def.is_read_only);
    REQUIRE(def.is_destructive);
    REQUIRE_FALSE(def.is_concurrency_safe);
}

TEST_CASE("WorktreeTool aliases") {
    WorktreeTool tool;
    auto& def = tool.definition();
    REQUIRE_FALSE(def.aliases.empty());
    REQUIRE(def.aliases.at(0) == "GitWorktree");
}

// ============================================================
// Validation tests
// ============================================================

TEST_CASE("WorktreeTool validate_input valid list") {
    WorktreeTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"action", "list"}}, ctx);
    REQUIRE(err.empty());
}

TEST_CASE("WorktreeTool validate_input missing action") {
    WorktreeTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("WorktreeTool validate_input invalid action") {
    WorktreeTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"action", "foo"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("WorktreeTool validate_input add missing path") {
    WorktreeTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"action", "add"}, {"branch", "feature"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("WorktreeTool validate_input add missing branch") {
    WorktreeTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"action", "add"}, {"path", "/tmp/wt"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

TEST_CASE("WorktreeTool validate_input remove missing path") {
    WorktreeTool tool;
    ToolContext ctx;
    auto err = tool.validate_input({{"action", "remove"}}, ctx);
    REQUIRE_FALSE(err.empty());
}

// ============================================================
// Execute tests (unit-level, no git required)
// ============================================================

TEST_CASE("WorktreeTool execute unknown action") {
    WorktreeTool tool;
    ToolContext ctx;
    auto result = tool.execute({{"action", "invalid"}}, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("unknown action") != std::string::npos);
}

TEST_CASE("WorktreeTool execute missing action") {
    WorktreeTool tool;
    ToolContext ctx;
    auto result = tool.execute({}, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("action is required") != std::string::npos);
}

// ============================================================
// Integration tests (real git operations in temp repo)
// ============================================================

TEST_CASE("WorktreeTool execute list in fresh repo") {
    auto tmp = make_tmp_repo();
    WorktreeTool tool;
    ToolContext ctx;
    ctx.cwd = tmp.string();
    auto result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE_FALSE(result.is_error);
    // A fresh repo should have at least the main worktree
    REQUIRE_FALSE(result.content.empty());
    std::filesystem::remove_all(tmp);
}

TEST_CASE("WorktreeTool execute add and list") {
    auto tmp = make_tmp_repo();
    auto wt_path = tmp / "worktree-feature";

    WorktreeTool tool;
    ToolContext ctx;
    ctx.cwd = tmp.string();

    // Add a worktree
    auto add_result = tool.execute({{"action", "add"}, {"path", wt_path.string()}, {"branch", "feature-branch"}}, ctx);
    REQUIRE_FALSE(add_result.is_error);
    REQUIRE(add_result.content.find("feature-branch") != std::string::npos);

    // List should show both worktrees
    auto list_result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE_FALSE(list_result.is_error);
    REQUIRE(list_result.content.find("feature-branch") != std::string::npos);
    REQUIRE(list_result.content.find("main") != std::string::npos);

    std::filesystem::remove_all(tmp);
}

TEST_CASE("WorktreeTool execute add and remove") {
    auto tmp = make_tmp_repo();
    auto wt_path = tmp / "worktree-temp";

    WorktreeTool tool;
    ToolContext ctx;
    ctx.cwd = tmp.string();

    // Add a worktree
    auto add_result = tool.execute({{"action", "add"}, {"path", wt_path.string()}, {"branch", "temp-branch"}}, ctx);
    REQUIRE_FALSE(add_result.is_error);

    // Remove it
    auto remove_result = tool.execute({{"action", "remove"}, {"path", wt_path.string()}}, ctx);
    REQUIRE_FALSE(remove_result.is_error);
    REQUIRE(remove_result.content.find("removed") != std::string::npos);

    // List should no longer show the removed worktree
    auto list_result = tool.execute({{"action", "list"}}, ctx);
    REQUIRE_FALSE(list_result.is_error);
    REQUIRE(list_result.content.find("temp-branch") == std::string::npos);

    std::filesystem::remove_all(tmp);
}

TEST_CASE("WorktreeTool execute remove nonexistent") {
    auto tmp = make_tmp_repo();

    WorktreeTool tool;
    ToolContext ctx;
    ctx.cwd = tmp.string();

    auto result = tool.execute({{"action", "remove"}, {"path", "/tmp/nonexistent-worktree-xyz"}}, ctx);
    REQUIRE(result.is_error);

    std::filesystem::remove_all(tmp);
}

TEST_CASE("WorktreeTool execute add missing path in execute") {
    auto tmp = make_tmp_repo();

    WorktreeTool tool;
    ToolContext ctx;
    ctx.cwd = tmp.string();

    auto result = tool.execute({{"action", "add"}, {"branch", "test"}}, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("path is required") != std::string::npos);

    std::filesystem::remove_all(tmp);
}

TEST_CASE("WorktreeTool execute add missing branch in execute") {
    auto tmp = make_tmp_repo();

    WorktreeTool tool;
    ToolContext ctx;
    ctx.cwd = tmp.string();

    auto result = tool.execute({{"action", "add"}, {"path", "/tmp/some-wt"}}, ctx);
    REQUIRE(result.is_error);
    REQUIRE(result.content.find("branch is required") != std::string::npos);

    std::filesystem::remove_all(tmp);
}
