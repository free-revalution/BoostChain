#include <catch2/catch_test_macros.hpp>
#include "config/config_loader.hpp"
#include "config/settings.hpp"
#include "query/query_engine.hpp"
#include "tools/file/read_tool.hpp"

#include <fstream>
#include <filesystem>
#include <sstream>

using namespace ccmake;

static std::string make_temp_path() {
    static int counter = 0;
    return "/tmp/ccmake_cfgloader_" + std::to_string(counter++);
}

// ============================================================
// merge_configs tests
// ============================================================

TEST_CASE("merge_configs: project overrides global model") {
    Config global;
    global.model = "global-model";
    global.permission_mode = PermissionMode::Default;

    Config project;
    project.model = "project-model";

    auto merged = merge_configs(global, project);
    REQUIRE(merged.model == "project-model");
    REQUIRE(merged.permission_mode == PermissionMode::Default);  // not overridden
}

TEST_CASE("merge_configs: project overrides permission mode") {
    Config global;
    global.permission_mode = PermissionMode::Default;

    Config project;
    project.permission_mode = PermissionMode::BypassPermissions;

    auto merged = merge_configs(global, project);
    REQUIRE(merged.permission_mode == PermissionMode::BypassPermissions);
}

TEST_CASE("merge_configs: project rules added after global") {
    Config global;
    global.allow_rules.push_back({"Read", PermissionBehavior::Allow});
    global.deny_rules.push_back({"Dangerous", PermissionBehavior::Deny});

    Config project;
    project.deny_rules.push_back({"Bash", PermissionBehavior::Deny});

    auto merged = merge_configs(global, project);
    REQUIRE(merged.allow_rules.size() == 1);
    REQUIRE(merged.deny_rules.size() == 2);
    REQUIRE(merged.deny_rules[0].tool_pattern == "Dangerous");
    REQUIRE(merged.deny_rules[1].tool_pattern == "Bash");
}

TEST_CASE("merge_configs: empty project returns global") {
    Config global;
    global.model = "global-model";
    global.theme = "dark";

    Config project;  // all defaults

    auto merged = merge_configs(global, project);
    REQUIRE(merged.model == "global-model");
}

// ============================================================
// YAML config tests
// ============================================================

TEST_CASE("load_yaml_config: loads basic config") {
    auto path = make_temp_path() + ".yaml";
    std::ofstream ofs(path);
    ofs << "model: claude-opus-4-20250514\n"
        << "permissionMode: bypassPermissions\n"
        << "theme: light\n"
        << "autoCompact: false\n"
        << "autoCompactPct: 90\n";
    ofs.close();

    auto cfg = load_yaml_config(path);
    REQUIRE(cfg.has_value());
    REQUIRE(cfg->model == "claude-opus-4-20250514");
    REQUIRE(cfg->permission_mode == PermissionMode::BypassPermissions);
    REQUIRE(cfg->theme == "light");
    REQUIRE_FALSE(cfg->auto_compact);
    REQUIRE(cfg->auto_compact_pct == 90);

    std::filesystem::remove(path);
}

TEST_CASE("load_yaml_config: loads permission rules") {
    auto path = make_temp_path() + ".yaml";
    std::ofstream ofs(path);
    ofs << "permissions:\n"
        << "  allow:\n"
        << "    - tool_pattern: Read\n"
        << "      behavior: Allow\n"
        << "    - tool_pattern: Write\n"
        << "      behavior: Allow\n"
        << "  deny:\n"
        << "    - tool_pattern: Bash\n"
        << "      behavior: Deny\n";
    ofs.close();

    auto cfg = load_yaml_config(path);
    REQUIRE(cfg.has_value());
    REQUIRE(cfg->allow_rules.size() == 2);
    REQUIRE(cfg->allow_rules[0].tool_pattern == "Read");
    REQUIRE(cfg->allow_rules[1].tool_pattern == "Write");
    REQUIRE(cfg->deny_rules.size() == 1);
    REQUIRE(cfg->deny_rules[0].tool_pattern == "Bash");
    REQUIRE(cfg->deny_rules[0].behavior == PermissionBehavior::Deny);

    std::filesystem::remove(path);
}

TEST_CASE("load_yaml_config: returns nullopt for missing file") {
    auto cfg = load_yaml_config("/nonexistent/path.yaml");
    REQUIRE_FALSE(cfg.has_value());
}

TEST_CASE("load_yaml_config: returns nullopt for invalid YAML") {
    auto path = make_temp_path() + ".yaml";
    std::ofstream ofs(path);
    ofs << "invalid: [\n";
    ofs.close();

    auto cfg = load_yaml_config(path);
    REQUIRE_FALSE(cfg.has_value());

    std::filesystem::remove(path);
}

TEST_CASE("save_yaml_config: round-trip") {
    auto path = make_temp_path() + ".yaml";

    Config cfg;
    cfg.model = "test-model";
    cfg.permission_mode = PermissionMode::AcceptEdits;
    cfg.theme = "dark";
    cfg.auto_compact = false;
    cfg.auto_compact_pct = 75;
    cfg.allow_rules.push_back({"Read", PermissionBehavior::Allow});
    cfg.deny_rules.push_back({"Bash", PermissionBehavior::Deny});

    REQUIRE(save_yaml_config(cfg, path));
    REQUIRE(std::filesystem::exists(path));

    auto loaded = load_yaml_config(path);
    REQUIRE(loaded.has_value());
    REQUIRE(loaded->model == "test-model");
    REQUIRE(loaded->permission_mode == PermissionMode::AcceptEdits);
    REQUIRE_FALSE(loaded->auto_compact);
    REQUIRE(loaded->auto_compact_pct == 75);
    REQUIRE(loaded->allow_rules.size() == 1);
    REQUIRE(loaded->deny_rules.size() == 1);

    std::filesystem::remove(path);
}

// ============================================================
// apply_config_to_permissions tests
// ============================================================

TEST_CASE("apply_config_to_permissions: sets mode and rules") {
    PermissionManager pm;
    Config cfg;
    cfg.permission_mode = PermissionMode::BypassPermissions;
    cfg.allow_rules.push_back({"Read", PermissionBehavior::Allow});
    cfg.deny_rules.push_back({"Bash", PermissionBehavior::Deny});

    apply_config_to_permissions(cfg, pm);

    REQUIRE(pm.mode() == PermissionMode::BypassPermissions);

    auto r1 = pm.check("Read");
    REQUIRE(r1.behavior == PermissionBehavior::Allow);

    auto r2 = pm.check("Bash");
    REQUIRE(r2.behavior == PermissionBehavior::Deny);
}

// ============================================================
// apply_config_to_engine tests
// ============================================================

TEST_CASE("apply_config_to_engine: sets model and permissions") {
    QueryEngine engine("default-model");
    Config cfg;
    cfg.model = "custom-model";
    cfg.permission_mode = PermissionMode::AcceptEdits;
    cfg.allow_rules.push_back({"Read", PermissionBehavior::Allow});

    apply_config_to_engine(cfg, engine);

    REQUIRE(engine.model() == "custom-model");
    REQUIRE(engine.permission_manager().mode() == PermissionMode::AcceptEdits);
}

// ============================================================
// build_runtime_settings tests
// ============================================================

TEST_CASE("build_runtime_settings: works with current directory") {
    auto settings = build_runtime_settings("/Users/jiang/development/BoostChain/cc-make");
    REQUIRE_FALSE(settings.working_directory.empty());
    // Should find git repo and CLAUDE.md discovery may succeed
}

TEST_CASE("build_runtime_settings: uses defaults when no config files") {
    auto tmp = make_temp_path();
    std::filesystem::create_directories(tmp);

    auto settings = build_runtime_settings(tmp);
    REQUIRE(settings.working_directory == std::filesystem::absolute(tmp).string());

    std::filesystem::remove_all(tmp);
}
