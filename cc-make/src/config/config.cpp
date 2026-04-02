#include "config/config.hpp"
#include "core/json_utils.hpp"
#include "core/string_utils.hpp"

#include <nlohmann/json.hpp>

#include <fstream>
#include <sstream>
#include <cstdlib>

#ifdef _WIN32
#include <windows.h>
#endif

namespace ccmake {

// ---------------------------------------------------------------------------
// parse_permission_mode
// ---------------------------------------------------------------------------

PermissionMode parse_permission_mode(const std::string& s) {
    if (s == "default")             return PermissionMode::Default;
    if (s == "acceptEdits")         return PermissionMode::AcceptEdits;
    if (s == "plan")                return PermissionMode::Plan;
    if (s == "bypassPermissions")   return PermissionMode::BypassPermissions;
    if (s == "auto")                return PermissionMode::Auto;
    return PermissionMode::Default;
}

// ---------------------------------------------------------------------------
// PermissionRule JSON helpers
// ---------------------------------------------------------------------------

static PermissionRule parse_permission_rule(const nlohmann::json& j) {
    PermissionRule rule;
    rule.tool_pattern = json_get_or<std::string>(j, "tool_pattern", "");
    std::string behavior_str = json_get_or<std::string>(j, "behavior", "Ask");
    if (behavior_str == "Allow")          rule.behavior = PermissionBehavior::Allow;
    else if (behavior_str == "Deny")      rule.behavior = PermissionBehavior::Deny;
    else if (behavior_str == "Passthrough") rule.behavior = PermissionBehavior::Passthrough;
    else                                   rule.behavior = PermissionBehavior::Ask;
    return rule;
}

static nlohmann::json permission_rule_to_json(const PermissionRule& rule) {
    nlohmann::json j;
    j["tool_pattern"] = rule.tool_pattern;
    switch (rule.behavior) {
        case PermissionBehavior::Allow:       j["behavior"] = "Allow"; break;
        case PermissionBehavior::Deny:        j["behavior"] = "Deny"; break;
        case PermissionBehavior::Passthrough: j["behavior"] = "Passthrough"; break;
        case PermissionBehavior::Ask:         j["behavior"] = "Ask"; break;
    }
    return j;
}

// ---------------------------------------------------------------------------
// Config
// ---------------------------------------------------------------------------

std::optional<Config> Config::load_from_file(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();

    auto json_result = json_safe_parse(content);
    if (!json_result.has_value()) {
        return std::nullopt;
    }

    const auto& j = json_result.value();
    Config cfg;

    cfg.model = json_get_or<std::string>(j, "model", "");
    cfg.theme = json_get_or<std::string>(j, "theme", "dark");
    cfg.api_key = json_get_or<std::string>(j, "apiKey", "");

    std::string perm_str = json_get_or<std::string>(j, "permissionMode", "default");
    cfg.permission_mode = parse_permission_mode(perm_str);

    cfg.auto_compact = json_get_or<bool>(j, "autoCompact", true);
    cfg.auto_compact_pct = json_get_or<int>(j, "autoCompactPct", 80);

    // Parse permission rules
    if (j.contains("permissions") && j["permissions"].is_object()) {
        const auto& perms = j["permissions"];

        if (perms.contains("allow") && perms["allow"].is_array()) {
            for (const auto& rule_j : perms["allow"]) {
                cfg.allow_rules.push_back(parse_permission_rule(rule_j));
            }
        }
        if (perms.contains("deny") && perms["deny"].is_array()) {
            for (const auto& rule_j : perms["deny"]) {
                cfg.deny_rules.push_back(parse_permission_rule(rule_j));
            }
        }
    }

    return cfg;
}

bool Config::save_to_file(const std::filesystem::path& path) const {
    nlohmann::json j;
    j["model"] = model;
    j["theme"] = theme;
    j["permissionMode"] = "default"; // Would need reverse mapping; simplified here
    j["autoCompact"] = auto_compact;
    j["autoCompactPct"] = auto_compact_pct;
    if (!api_key.empty()) {
        j["apiKey"] = api_key;
    }

    nlohmann::json perms;
    nlohmann::json allow_arr = nlohmann::json::array();
    for (const auto& rule : allow_rules) {
        allow_arr.push_back(permission_rule_to_json(rule));
    }
    nlohmann::json deny_arr = nlohmann::json::array();
    for (const auto& rule : deny_rules) {
        deny_arr.push_back(permission_rule_to_json(rule));
    }
    perms["allow"] = allow_arr;
    perms["deny"] = deny_arr;
    j["permissions"] = perms;

    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;
    ofs << j.dump(2);
    return true;
}

// ---------------------------------------------------------------------------
// ProjectConfig
// ---------------------------------------------------------------------------

std::filesystem::path ProjectConfig::config_path(const std::filesystem::path& project_dir) {
    return project_dir / ".claude" / "settings.json";
}

std::optional<ProjectConfig> ProjectConfig::load(const std::filesystem::path& project_dir) {
    auto path = config_path(project_dir);
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        return std::nullopt;
    }

    std::ostringstream oss;
    oss << ifs.rdbuf();
    std::string content = oss.str();

    auto json_result = json_safe_parse(content);
    if (!json_result.has_value()) {
        return std::nullopt;
    }

    const auto& j = json_result.value();
    ProjectConfig cfg;

    if (j.contains("allowedTools") && j["allowedTools"].is_array()) {
        for (const auto& tool : j["allowedTools"]) {
            if (tool.is_string()) {
                cfg.allowed_tools.push_back(tool.get<std::string>());
            }
        }
    }

    cfg.mcp_config_uri = json_get_or<std::string>(j, "mcpConfigUri", "");

    return cfg;
}

bool ProjectConfig::save(const std::filesystem::path& project_dir) const {
    auto path = config_path(project_dir);
    auto parent = path.parent_path();

    // Ensure the .claude directory exists
    std::filesystem::create_directories(parent);

    nlohmann::json j;
    nlohmann::json tools = nlohmann::json::array();
    for (const auto& tool : allowed_tools) {
        tools.push_back(tool);
    }
    j["allowedTools"] = tools;

    if (!mcp_config_uri.empty()) {
        j["mcpConfigUri"] = mcp_config_uri;
    }

    std::ofstream ofs(path);
    if (!ofs.is_open()) return false;
    ofs << j.dump(2);
    return true;
}

// ---------------------------------------------------------------------------
// discover_claude_md
// ---------------------------------------------------------------------------

std::optional<ClaudeMdContent> discover_claude_md(const std::filesystem::path& start_dir) {
    std::filesystem::path dir = std::filesystem::absolute(start_dir);

    // Walk up the directory tree
    while (true) {
        auto candidate = dir / "CLAUDE.md";
        if (std::filesystem::exists(candidate)) {
            std::ifstream ifs(candidate);
            if (ifs.is_open()) {
                std::ostringstream oss;
                oss << ifs.rdbuf();
                ClaudeMdContent result;
                result.file_path = candidate;
                result.text = oss.str();
                return result;
            }
        }

        auto parent = dir.parent_path();
        // Stop if we've reached the root (parent == dir)
        if (parent == dir) break;
        dir = parent;
    }

    return std::nullopt;
}

// ---------------------------------------------------------------------------
// get_home_dir / get_global_config_dir
// ---------------------------------------------------------------------------

std::filesystem::path get_home_dir() {
#ifdef _WIN32
    const char* home = std::getenv("USERPROFILE");
    if (!home) home = std::getenv("HOMEDRIVE");
    if (!home) return {};
#else
    const char* home = std::getenv("HOME");
    if (!home) return {};
#endif
    return std::filesystem::path(home);
}

std::filesystem::path get_global_config_dir() {
    return get_home_dir() / ".claude";
}

}  // namespace ccmake
