#include "permissions/permissions.hpp"
#include <algorithm>

namespace ccmake {

void add_rule(PermissionContext& ctx, PermissionCheckRule rule) {
    ctx.rules.push_back(std::move(rule));
}

static bool tool_matches_rule(const std::string& tool_name, const std::string& rule_pattern) {
    if (rule_pattern.empty()) return false;
    if (rule_pattern == "*") return true;
    return tool_name == rule_pattern;
}

bool is_denied(const std::string& tool_name, const PermissionContext& ctx) {
    for (const auto& rule : ctx.rules) {
        if (rule.behavior == PermissionBehavior::Deny &&
            tool_matches_rule(tool_name, rule.tool_name)) {
            return true;
        }
    }
    return false;
}

bool is_allowed(const std::string& tool_name, const PermissionContext& ctx) {
    for (const auto& rule : ctx.rules) {
        if (rule.behavior == PermissionBehavior::Allow &&
            tool_matches_rule(tool_name, rule.tool_name)) {
            return true;
        }
    }
    return false;
}

PermissionResult check_permission(
    const std::string& tool_name,
    const std::string& input_content,
    const PermissionContext& ctx
) {
    // Step 1: Check deny rules first (highest priority)
    for (const auto& rule : ctx.rules) {
        if (rule.behavior == PermissionBehavior::Deny &&
            tool_matches_rule(tool_name, rule.tool_name)) {
            return {
                PermissionBehavior::Deny,
                "Tool '" + tool_name + "' is denied by " + rule.source + " rule",
                "rule"
            };
        }
    }

    // Step 2: Check allow rules
    for (const auto& rule : ctx.rules) {
        if (rule.behavior == PermissionBehavior::Allow &&
            tool_matches_rule(tool_name, rule.tool_name)) {
            return {
                PermissionBehavior::Allow,
                "",
                "rule"
            };
        }
    }

    // Step 3: Mode-based decision
    switch (ctx.mode) {
        case PermissionMode::BypassPermissions:
            return { PermissionBehavior::Allow, "", "mode" };
        case PermissionMode::AcceptEdits:
            return { PermissionBehavior::Allow, "", "mode" };
        case PermissionMode::Plan:
            return {
                PermissionBehavior::Ask,
                "Plan mode: tool '" + tool_name + "' requires approval",
                "mode"
            };
        case PermissionMode::Default:
        default:
            return {
                PermissionBehavior::Ask,
                "Tool '" + tool_name + "' requires approval",
                "mode"
            };
    }
}

}  // namespace ccmake
