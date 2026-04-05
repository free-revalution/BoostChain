#include "permissions/permission_manager.hpp"

namespace ccmake {

void PermissionManager::set_mode(PermissionMode mode) {
    ctx_.mode = mode;
}

PermissionMode PermissionManager::mode() const {
    return ctx_.mode;
}

void PermissionManager::add_allow_rule(const std::string& tool_pattern, const std::string& source) {
    PermissionCheckRule rule;
    rule.source = source;
    rule.behavior = PermissionBehavior::Allow;
    rule.tool_name = tool_pattern;
    ctx_.rules.push_back(std::move(rule));
}

void PermissionManager::add_deny_rule(const std::string& tool_pattern, const std::string& source) {
    PermissionCheckRule rule;
    rule.source = source;
    rule.behavior = PermissionBehavior::Deny;
    rule.tool_name = tool_pattern;
    ctx_.rules.push_back(std::move(rule));
}

void PermissionManager::clear_rules() {
    ctx_.rules.clear();
}

const PermissionContext& PermissionManager::context() const {
    return ctx_;
}

PermissionResult PermissionManager::check(const std::string& tool_name, const std::string& input_content) {
    return check_permission(tool_name, input_content, ctx_);
}

PermissionResult PermissionManager::check_with_memo(const std::string& tool_name, const std::string& input_content) {
    // Already auto-approved? Allow immediately
    if (approved_tools_.count(tool_name)) {
        return {PermissionBehavior::Allow, "", "memo"};
    }

    auto result = check_permission(tool_name, input_content, ctx_);

    if (result.behavior == PermissionBehavior::Allow) {
        return result;
    }

    if (result.behavior == PermissionBehavior::Deny) {
        return result;
    }

    // Ask mode — need user approval
    if (approval_callback_) {
        bool approved = approval_callback_(tool_name, input_content);
        if (approved) {
            approved_tools_.insert(tool_name);
            return {PermissionBehavior::Allow, "", "user_approved"};
        } else {
            return {PermissionBehavior::Deny, "User denied tool '" + tool_name + "'", "user_denied"};
        }
    }

    // No callback set — return Ask
    return result;
}

void PermissionManager::set_approval_callback(ApprovalCallback callback) {
    approval_callback_ = std::move(callback);
}

void PermissionManager::reset_approvals() {
    approved_tools_.clear();
}

bool PermissionManager::is_approved(const std::string& tool_name) const {
    return approved_tools_.count(tool_name) > 0;
}

}  // namespace ccmake
