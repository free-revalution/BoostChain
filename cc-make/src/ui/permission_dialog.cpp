#include "ui/permission_dialog.hpp"
#include "terminal/ansi.hpp"

#include <algorithm>
#include <iostream>
#include <sstream>

namespace ccmake {

PermissionDialog::PermissionDialog() = default;

void PermissionDialog::set_use_colors(bool use) {
    use_colors_ = use;
}

std::string PermissionDialog::truncate(const std::string& text, int max_len) const {
    if (static_cast<int>(text.size()) <= max_len) {
        return text;
    }
    return text.substr(0, max_len - 3) + "...";
}

std::string PermissionDialog::build_summary(const PermissionPrompt& prompt) const {
    if (prompt.tool_name == "Bash") {
        // For Bash: show the command
        return truncate(prompt.full_input, 200);
    } else if (prompt.tool_name == "Write" || prompt.tool_name == "Edit") {
        // For Write/Edit: show the file path
        // Try to extract the file path from the input (usually first line or "file_path" field)
        if (!prompt.input_summary.empty()) {
            return truncate(prompt.input_summary, 200);
        }
        return truncate(prompt.full_input, 200);
    } else {
        // For others: show tool name + first line of input
        std::string first_line = prompt.full_input;
        auto nl = first_line.find('\n');
        if (nl != std::string::npos) {
            first_line = first_line.substr(0, nl);
        }
        return truncate(prompt.tool_name + ": " + first_line, 200);
    }
}

std::string PermissionDialog::render(const PermissionPrompt& prompt) const {
    std::ostringstream oss;

    // Optional tag line: destructive and/or new tool markers
    if (use_colors_) {
        if (prompt.is_destructive) {
            oss << ansi::fg_red() << ansi::bold() << "  ! Destructive action" << ansi::reset() << "\n";
        }
        if (prompt.is_new_tool) {
            oss << ansi::fg_yellow() << "  (new tool)" << ansi::reset() << "\n";
        }
    } else {
        if (prompt.is_destructive) {
            oss << "  ! Destructive action\n";
        }
        if (prompt.is_new_tool) {
            oss << "  (new tool)\n";
        }
    }

    oss << "\n";

    // Tool name
    if (use_colors_) {
        oss << ansi::fg_cyan() << ansi::bold() << "  Tool: " << ansi::reset();
    } else {
        oss << "  Tool: ";
    }
    oss << prompt.tool_name << "\n";

    // Input summary
    if (use_colors_) {
        oss << ansi::fg_cyan() << ansi::bold() << "  Input: " << ansi::reset();
    } else {
        oss << "  Input: ";
    }
    oss << build_summary(prompt) << "\n";

    // Action type
    if (use_colors_) {
        oss << ansi::fg_cyan() << ansi::bold() << "  Action: " << ansi::reset();
    } else {
        oss << "  Action: ";
    }
    if (prompt.is_destructive) {
        if (use_colors_) {
            oss << ansi::fg_red() << "Destructive" << ansi::reset();
        } else {
            oss << "Destructive";
        }
    } else {
        oss << "Read";
    }
    oss << "\n";

    oss << "\n";

    // Prompt line
    oss << "  Allow? (y)es / (n)o / (a)lways / (Esc)abort";

    return oss.str();
}

PromptResult PermissionDialog::parse_response(const std::string& response) const {
    // Trim whitespace
    std::string trimmed = response;
    while (!trimmed.empty() && (trimmed.front() == ' ' || trimmed.front() == '\t')) {
        trimmed.erase(trimmed.begin());
    }
    while (!trimmed.empty() && (trimmed.back() == ' ' || trimmed.back() == '\t' || trimmed.back() == '\n' || trimmed.back() == '\r')) {
        trimmed.pop_back();
    }

    // Convert to lowercase for comparison
    std::string lower;
    lower.reserve(trimmed.size());
    for (char c : trimmed) {
        lower.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    if (lower == "y" || lower == "yes") {
        return PromptResult::Allow;
    } else if (lower == "n" || lower == "no") {
        return PromptResult::Deny;
    } else if (lower == "a" || lower == "always") {
        return PromptResult::AllowAlways;
    } else if (lower == "escape") {
        return PromptResult::Abort;
    } else {
        // Safe default: deny
        return PromptResult::Deny;
    }
}

std::string PermissionDialog::format_result(PromptResult result, const std::string& tool_name) {
    switch (result) {
        case PromptResult::Allow:
            return "\xe2\x9c\x93 Allowed: " + tool_name;
        case PromptResult::Deny:
            return "\xe2\x9c\x97 Denied: " + tool_name;
        case PromptResult::AllowAlways:
            return "\xe2\x9c\x93 Always allowed: " + tool_name;
        case PromptResult::Abort:
            return "\xe2\x9a\xa1 Aborted";
    }
    return "";
}

std::function<bool(const std::string&, const std::string&)> PermissionDialog::create_callback() {
    return [this](const std::string& tool_name, const std::string& input_summary) -> bool {
        PermissionPrompt prompt;
        prompt.tool_name = tool_name;
        prompt.input_summary = input_summary;
        prompt.full_input = input_summary;
        prompt.is_destructive = false;
        prompt.is_new_tool = true;

        std::string output = render(prompt);
        std::cout << output << std::endl;

        std::string response;
        if (std::getline(std::cin, response)) {
            PromptResult result = parse_response(response);
            return (result == PromptResult::Allow || result == PromptResult::AllowAlways);
        }
        return false;
    };
}

}  // namespace ccmake
