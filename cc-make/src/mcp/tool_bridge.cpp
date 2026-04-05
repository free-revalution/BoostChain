#include "mcp/tool_bridge.hpp"

#include <spdlog/spdlog.h>

namespace ccmake {

// ============================================================
// McpToolAdapter
// ============================================================

McpToolAdapter::McpToolAdapter(
    std::string full_name,
    std::string description,
    nlohmann::json input_schema,
    McpClient& client,
    std::string mcp_tool_name
) : client_(client),
    mcp_tool_name_(std::move(mcp_tool_name)) {

    def_ = make_tool_def(
        full_name,
        description,
        input_schema,
        {},     // no aliases
        true,   // read-only
        false,  // not destructive
        false   // not concurrency-safe
    );
}

const ToolDefinition& McpToolAdapter::definition() const {
    return def_;
}

ToolOutput McpToolAdapter::execute(const nlohmann::json& input, const ToolContext& ctx) {
    (void)ctx;

    auto result = client_.call_tool(mcp_tool_name_, input);

    if (!result.has_value()) {
        return ToolOutput{result.error(), true};
    }

    auto& resp = result.value();

    // MCP tool results have a "content" array
    if (resp.contains("content") && resp["content"].is_array()) {
        std::string text;
        bool has_error = false;

        for (const auto& item : resp["content"]) {
            if (item.contains("text")) {
                if (!text.empty()) text += "\n";
                text += item["text"].get<std::string>();
            }
            if (item.contains("isError") && item["isError"].get<bool>()) {
                has_error = true;
            }
        }

        return ToolOutput{text, has_error};
    }

    // Fallback: return the raw JSON
    return ToolOutput{resp.dump(), false};
}

// ============================================================
// McpToolBridge
// ============================================================

McpToolBridge::McpToolBridge(McpClient& client) : client_(client) {}

Result<std::vector<std::unique_ptr<ToolBase>>, std::string> McpToolBridge::create_tools() {
    auto tools_result = client_.list_tools();
    if (!tools_result.has_value()) {
        return Result<std::vector<std::unique_ptr<ToolBase>>, std::string>::err(
            "Failed to list tools: " + tools_result.error());
    }

    std::vector<std::unique_ptr<ToolBase>> tools;
    auto& mcp_tools = tools_result.value();

    for (const auto& t : mcp_tools) {
        std::string full_name = "mcp__" + client_.server_name() + "__" + t.name;

        auto adapter = std::make_unique<McpToolAdapter>(
            full_name,
            t.description,
            t.input_schema,
            client_,
            t.name
        );

        tools.push_back(std::move(adapter));
        spdlog::debug("MCP tool registered: {}", full_name);
    }

    return Result<std::vector<std::unique_ptr<ToolBase>>, std::string>::ok(std::move(tools));
}

const std::string& McpToolBridge::server_name() const {
    return client_.server_name();
}

}  // namespace ccmake
