#pragma once

#include "config/config.hpp"
#include "config/settings.hpp"
#include "permissions/permission_manager.hpp"

#include <filesystem>
#include <optional>

namespace ccmake {

// Merge project config on top of global config.
// Project settings override global where set; global provides defaults.
Config merge_configs(const Config& global, const Config& project);

// Load YAML config file (returns nullopt if not found or parse error)
std::optional<Config> load_yaml_config(const std::filesystem::path& path);

// Save config as YAML
bool save_yaml_config(const Config& config, const std::filesystem::path& path);

// Apply a Config to a PermissionManager
void apply_config_to_permissions(const Config& config, PermissionManager& pm);

// Apply a Config to a QueryEngine (model, permission mode, rules)
class QueryEngine;
void apply_config_to_engine(const Config& config, QueryEngine& engine);

// Build merged RuntimeSettings with proper override logic
RuntimeSettings build_runtime_settings(const std::filesystem::path& working_dir);

}  // namespace ccmake
