#pragma once

#include "permissions/permissions.hpp"

#include <string>
#include <vector>
#include <unordered_set>
#include <functional>

namespace ccmake {

class PermissionManager {
public:
    // Callback type: called when a tool needs user approval
    // Returns true if user approved, false if denied
    using ApprovalCallback = std::function<bool(const std::string& tool_name, const std::string& input_summary)>;

    // Set/get permission mode
    void set_mode(PermissionMode mode);
    PermissionMode mode() const;

    // Rule management
    void add_allow_rule(const std::string& tool_pattern, const std::string& source = "user");
    void add_deny_rule(const std::string& tool_pattern, const std::string& source = "user");
    void clear_rules();

    // Get the permission context (for check_permission function)
    const PermissionContext& context() const;

    // Check if a tool execution is permitted
    // Returns the permission behavior (Allow, Deny, Ask)
    PermissionResult check(const std::string& tool_name, const std::string& input_content = "");

    // Check and auto-approve if previously approved (memoization)
    // If behavior is Ask and callback is set, calls callback for approval
    // If approved, remembers the decision
    PermissionResult check_with_memo(const std::string& tool_name, const std::string& input_content = "");

    // Set approval callback (for interactive approval)
    void set_approval_callback(ApprovalCallback callback);

    // Reset memoized approvals
    void reset_approvals();

    // Check if a tool was previously auto-approved
    bool is_approved(const std::string& tool_name) const;

private:
    PermissionContext ctx_;
    std::unordered_set<std::string> approved_tools_;
    ApprovalCallback approval_callback_;
};

}  // namespace ccmake
