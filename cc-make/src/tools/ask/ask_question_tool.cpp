#include "tools/ask/ask_question_tool.hpp"

#include <sstream>

namespace ccmake {

AskUserQuestionTool::AskUserQuestionTool() = default;

const ToolDefinition& AskUserQuestionTool::definition() const {
    static ToolDefinition def = [] {
        ToolDefinition d;
        d.name = "AskUserQuestion";
        d.description = "Ask the user a question during execution. Returns the question text.";
        d.input_schema = {
            {"type", "object"},
            {"properties", {
                {"question", {{"type", "string"}, {"description", "The question to ask the user"}}},
                {"options", {{"type", "array"}, {"items", {{"type", "object"}, {"properties", {
                    {"label", {{"type", "string"}}},
                    {"description", {{"type", "string"}}}
                }}, {"required", {"label"}}}}, {"description", "Optional list of choices for the user"}}}
            }},
            {"required", {"question"}}
        };
        d.is_read_only = true;
        d.is_destructive = false;
        d.enabled = true;
        return d;
    }();
    return def;
}

std::string AskUserQuestionTool::validate_input(const nlohmann::json& input, const ToolContext& ctx) const {
    (void)ctx;
    if (!input.contains("question") || !input["question"].is_string()) {
        return "question is required and must be a string";
    }
    if (input["question"].get<std::string>().empty()) {
        return "question must not be empty";
    }
    return "";
}

ToolOutput AskUserQuestionTool::execute(const nlohmann::json& input, const ToolContext& ctx) {
    (void)ctx;
    auto question = input["question"].get<std::string>();

    std::ostringstream oss;
    oss << "Question: " << question;

    if (input.contains("options") && input["options"].is_array()) {
        oss << "\nOptions:\n";
        for (const auto& opt : input["options"]) {
            auto label = opt.value("label", std::string("(no label)"));
            auto desc = opt.value("description", std::string(""));
            oss << "  - " << label;
            if (!desc.empty()) {
                oss << ": " << desc;
            }
            oss << "\n";
        }
    }

    return {oss.str(), false};
}

}  // namespace ccmake
