#pragma once

#include "tools/tool.hpp"

namespace ccmake {

class GlobTool : public ToolBase {
public:
    GlobTool();
    const ToolDefinition& definition() const override;
    std::string validate_input(const nlohmann::json& input, const ToolContext& ctx) const override;
    ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) override;

private:
    static bool match_glob(const std::string& text, const std::string& pattern);
    static std::vector<std::string> split_path(const std::string& path);
};

}  // namespace ccmake
