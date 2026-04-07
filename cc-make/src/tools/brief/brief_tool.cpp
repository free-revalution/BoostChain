#include "tools/brief/brief_tool.hpp"

namespace ccmake {

BriefTool::BriefTool() = default;

const ToolDefinition& BriefTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "Brief";
        d.description = "Create a brief summary by truncating content to a maximum length at word boundaries.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"content", {{"type", "string"}, {"description", "The content to summarize"}}},
                {"max_length", {{"type", "integer"}, {"description", "Maximum length of the brief (default 500)"}}}
            }},
            {"required", {"content"}}
        };
        d.is_read_only = true;
        d.is_destructive = false;
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string BriefTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    (void)ctx;
    if (!input.contains("content") || !input["content"].is_string()) {
        return "content is required and must be a string";
    }
    return "";
}

ToolOutput BriefTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    (void)ctx;
    auto content = input["content"].get<std::string>();
    int max_length = input.value("max_length", 500);

    if (static_cast<int>(content.size()) <= max_length) {
        return {content, false};
    }

    // Truncate at word boundary
    auto trunc = content.substr(0, static_cast<size_t>(max_length));

    // Find the last space within the truncated portion
    auto last_space = trunc.rfind(' ');
    if (last_space != std::string::npos && last_space > static_cast<size_t>(max_length * 0.5)) {
        trunc = trunc.substr(0, last_space);
    }

    trunc += "...";
    return {trunc, false};
}

}  // namespace ccmake
