#pragma once

#include "tools/tool.hpp"

namespace ccmake {

class WriteTool : public ToolBase {
public:
    WriteTool();
    const ToolDefinition& definition() const override;
    std::string validate_input(const nlohmann::json& input, const ToolContext& ctx) const override;
    ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) override;
};

}  // namespace ccmake
