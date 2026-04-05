#pragma once

#include "mcp/client.hpp"
#include "mcp/types.hpp"
#include "tools/tool.hpp"
#include "core/result.hpp"

#include <memory>
#include <string>
#include <vector>

namespace ccmake {

// Adapter that wraps an MCP tool as a ToolBase for the tool registry
// Tool names are prefixed: mcp__{server_name}__{tool_name}
class McpToolAdapter : public ToolBase {
public:
    McpToolAdapter(
        std::string full_name,
        std::string description,
        nlohmann::json input_schema,
        McpClient& client,
        std::string mcp_tool_name
    );

    const ToolDefinition& definition() const override;
    ToolOutput execute(const nlohmann::json& input, const ToolContext& ctx) override;

private:
    ToolDefinition def_;
    McpClient& client_;
    std::string mcp_tool_name_;
};

// Creates ToolBase instances for all tools exposed by an MCP server
class McpToolBridge {
public:
    explicit McpToolBridge(McpClient& client);

    // Discover tools from the server and create ToolBase instances
    Result<std::vector<std::unique_ptr<ToolBase>>, std::string> create_tools();

    const std::string& server_name() const;

private:
    McpClient& client_;
};

}  // namespace ccmake
