#pragma once
#include <string>
#include <functional>
#include "permissions/permissions.hpp"

namespace ccmake {

// Represents a permission prompt to show the user
struct PermissionPrompt {
    std::string tool_name;           // e.g. "Bash", "Write"
    std::string input_summary;       // Brief summary of what the tool will do
    std::string full_input;          // Full tool input (for detailed view)
    bool is_destructive = false;     // Whether this is a destructive action
    bool is_new_tool = true;         // Whether this tool hasn't been approved before
};

// Result of a permission prompt interaction
enum class PromptResult {
    Allow,        // User approved
    Deny,         // User denied
    AllowAlways,  // User approved and wants to remember
    Abort         // User wants to abort the entire query
};

class PermissionDialog {
public:
    PermissionDialog();

    // Render a permission prompt to string (for terminal output)
    std::string render(const PermissionPrompt& prompt) const;

    // Parse user response to a prompt
    // "y" or "yes" -> Allow, "n" or "no" -> Deny, "a" -> AllowAlways, "escape" -> Abort
    PromptResult parse_response(const std::string& response) const;

    // Format a permission result for display
    static std::string format_result(PromptResult result, const std::string& tool_name);

    // Create an ApprovalCallback for use with PermissionManager
    std::function<bool(const std::string&, const std::string&)> create_callback();

    // Set whether to use ANSI colors in output
    void set_use_colors(bool use);

private:
    // Build the summary line for a tool
    std::string build_summary(const PermissionPrompt& prompt) const;

    // Truncate long strings for display
    std::string truncate(const std::string& text, int max_len) const;

    bool use_colors_ = true;
};

}  // namespace ccmake
