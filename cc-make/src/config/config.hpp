#pragma once

#include "core/types.hpp"
#include "core/result.hpp"

#include <string>
#include <vector>
#include <optional>
#include <filesystem>

namespace ccmake {

struct PermissionRule {
    std::string tool_pattern;       // e.g., "Bash", "Bash(git *)", "mcp__server"
    PermissionBehavior behavior;
};

struct Config {
    std::string model;
    PermissionMode permission_mode = PermissionMode::Default;
    std::string theme = "dark";
    std::string api_key;
    std::vector<PermissionRule> allow_rules;
    std::vector<PermissionRule> deny_rules;
    bool auto_compact = true;
    int auto_compact_pct = 80;

    static std::optional<Config> load_from_file(const std::filesystem::path& path);
    bool save_to_file(const std::filesystem::path& path) const;
};

struct ProjectConfig {
    std::vector<std::string> allowed_tools;
    std::string mcp_config_uri;

    static std::optional<ProjectConfig> load(const std::filesystem::path& project_dir);
    bool save(const std::filesystem::path& project_dir) const;

private:
    static std::filesystem::path config_path(const std::filesystem::path& project_dir);
};

PermissionMode parse_permission_mode(const std::string& s);

struct ClaudeMdContent {
    std::filesystem::path file_path;
    std::string text;
};

std::optional<ClaudeMdContent> discover_claude_md(const std::filesystem::path& start_dir);

std::filesystem::path get_global_config_dir();
std::filesystem::path get_home_dir();

}  // namespace ccmake
