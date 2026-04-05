#pragma once

#include "tools/tool.hpp"

namespace ccmake {

class WebFetchTool : public ToolBase {
public:
    WebFetchTool();
    const ToolDefinition& definition() const override;
    std::string validate_input(const nlohmann::json& input, const ToolContext& ctx) const override;
    ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) override;

private:
    ToolDefinition def_;
    static std::string strip_html_tags(const std::string& html);
    static std::string decode_html_entities(const std::string& text);
};

}  // namespace ccmake
