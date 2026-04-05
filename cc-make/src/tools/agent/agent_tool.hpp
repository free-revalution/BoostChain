#pragma once

#include "tools/tool.hpp"
#include "query/query_engine.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace ccmake {

class AgentTool : public ToolBase {
public:
    using EngineFactory = std::function<std::unique_ptr<QueryEngine>()>;

    explicit AgentTool(EngineFactory factory);
    const ToolDefinition& definition() const override;
    std::string validate_input(const nlohmann::json& input, const ToolContext& ctx) const override;
    ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) override;

private:
    EngineFactory engine_factory_;
    ToolDefinition def_;
};

}  // namespace ccmake
