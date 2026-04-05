#pragma once

#include "core/types.hpp"

#include <string>
#include <vector>

namespace ccmake {

// ============================================================
// Permission Check Rule (extended from config's PermissionRule)
// ============================================================

struct PermissionCheckRule {
    std::string source;       // Where the rule came from (userSettings, projectSettings, etc.)
    PermissionBehavior behavior;
    std::string tool_name;    // Tool name pattern (e.g., "Bash", "*")
    std::string rule_content; // Optional content filter (e.g., "Bash(git push:*)")
};

struct PermissionResult {
    PermissionBehavior behavior = PermissionBehavior::Ask;
    std::string message;
    std::string decision_reason;
};

// ============================================================
// Permission Context
// ============================================================

struct PermissionContext {
    PermissionMode mode = PermissionMode::Default;
    std::vector<PermissionCheckRule> rules;
    std::string cwd;
};

// ============================================================
// Permission Check
// ============================================================

PermissionResult check_permission(
    const std::string& tool_name,
    const std::string& input_content,
    const PermissionContext& ctx
);

void add_rule(PermissionContext& ctx, PermissionCheckRule rule);
bool is_denied(const std::string& tool_name, const PermissionContext& ctx);
bool is_allowed(const std::string& tool_name, const PermissionContext& ctx);

}  // namespace ccmake
