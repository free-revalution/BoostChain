#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <fstream>
#include <filesystem>

#include "config/config.hpp"

using namespace ccmake;

// Helper: write a string to a temp file and return its path
static std::filesystem::path write_temp_file(const std::string& content) {
    auto dir = std::filesystem::temp_directory_path() / "ccmake_test";
    std::filesystem::create_directories(dir);
    auto path = dir / "test_config.json";
    std::ofstream ofs(path);
    ofs << content;
    ofs.close();
    return path;
}

// Helper: create a temporary directory
static std::filesystem::path create_temp_dir(const std::string& name) {
    auto dir = std::filesystem::temp_directory_path() / "ccmake_test" / name;
    std::filesystem::create_directories(dir);
    return dir;
}

// ---------------------------------------------------------------------------
// Test 1: Config loads from JSON file
// ---------------------------------------------------------------------------
TEST_CASE("Config loads from JSON file") {
    std::string json = R"({
        "model": "claude-sonnet-4-20250514",
        "permissionMode": "default",
        "theme": "dark"
    })";
    auto path = write_temp_file(json);

    auto cfg = Config::load_from_file(path);
    REQUIRE(cfg.has_value());
    REQUIRE(cfg->model == "claude-sonnet-4-20250514");
    REQUIRE(cfg->permission_mode == PermissionMode::Default);
    REQUIRE(cfg->theme == "dark");

    std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// Test 2: Config returns defaults for missing file
// ---------------------------------------------------------------------------
TEST_CASE("Config returns defaults for missing file") {
    auto cfg = Config::load_from_file("/nonexistent/path.json");
    REQUIRE_FALSE(cfg.has_value());
}

// ---------------------------------------------------------------------------
// Test 3: Config handles invalid JSON gracefully
// ---------------------------------------------------------------------------
TEST_CASE("Config handles invalid JSON gracefully") {
    auto path = write_temp_file("{invalid json");

    auto cfg = Config::load_from_file(path);
    REQUIRE_FALSE(cfg.has_value());

    std::filesystem::remove(path);
}

// ---------------------------------------------------------------------------
// Test 4: Config resolves permission mode strings
// ---------------------------------------------------------------------------
TEST_CASE("Config resolves permission mode strings") {
    REQUIRE(parse_permission_mode("default") == PermissionMode::Default);
    REQUIRE(parse_permission_mode("acceptEdits") == PermissionMode::AcceptEdits);
    REQUIRE(parse_permission_mode("plan") == PermissionMode::Plan);
    REQUIRE(parse_permission_mode("bypassPermissions") == PermissionMode::BypassPermissions);
    REQUIRE(parse_permission_mode("auto") == PermissionMode::Auto);
    REQUIRE(parse_permission_mode("unknownMode") == PermissionMode::Default);
    REQUIRE(parse_permission_mode("") == PermissionMode::Default);
}

// ---------------------------------------------------------------------------
// Test 5: ProjectConfig manages project settings
// ---------------------------------------------------------------------------
TEST_CASE("ProjectConfig manages project settings") {
    auto tmp_dir = create_temp_dir("project_settings_test");

    // Create a ProjectConfig, save it
    ProjectConfig orig;
    orig.allowed_tools = {"Bash", "Read", "Write"};
    orig.mcp_config_uri = "file:///path/to/mcp.json";
    REQUIRE(orig.save(tmp_dir));

    // Reload it
    auto loaded = ProjectConfig::load(tmp_dir);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->allowed_tools.size() == 3);
    REQUIRE(loaded->allowed_tools[0] == "Bash");
    REQUIRE(loaded->allowed_tools[1] == "Read");
    REQUIRE(loaded->allowed_tools[2] == "Write");
    REQUIRE(loaded->mcp_config_uri == "file:///path/to/mcp.json");

    // Clean up
    std::filesystem::remove_all(tmp_dir);
}

// ---------------------------------------------------------------------------
// Test 6: CLAUDE.md discovery finds files walking up
// ---------------------------------------------------------------------------
TEST_CASE("CLAUDE.md discovery finds files walking up") {
    auto tmp_dir = create_temp_dir("claude_md_test");

    // Create directory structure: tmp_dir/sub/deep
    auto deep_dir = tmp_dir / "sub" / "deep";
    std::filesystem::create_directories(deep_dir);

    // Write CLAUDE.md in tmp_dir (the "root")
    auto claude_md_path = tmp_dir / "CLAUDE.md";
    std::ofstream ofs(claude_md_path);
    ofs << "This is the CLAUDE.md content for the project.";
    ofs.close();

    // Discover from deep_dir -- should walk up and find it
    auto result = discover_claude_md(deep_dir);
    REQUIRE(result.has_value());
    REQUIRE_THAT(result->text, Catch::Matchers::ContainsSubstring("CLAUDE.md content for the project"));

    // Clean up
    std::filesystem::remove_all(tmp_dir);
}
