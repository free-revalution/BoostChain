#include "tools/bash/bash_tool.hpp"
#include "utils/process.hpp"

#include <sstream>

namespace ccmake {

BashTool::BashTool() = default;

const ToolDefinition& BashTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Bash";
        d.description = "Execute a shell command and return its output.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"command", {{"type", "string"}, {"description", "The shell command to execute"}}},
                {"timeout", {{"type", "integer"}, {"description", "Timeout in milliseconds (max 600000)"}}}
            }},
            {"required", {"command"}}
        };
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string BashTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    if (!input.contains("command") || !input["command"].is_string()) {
        return "command is required and must be a string";
    }
    auto cmd = input["command"].get<std::string>();
    if (cmd.empty()) {
        return "command must not be empty";
    }
    return "";
}

ToolOutput BashTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    auto command = input["command"].get<std::string>();
    int timeout_ms = input.value("timeout", 120000);
    if (timeout_ms > 600000) timeout_ms = 600000;

    std::optional<std::string> cwd_opt;
    if (!ctx.cwd.empty()) cwd_opt = ctx.cwd;

    auto result = run_command(
        command,
        cwd_opt,
        std::chrono::milliseconds(timeout_ms)
    );

    std::ostringstream output;
    output << "Exit code: " << result.exit_code << "\n";
    if (result.timed_out) {
        output << "(timed out after " << timeout_ms << "ms)\n";
    }
    if (!result.stdout_output.empty()) {
        output << "\nstdout:\n" << result.stdout_output;
    }
    if (!result.stderr_output.empty()) {
        output << "\nstderr:\n" << result.stderr_output;
    }

    if (result.exit_code != 0) {
        return {output.str(), true};
    }

    return {output.str(), false};
}

}  // namespace ccmake
