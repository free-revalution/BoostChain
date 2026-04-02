#pragma once

#include "config/config.hpp"
#include <memory>
#include <filesystem>

namespace ccmake {

struct RuntimeSettings {
    Config global_config;
    std::optional<ProjectConfig> project_config;
    std::optional<ClaudeMdContent> claude_md;
    std::string effective_model;
    PermissionMode effective_permission_mode;
    std::string working_directory;

    static RuntimeSettings build(const std::filesystem::path& working_dir);
};

}  // namespace ccmake
