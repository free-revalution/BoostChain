#include "tools/agent/agent_tool.hpp"
#include "query/types.hpp"

#include <spdlog/spdlog.h>

namespace ccmake {

AgentTool::AgentTool(EngineFactory factory) : engine_factory_(std::move(factory)) {
    def_ = make_tool_def(
        "Agent",
        "Spawn a subagent to work on an independent task using its own QueryEngine instance.",
        {
            {"type", "object"},
            {"properties", {
                {"prompt", {{"type", "string"}, {"description", "The task description for the subagent"}}},
                {"max_turns", {{"type", "integer"}, {"description", "Maximum agentic loop turns (default: 10)"}}}
            }},
            {"required", nlohmann::json::array({"prompt"})}
        },
        {},
        true,   // read-only
        false   // not destructive
    );
}

const ToolDefinition& AgentTool::definition() const {
    return def_;
}

std::string AgentTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    (void)ctx;

    if (!input.contains("prompt") || !input["prompt"].is_string()) {
        return "Missing required field: prompt (string)";
    }
    if (input["prompt"].get<std::string>().empty()) {
        return "Field 'prompt' must not be empty";
    }
    return "";
}

ToolOutput AgentTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    (void)ctx;

    auto prompt = input["prompt"].get<std::string>();
    int max_turns = 10;
    if (input.contains("max_turns") && input["max_turns"].is_number_integer()) {
        max_turns = input["max_turns"].get<int>();
    }

    try {
        auto engine = engine_factory_();
        engine->set_max_turns(max_turns);
        engine->set_permission_mode(PermissionMode::BypassPermissions);

        spdlog::debug("AgentTool: spawning subagent with {} max turns", max_turns);

        auto result = engine->submit_message(prompt);

        if (result.exit_reason == LoopExitReason::ModelError) {
            return ToolOutput{"Agent error: " + result.error_message, true};
        }

        // Extract final assistant text
        std::string output;
        for (const auto& msg : result.messages) {
            if (msg.role == MessageRole::Assistant) {
                for (const auto& block : msg.content) {
                    if (auto* text = std::get_if<TextBlock>(&block)) {
                        if (!output.empty()) output += "\n";
                        output += text->text;
                    }
                }
            }
        }

        if (output.empty()) {
            output = "[Agent completed with no text output]";
        }

        spdlog::debug("AgentTool: subagent completed");
        return ToolOutput{output, false};
    } catch (const std::exception& e) {
        return ToolOutput{std::string("Agent error: ") + e.what(), true};
    }
}

}  // namespace ccmake
