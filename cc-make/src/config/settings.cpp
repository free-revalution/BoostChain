#include "config/settings.hpp"

namespace ccmake {

RuntimeSettings RuntimeSettings::build(const std::filesystem::path& working_dir) {
    RuntimeSettings settings;
    settings.working_directory = std::filesystem::absolute(working_dir).string();

    // Load global config from ~/.claude/settings.json
    auto global_cfg_path = get_global_config_dir() / "settings.json";
    auto global_cfg = Config::load_from_file(global_cfg_path);
    if (global_cfg) {
        settings.global_config = std::move(*global_cfg);
    }

    // Load project config
    settings.project_config = ProjectConfig::load(working_dir);

    // Discover CLAUDE.md
    settings.claude_md = discover_claude_md(working_dir);

    // Resolve effective model: project > global > default
    // For now, use global config's model if set
    settings.effective_model = settings.global_config.model;

    // Resolve effective permission mode
    settings.effective_permission_mode = settings.global_config.permission_mode;

    return settings;
}

}  // namespace ccmake
