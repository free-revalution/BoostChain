#pragma once

#include "tools/tool.hpp"

namespace ccmake {

class AskUserQuestionTool : public ToolBase {
public:
    AskUserQuestionTool();

    const ToolDefinition& definition() const override;
    std::string validate_input(const nlohmann::json& input, const ToolContext& ctx) const override;
    ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) override;
};

}  // namespace ccmake
