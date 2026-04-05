#include "config/config_loader.hpp"
#include "query/query_engine.hpp"

#include <yaml-cpp/yaml.h>
#include <fstream>

namespace ccmake {

// ---------------------------------------------------------------------------
// merge_configs
// ---------------------------------------------------------------------------

Config merge_configs(const Config& global, const Config& project) {
    Config merged = global;

    // Project overrides global where set
    if (!project.model.empty()) merged.model = project.model;
    if (project.permission_mode != PermissionMode::Default) {
        merged.permission_mode = project.permission_mode;
    }
    if (!project.theme.empty() && project.theme != "dark") merged.theme = project.theme;
    if (!project.api_key.empty()) merged.api_key = project.api_key;

    // Permission rules: project rules are added AFTER global rules
    // so they take priority in deny-first checking
    for (const auto& rule : project.allow_rules) {
        merged.allow_rules.push_back(rule);
    }
    for (const auto& rule : project.deny_rules) {
        merged.deny_rules.push_back(rule);
    }

    if (!project.auto_compact) merged.auto_compact = project.auto_compact;
    if (project.auto_compact_pct != 80) merged.auto_compact_pct = project.auto_compact_pct;

    return merged;
}

// ---------------------------------------------------------------------------
// YAML config
// ---------------------------------------------------------------------------

static PermissionBehavior parse_yaml_behavior(const std::string& s) {
    if (s == "Allow" || s == "allow")       return PermissionBehavior::Allow;
    if (s == "Deny" || s == "deny")          return PermissionBehavior::Deny;
    if (s == "Passthrough" || s == "passthrough") return PermissionBehavior::Passthrough;
    return PermissionBehavior::Ask;
}

std::optional<Config> load_yaml_config(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        return std::nullopt;
    }

    try {
        auto node = YAML::LoadFile(path.string());
        Config cfg;

        if (node["model"]) cfg.model = node["model"].as<std::string>();

        if (node["permissionMode"]) {
            cfg.permission_mode = parse_permission_mode(node["permissionMode"].as<std::string>());
        }

        if (node["theme"]) cfg.theme = node["theme"].as<std::string>();
        if (node["apiKey"]) cfg.api_key = node["apiKey"].as<std::string>();

        if (node["autoCompact"]) cfg.auto_compact = node["autoCompact"].as<bool>();
        if (node["autoCompactPct"]) cfg.auto_compact_pct = node["autoCompactPct"].as<int>();

        if (node["permissions"]) {
            const auto& perms = node["permissions"];
            if (perms["allow"] && perms["allow"].IsSequence()) {
                for (const auto& rule_node : perms["allow"]) {
                    PermissionRule rule;
                    if (rule_node["tool_pattern"]) rule.tool_pattern = rule_node["tool_pattern"].as<std::string>();
                    if (rule_node["behavior"]) rule.behavior = parse_yaml_behavior(rule_node["behavior"].as<std::string>());
                    cfg.allow_rules.push_back(std::move(rule));
                }
            }
            if (perms["deny"] && perms["deny"].IsSequence()) {
                for (const auto& rule_node : perms["deny"]) {
                    PermissionRule rule;
                    if (rule_node["tool_pattern"]) rule.tool_pattern = rule_node["tool_pattern"].as<std::string>();
                    if (rule_node["behavior"]) rule.behavior = parse_yaml_behavior(rule_node["behavior"].as<std::string>());
                    cfg.deny_rules.push_back(std::move(rule));
                }
            }
        }

        return cfg;
    } catch (const YAML::Exception&) {
        return std::nullopt;
    }
}

bool save_yaml_config(const Config& config, const std::filesystem::path& path) {
    try {
        YAML::Emitter out;
        out << YAML::BeginMap;

        if (!config.model.empty()) out << YAML::Key << "model" << YAML::Value << config.model;

        // Permission mode string
        std::string perm_str = "default";
        switch (config.permission_mode) {
            case PermissionMode::AcceptEdits: perm_str = "acceptEdits"; break;
            case PermissionMode::Plan: perm_str = "plan"; break;
            case PermissionMode::BypassPermissions: perm_str = "bypassPermissions"; break;
            case PermissionMode::Auto: perm_str = "auto"; break;
            default: break;
        }
        out << YAML::Key << "permissionMode" << YAML::Value << perm_str;
        out << YAML::Key << "theme" << YAML::Value << config.theme;
        out << YAML::Key << "autoCompact" << YAML::Value << config.auto_compact;
        out << YAML::Key << "autoCompactPct" << YAML::Value << config.auto_compact_pct;

        if (!config.api_key.empty()) {
            out << YAML::Key << "apiKey" << YAML::Value << config.api_key;
        }

        // Permissions
        if (!config.allow_rules.empty() || !config.deny_rules.empty()) {
            out << YAML::Key << "permissions" << YAML::Value << YAML::BeginMap;

            if (!config.allow_rules.empty()) {
                out << YAML::Key << "allow" << YAML::Value << YAML::BeginSeq;
                for (const auto& rule : config.allow_rules) {
                    out << YAML::BeginMap;
                    out << YAML::Key << "tool_pattern" << YAML::Value << rule.tool_pattern;
                    std::string behavior_str = "Ask";
                    switch (rule.behavior) {
                        case PermissionBehavior::Allow: behavior_str = "Allow"; break;
                        case PermissionBehavior::Deny: behavior_str = "Deny"; break;
                        case PermissionBehavior::Passthrough: behavior_str = "Passthrough"; break;
                        default: break;
                    }
                    out << YAML::Key << "behavior" << YAML::Value << behavior_str;
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }

            if (!config.deny_rules.empty()) {
                out << YAML::Key << "deny" << YAML::Value << YAML::BeginSeq;
                for (const auto& rule : config.deny_rules) {
                    out << YAML::BeginMap;
                    out << YAML::Key << "tool_pattern" << YAML::Value << rule.tool_pattern;
                    std::string behavior_str = "Ask";
                    switch (rule.behavior) {
                        case PermissionBehavior::Allow: behavior_str = "Allow"; break;
                        case PermissionBehavior::Deny: behavior_str = "Deny"; break;
                        case PermissionBehavior::Passthrough: behavior_str = "Passthrough"; break;
                        default: break;
                    }
                    out << YAML::Key << "behavior" << YAML::Value << behavior_str;
                    out << YAML::EndMap;
                }
                out << YAML::EndSeq;
            }

            out << YAML::EndMap;
        }

        out << YAML::EndMap;

        auto parent = path.parent_path();
        if (!parent.empty()) std::filesystem::create_directories(parent);

        std::ofstream ofs(path);
        if (!ofs.is_open()) return false;
        ofs << out.c_str();
        return true;
    } catch (...) {
        return false;
    }
}

// ---------------------------------------------------------------------------
// Apply config to PermissionManager
// ---------------------------------------------------------------------------

void apply_config_to_permissions(const Config& config, PermissionManager& pm) {
    pm.set_mode(config.permission_mode);

    // Clear existing rules and apply from config
    // Note: we add deny rules first so they have higher priority
    // (check_permission iterates deny first, then allow)
    for (const auto& rule : config.deny_rules) {
        pm.add_deny_rule(rule.tool_pattern, "config");
    }
    for (const auto& rule : config.allow_rules) {
        pm.add_allow_rule(rule.tool_pattern, "config");
    }
}

// ---------------------------------------------------------------------------
// Apply config to QueryEngine
// ---------------------------------------------------------------------------

void apply_config_to_engine(const Config& config, QueryEngine& engine) {
    if (!config.model.empty()) {
        engine.set_model(config.model);
    }

    apply_config_to_permissions(config, engine.permission_manager());
}

// ---------------------------------------------------------------------------
// Build runtime settings
// ---------------------------------------------------------------------------

RuntimeSettings build_runtime_settings(const std::filesystem::path& working_dir) {
    RuntimeSettings settings;
    settings.working_directory = std::filesystem::absolute(working_dir).string();

    // Load global config
    auto global_dir = get_global_config_dir();
    auto global_json = global_dir / "settings.json";
    auto global_yaml = global_dir / "config.yaml";

    Config global_cfg;
    if (auto cfg = Config::load_from_file(global_json)) {
        global_cfg = std::move(*cfg);
    }
    // YAML overrides JSON if both exist
    if (auto cfg = load_yaml_config(global_yaml)) {
        global_cfg = merge_configs(global_cfg, *cfg);
    }
    settings.global_config = std::move(global_cfg);

    // Load project config
    auto project_json = std::filesystem::path(working_dir) / ".claude" / "settings.json";
    auto project_yaml = std::filesystem::path(working_dir) / ".claude" / "config.yaml";

    Config project_cfg;
    if (auto cfg = Config::load_from_file(project_json)) {
        project_cfg = std::move(*cfg);
    }
    if (auto cfg = load_yaml_config(project_yaml)) {
        project_cfg = merge_configs(project_cfg, *cfg);
    }

    // Load legacy ProjectConfig
    auto legacy_project = ProjectConfig::load(working_dir);
    if (legacy_project) {
        settings.project_config = std::move(*legacy_project);
    }

    // Discover CLAUDE.md
    settings.claude_md = discover_claude_md(working_dir);

    // Merge: project overrides global
    auto merged = merge_configs(settings.global_config, project_cfg);

    // Resolve effective values
    settings.effective_model = merged.model;
    settings.effective_permission_mode = merged.permission_mode;

    return settings;
}

}  // namespace ccmake
